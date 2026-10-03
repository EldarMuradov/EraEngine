#include "physics/physical_animation/ragdoll_drive_compiler.h"
#include "physics/physical_animation/physical_animation_component.h"
#include "physics/core/physics.h"
#include "physics/body_component.h"
#include "physics/joint.h"
#include "physics/physx_api.h"

#include <algorithm>

namespace era_engine::physics
{
	static constexpr float TWO_PI = 2.0f * M_PI;

	static physx::PxRigidDynamic* get_limb_rigid_dynamic(Entity limb)
	{
		const DynamicBodyComponent* body_component = limb.get_component_if_exists<DynamicBodyComponent>();
		return body_component != nullptr ? body_component->get_rigid_dynamic() : nullptr;
	}

	static vec2 make_error_range(vec2 range)
	{
		range.x = std::max(range.x, 0.0f);
		range.y = std::max(range.y, range.x + 1e-4f);
		return range;
	}

	CompiledDriveValues CompiledDriveValues::lerp(const CompiledDriveValues& from, const CompiledDriveValues& to, float t)
	{
		CompiledDriveValues result;
		result.stiffness = ::lerp(from.stiffness, to.stiffness, t);
		result.damping_range = ::lerp(from.damping_range, to.damping_range, t);
		result.error_range = ::lerp(from.error_range, to.error_range, t);
		result.force_limit = ::lerp(from.force_limit, to.force_limit, t);
		result.accelerated = to.accelerated;
		return result;
	}

	CompiledLimbDrive CompiledLimbDrive::lerp(const CompiledLimbDrive& from, const CompiledLimbDrive& to, float t)
	{
		CompiledLimbDrive result;

		result.angular = CompiledDriveValues::lerp(from.angular, to.angular, t);
		result.linear = CompiledDriveValues::lerp(from.linear, to.linear, t);

		result.angular_frequency = ::lerp(from.angular_frequency, to.angular_frequency, t);
		result.angular_damping_ratio = ::lerp(from.angular_damping_ratio, to.angular_damping_ratio, t);
		result.torque_stiffness = ::lerp(from.torque_stiffness, to.torque_stiffness, t);
		result.max_torque = ::lerp(from.max_torque, to.max_torque, t);

		result.linear_frequency = ::lerp(from.linear_frequency, to.linear_frequency, t);
		result.linear_damping_ratio = ::lerp(from.linear_damping_ratio, to.linear_damping_ratio, t);
		result.max_force = ::lerp(from.max_force, to.max_force, t);

		return result;
	}

	LimbMassProperties RagdollDriveCompiler::compute_limb_mass_properties(const PhysicalAnimationLimbComponent* limb_component)
	{
		using namespace physx;

		LimbMassProperties result;

		ASSERT(limb_component != nullptr);

		const PxRigidDynamic* rigid_dynamic = get_limb_rigid_dynamic(limb_component->get_entity());
		if (rigid_dynamic == nullptr)
		{
			return result;
		}

		// Drive anchor in the limb actor frame (joint frame of actor1 of the drive joint).
		PxVec3 anchor = PxVec3(0.0f);
		if (const D6JointComponent* drive_joint = dynamic_cast<const D6JointComponent*>(limb_component->drive_joint_component.get()))
		{
			anchor = create_PxVec3(drive_joint->get_second_local_frame().position);
		}

		PhysicsEngine::execute_read([&]()
		{
			const float mass = rigid_dynamic->getMass();
			const PxTransform cmass_pose = rigid_dynamic->getCMassLocalPose();
			const PxVec3 inertia = rigid_dynamic->getMassSpaceInertiaTensor();

			if (mass <= 0.0f || inertia.x <= 0.0f || inertia.y <= 0.0f || inertia.z <= 0.0f)
			{
				return;
			}

			// Lever arm from the centre of mass to the anchor, in the actor frame.
			const PxVec3 lever = anchor - cmass_pose.p;

			float best_ratio = 0.0f;
			float effective_mass_sum = 0.0f;

			for (uint32 axis = 0; axis < 3; ++axis)
			{
				PxVec3 unit_axis = PxVec3(0.0f);
				unit_axis[axis] = 1.0f;

				const PxVec3 n = cmass_pose.q.rotate(unit_axis);

				const float inertia_cm = inertia[axis];
				const float distance_sq = std::max(lever.magnitudeSquared() - lever.dot(n) * lever.dot(n), 0.0f);
				const float inertia_pivot = inertia_cm + mass * distance_sq;

				const float ratio = inertia_pivot / inertia_cm;
				if (ratio > best_ratio)
				{
					best_ratio = ratio;
					result.inertia_cm = inertia_cm;
					result.inertia_pivot = inertia_pivot;
				}

				// Linear row along n at the anchor: 1 / m_eff = 1 / m + (r x n)^T * I^-1 * (r x n).
				const PxVec3 angular_part = cmass_pose.q.rotateInv(lever.cross(n));
				const float angular_response = angular_part.x * angular_part.x / inertia.x +
					angular_part.y * angular_part.y / inertia.y +
					angular_part.z * angular_part.z / inertia.z;

				effective_mass_sum += 1.0f / (1.0f / mass + angular_response);
			}

			result.mass = mass;
			result.linear_effective_mass = effective_mass_sum / 3.0f;
			result.valid = true;
		});

		return result;
	}

	CompiledLimbDrive RagdollDriveCompiler::compile_limb_drive(const MotorDriveSettings& settings, const LimbMassProperties& mass_properties, float strength)
	{
		CompiledLimbDrive result;

		if (!mass_properties.valid)
		{
			return result;
		}

		// Strength scales stiffness, damping scales with sqrt(strength), so damping ratios stay the same.
		const float strength_coeff = std::max(strength, 0.0f);
		const float sqrt_strength_coeff = std::sqrt(strength_coeff);

		// Angular drive.
		{
			const AngularDriveSettings& angular = settings.angular;

			const float omega = TWO_PI * std::max(angular.frequency, 0.0f);
			const float torque_stiffness = mass_properties.inertia_pivot * omega * omega;
			const float inertia_ratio = mass_properties.get_inertia_ratio();

			result.angular.stiffness = torque_stiffness / mass_properties.inertia_cm * strength_coeff;
			result.angular.damping_range = vec2(angular.damping_ratio, angular.far_damping_ratio) * (2.0f * omega * inertia_ratio * sqrt_strength_coeff);
			result.angular.error_range = make_error_range(vec2(deg2rad(angular.error_range_deg.x), deg2rad(angular.error_range_deg.y)));

			result.torque_stiffness = torque_stiffness * strength_coeff;
			result.max_torque = result.torque_stiffness * deg2rad(angular.saturation_angle_deg);

			// Slerp rows use half-length axes (PxD6Joint slerp Jacobian): the solver applies 0.5 * limit as torque per axis.
			result.angular.force_limit = 2.0f * result.max_torque;
			result.angular.accelerated = true;

			result.angular_frequency = omega * sqrt_strength_coeff / TWO_PI;
			result.angular_damping_ratio = vec2(angular.damping_ratio, angular.far_damping_ratio);
		}

		// Linear X/Y/Z drives at the anchor, designed for the limb mass.
		{
			const LinearDriveSettings& linear = settings.linear;

			const float omega = TWO_PI * std::max(linear.frequency, 0.0f);
			const float mass_ratio = mass_properties.get_linear_mass_ratio();

			result.linear.stiffness = omega * omega * mass_ratio * strength_coeff;
			result.linear.damping_range = vec2(linear.damping_ratio, linear.far_damping_ratio) * (2.0f * omega * mass_ratio * sqrt_strength_coeff);
			result.linear.error_range = make_error_range(linear.error_range);

			// Force at the anchor is m_eff * k * x = m * w^2 * x.
			result.max_force = mass_properties.mass * omega * omega * strength_coeff * linear.saturation_distance;
			result.linear.force_limit = result.max_force;
			result.linear.accelerated = true;

			result.linear_frequency = omega * sqrt_strength_coeff / TWO_PI;
			result.linear_damping_ratio = vec2(linear.damping_ratio, linear.far_damping_ratio);
		}

		return result;
	}
}
