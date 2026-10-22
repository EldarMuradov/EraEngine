#include "physics/vehicles/vehicle_simulation.h"

#include <core/log.h>

#include <format>

namespace era_engine::physics
{
    using namespace physx;

    static constexpr float DIFF_BIAS = 1.3f;
    static constexpr float DIFF_TARGET = 1.29f;
    static constexpr float LOW_SPEED_SUBSTEPS_THRESHOLD = 5.0f;
    static constexpr float STABILITY_MIN_SPEED = 3.0f;
    static constexpr float STABILITY_GAIN = 8.0f;
    static constexpr float STABILITY_YAW_MARGIN = 0.05f;

    PxQueryHitType::Enum VehicleRoadQueryFilter::preFilter(const PxFilterData& filter_data,
        const PxShape* shape,
        const PxRigidActor* actor,
        PxHitFlags& query_flags)
    {
        if (actor == self || (shape->getFlags() & PxShapeFlag::eTRIGGER_SHAPE))
        {
            return PxQueryHitType::eNONE;
        }

        if (collision_mask != 0)
        {
            const uint32 shape_type = shape->getQueryFilterData().word0;
            if (shape_type != 0 && (shape_type & collision_mask) == 0)
            {
                return PxQueryHitType::eNONE;
            }
        }

        return PxQueryHitType::eBLOCK;
    }

    PxQueryHitType::Enum VehicleRoadQueryFilter::postFilter(const PxFilterData& filter_data,
        const PxQueryHit& hit,
        const PxShape* shape,
        const PxRigidActor* actor)
    {
        return PxQueryHitType::eBLOCK;
    }

    VehicleSimulation::VehicleSimulation()
    {
        mPhysXState.setToDefault();
    }

    uint32 VehicleSimulation::get_wheel_id(uint32 axle, bool left)
    {
        return axle * 2 + (left ? 1 : 0);
    }

    bool VehicleSimulation::build_params(const VehicleConfig& config, std::string& error)
    {
        const uint32 nb_axles = static_cast<uint32>(config.axles.size());
        if (nb_axles < 2 || nb_axles > MAX_AXLES)
        {
            error = std::format("Vehicle needs 2..{} axles, got {}.", MAX_AXLES, nb_axles);
            return false;
        }

        const VehicleChassisConfig& chassis = config.chassis;
        if (chassis.mass <= 0.0f)
        {
            error = "Chassis mass must be positive.";
            return false;
        }

        const VehicleGearboxConfig& gearbox = config.gearbox;
        if (gearbox.forward_ratios.empty() || gearbox.forward_ratios.size() + 2 > PxVehicleGearboxParams::eMAX_NB_GEARS)
        {
            error = std::format("Gearbox needs 1..{} forward gears.", PxVehicleGearboxParams::eMAX_NB_GEARS - 2);
            return false;
        }

        uint32 nb_steered_axles = 0;
        uint32 nb_driven_axles = 0;
        for (const VehicleAxleConfig& axle : config.axles)
        {
            nb_steered_axles += axle.steered ? 1 : 0;
            nb_driven_axles += axle.driven ? 1 : 0;

            if (axle.wheel_radius <= 0.0f || axle.wheel_width <= 0.0f || axle.wheel_mass <= 0.0f || axle.track_width <= 0.0f)
            {
                error = "Wheel radius, width, mass and track width must be positive.";
                return false;
            }
            if (axle.suspension_frequency <= 0.0f || axle.suspension_travel <= 0.0f)
            {
                error = "Suspension frequency and travel must be positive.";
                return false;
            }
        }

        if (nb_steered_axles == 0 || nb_driven_axles == 0)
        {
            error = "Vehicle needs at least one steered and one driven axle.";
            return false;
        }

        const uint32 nb_wheels = nb_axles * 2;
        const float gravity = PX_GRAVITY.magnitude();

        snippetvehicle::BaseVehicleParams& base = mBaseParams;

        base.frame.lngAxis = PxVehicleAxes::ePosZ;
        base.frame.latAxis = PxVehicleAxes::ePosX;
        base.frame.vrtAxis = PxVehicleAxes::ePosY;
        base.scale.scale = 1.0f;

        base.axleDescription.setToDefault();
        for (uint32 axle = 0; axle < nb_axles; ++axle)
        {
            const PxU32 ids[2] = {get_wheel_id(axle, false), get_wheel_id(axle, true)};
            base.axleDescription.addAxle(2, ids);
        }

        {
            const vec3& h = chassis.collision_half_extents;
            const float box_moi_factor = chassis.mass / 3.0f;
            const PxVec3 box_moi(box_moi_factor * (h.y * h.y + h.z * h.z),
                box_moi_factor * (h.x * h.x + h.z * h.z),
                box_moi_factor * (h.x * h.x + h.y * h.y));

            base.rigidBodyParams.mass = chassis.mass;
            base.rigidBodyParams.moi = PxVec3(chassis.moi.x > 0.0f ? chassis.moi.x : box_moi.x,
                chassis.moi.y > 0.0f ? chassis.moi.y : box_moi.y,
                chassis.moi.z > 0.0f ? chassis.moi.z : box_moi.z);
        }

        const PxVec3 center_of_mass = create_PxVec3(chassis.center_of_mass);
        PxVec3 wheel_centers[PxVehicleLimits::eMAX_NB_WHEELS];
        for (uint32 axle = 0; axle < nb_axles; ++axle)
        {
            const VehicleAxleConfig& axle_config = config.axles[axle];
            const PxVec3 axle_center = create_PxVec3(axle_config.position) - center_of_mass;
            const PxVec3 half_track(axle_config.track_width * 0.5f, 0.0f, 0.0f);

            wheel_centers[get_wheel_id(axle, false)] = axle_center - half_track;
            wheel_centers[get_wheel_id(axle, true)] = axle_center + half_track;
        }

        PxReal sprung_masses[PxVehicleLimits::eMAX_NB_WHEELS];
        if (!PxVehicleComputeSprungMasses(nb_wheels, wheel_centers, chassis.mass, PxVehicleAxes::ePosY, sprung_masses))
        {
            error = "Failed to compute sprung masses.";
            return false;
        }

        for (uint32 i = 0; i < nb_wheels; ++i)
        {
            if (sprung_masses[i] <= 0.0f)
            {
                error = "Center of mass must lie inside the wheel footprint.";
                return false;
            }
        }

        base.suspensionStateCalculationParams.suspensionJounceCalculationType = PxVehicleSuspensionJounceCalculationType::eSWEEP;
        base.suspensionStateCalculationParams.limitSuspensionExpansionVelocity = true;

        float max_brake_torque = 0.0f;
        float max_handbrake_torque = 0.0f;
        for (const VehicleAxleConfig& axle : config.axles)
        {
            max_brake_torque = std::max(max_brake_torque, axle.brake_torque);
            max_handbrake_torque = std::max(max_handbrake_torque, axle.handbrake_torque);
        }

        base.brakeResponseParams[0].nonlinearResponse.clear();
        base.brakeResponseParams[0].maxResponse = max_brake_torque;
        base.brakeResponseParams[1].nonlinearResponse.clear();
        base.brakeResponseParams[1].maxResponse = max_handbrake_torque;

        base.steerResponseParams.nonlinearResponse.clear();
        base.steerResponseParams.maxResponse = PxDegToRad(config.steering.max_angle);

        nb_anti_roll = 0;
        max_grip = 0.0f;

        for (uint32 axle = 0; axle < nb_axles; ++axle)
        {
            const VehicleAxleConfig& axle_config = config.axles[axle];

            max_grip = std::max(max_grip, axle_config.tire_grip);

            const float omega = PxTwoPi * axle_config.suspension_frequency;
            const float rest_jounce = gravity / (omega * omega);
            const float travel = std::max(axle_config.suspension_travel, rest_jounce * 1.2f);

            float axle_stiffness = 0.0f;

            for (uint32 side = 0; side < 2; ++side)
            {
                const uint32 wheel_id = get_wheel_id(axle, side == 1);
                const float sprung_mass = sprung_masses[wheel_id];

                base.brakeResponseParams[0].wheelResponseMultipliers[wheel_id] = max_brake_torque > 0.0f ? axle_config.brake_torque / max_brake_torque : 0.0f;
                base.brakeResponseParams[1].wheelResponseMultipliers[wheel_id] = max_handbrake_torque > 0.0f ? axle_config.handbrake_torque / max_handbrake_torque : 0.0f;
                base.steerResponseParams.wheelResponseMultipliers[wheel_id] = axle_config.steered ? 1.0f : 0.0f;

                PxVehicleSuspensionParams& suspension = base.suspensionParams[wheel_id];
                suspension.suspensionAttachment = PxTransform(wheel_centers[wheel_id] + PxVec3(0.0f, travel - rest_jounce, 0.0f));
                suspension.suspensionTravelDir = PxVec3(0.0f, -1.0f, 0.0f);
                suspension.suspensionTravelDist = travel;
                suspension.wheelAttachment = PxTransform(PxIdentity);

                PxVehicleSuspensionComplianceParams& compliance = base.suspensionComplianceParams[wheel_id];
                compliance.wheelToeAngle.clear();
                compliance.wheelToeAngle.addPair(0.0f, 0.0f);
                compliance.wheelCamberAngle.clear();
                compliance.wheelCamberAngle.addPair(0.0f, 0.0f);
                compliance.suspForceAppPoint.clear();
                compliance.suspForceAppPoint.addPair(0.0f, PxVec3(0.0f));
                compliance.tireForceAppPoint.clear();
                compliance.tireForceAppPoint.addPair(0.0f, PxVec3(0.0f));

                PxVehicleSuspensionForceParams& force = base.suspensionForceParams[wheel_id];
                force.stiffness = sprung_mass * omega * omega;
                force.damping = 2.0f * axle_config.suspension_damping_ratio * PxSqrt(force.stiffness * sprung_mass);
                force.sprungMass = sprung_mass;
                axle_stiffness += force.stiffness * 0.5f;

                const float rest_load = gravity * (sprung_mass + axle_config.wheel_mass);

                PxVehicleTireForceParams& tire = base.tireForceParams[wheel_id];
                tire.latStiffX = 0.01f;
                tire.latStiffY = axle_config.tire_lateral_stiffness * rest_load;
                tire.longStiff = axle_config.tire_longitudinal_stiffness * rest_load;
                tire.camberStiff = 0.0f;
                tire.frictionVsSlip[0][0] = 0.0f;
                tire.frictionVsSlip[0][1] = axle_config.tire_grip;
                tire.frictionVsSlip[1][0] = 0.1f;
                tire.frictionVsSlip[1][1] = axle_config.tire_grip;
                tire.frictionVsSlip[2][0] = 1.0f;
                tire.frictionVsSlip[2][1] = axle_config.tire_grip;
                tire.restLoad = rest_load;
                tire.loadFilter[0][0] = 0.0f;
                tire.loadFilter[0][1] = 0.2308f;
                tire.loadFilter[1][0] = 3.0f;
                tire.loadFilter[1][1] = 3.0f;

                PxVehicleWheelParams& wheel = base.wheelParams[wheel_id];
                wheel.radius = axle_config.wheel_radius;
                wheel.halfWidth = axle_config.wheel_width * 0.5f;
                wheel.mass = axle_config.wheel_mass;
                wheel.moi = 0.5f * axle_config.wheel_mass * axle_config.wheel_radius * axle_config.wheel_radius;
                wheel.dampingRate = 0.25f;
            }

            if (axle_config.anti_roll_ratio > 0.0f)
            {
                PxVehicleAntiRollForceParams& anti_roll = anti_roll_params[nb_anti_roll++];
                anti_roll.wheel0 = get_wheel_id(axle, false);
                anti_roll.wheel1 = get_wheel_id(axle, true);
                anti_roll.stiffness = axle_config.anti_roll_ratio * axle_stiffness;
            }
        }

        {
            PxVehicleAckermannParams& ackermann = base.ackermannParams[0];
            ackermann.wheelIds[0] = get_wheel_id(0, false);
            ackermann.wheelIds[1] = get_wheel_id(0, true);
            ackermann.wheelBase = 1.0f;
            ackermann.trackWidth = 1.0f;
            ackermann.strength = 0.0f;

            float steered_z = 0.0f;
            float reference_z = 0.0f;
            uint32 steered_axle = 0;
            uint32 nb_reference_axles = 0;
            for (uint32 axle = 0; axle < nb_axles; ++axle)
            {
                if (config.axles[axle].steered)
                {
                    steered_axle = axle;
                    steered_z = config.axles[axle].position.z;
                }
                else
                {
                    reference_z += config.axles[axle].position.z;
                    ++nb_reference_axles;
                }
            }

            wheel_base = 0.0f;
            if (nb_reference_axles > 0)
            {
                wheel_base = PxAbs(steered_z - reference_z / nb_reference_axles);
            }

            if (nb_steered_axles == 1 && wheel_base > 0.0f)
            {
                ackermann.wheelIds[0] = get_wheel_id(steered_axle, false);
                ackermann.wheelIds[1] = get_wheel_id(steered_axle, true);
                ackermann.wheelBase = wheel_base;
                ackermann.trackWidth = config.axles[steered_axle].track_width;
                ackermann.strength = PxClamp(config.steering.ackermann_strength, 0.0f, 1.0f);
            }
            else
            {
                float min_z = config.axles.front().position.z;
                float max_z = min_z;
                for (const VehicleAxleConfig& axle : config.axles)
                {
                    min_z = std::min(min_z, axle.position.z);
                    max_z = std::max(max_z, axle.position.z);
                }
                wheel_base = max_z - min_z;
            }
        }

        collision_hull.clear();
        for (const vec3& point : chassis.collision_hull)
        {
            collision_hull.push_back(create_PxVec3(point));
        }

        {
            const PxQueryFilterData query_filter_data(PxFilterData(0, 0, 0, 0), PxQueryFlag::eSTATIC | PxQueryFlag::eDYNAMIC | PxQueryFlag::ePREFILTER);
            const PxTransform cmass_local_pose(center_of_mass);
            const PxVec3 box_half_extents = create_PxVec3(chassis.collision_half_extents);
            const PxTransform box_local_pose(create_PxVec3(chassis.collision_center));

            mPhysXParams.create(base.axleDescription,
                query_filter_data,
                &road_query_filter,
                nullptr,
                0,
                1.0f,
                cmass_local_pose,
                box_half_extents,
                box_local_pose);
        }

        {
            snippetvehicle::EngineDrivetrainParams& drivetrain = mEngineDriveParams;
            const VehicleEngineConfig& engine = config.engine;
            const float rpm_to_omega = PxTwoPi / 60.0f;

            drivetrain.engineParams.torqueCurve.clear();
            drivetrain.engineParams.torqueCurve.addPair(0.0f, engine.torque_curve.x);
            drivetrain.engineParams.torqueCurve.addPair(1.0f / 3.0f, engine.torque_curve.y);
            drivetrain.engineParams.torqueCurve.addPair(2.0f / 3.0f, engine.torque_curve.z);
            drivetrain.engineParams.torqueCurve.addPair(1.0f, engine.torque_curve.w);
            drivetrain.engineParams.moi = engine.moi;
            drivetrain.engineParams.peakTorque = engine.peak_torque;
            drivetrain.engineParams.idleOmega = engine.idle_rpm * rpm_to_omega;
            drivetrain.engineParams.maxOmega = engine.max_rpm * rpm_to_omega;
            drivetrain.engineParams.dampingRateFullThrottle = 0.15f;
            drivetrain.engineParams.dampingRateZeroThrottleClutchEngaged = 2.0f;
            drivetrain.engineParams.dampingRateZeroThrottleClutchDisengaged = 0.35f;

            PxVehicleGearboxParams& gearbox_params = drivetrain.gearBoxParams;
            gearbox_params.neutralGear = 1;
            gearbox_params.ratios[0] = -PxAbs(gearbox.reverse_ratio);
            gearbox_params.ratios[1] = 0.0f;
            for (size_t i = 0; i < gearbox.forward_ratios.size(); ++i)
            {
                gearbox_params.ratios[i + 2] = gearbox.forward_ratios[i];
            }
            gearbox_params.nbRatios = static_cast<PxU32>(gearbox.forward_ratios.size() + 2);
            gearbox_params.finalRatio = gearbox.final_ratio;
            gearbox_params.switchTime = gearbox.switch_time;

            PxVehicleAutoboxParams& autobox = drivetrain.autoboxParams;
            for (PxU32 i = 0; i < PxVehicleGearboxParams::eMAX_NB_GEARS; ++i)
            {
                autobox.upRatios[i] = gearbox.shift_up_rpm;
                autobox.downRatios[i] = gearbox.shift_down_rpm;
            }
            autobox.upRatios[1] = 0.15f;
            autobox.latency = gearbox.shift_latency;

            drivetrain.clutchCommandResponseParams.maxResponse = gearbox.clutch_strength;
            drivetrain.clutchParams.accuracyMode = PxVehicleClutchAccuracyMode::eBEST_POSSIBLE;
            drivetrain.clutchParams.estimateIterations = 5;

            drivetrain.multiWheelDifferentialParams.setToDefault();
            const float torque_ratio = 1.0f / (2.0f * nb_driven_axles);
            for (uint32 axle = 0; axle < nb_axles; ++axle)
            {
                const float ratio = config.axles[axle].driven ? torque_ratio : 0.0f;
                for (uint32 side = 0; side < 2; ++side)
                {
                    const uint32 wheel_id = get_wheel_id(axle, side == 1);
                    drivetrain.multiWheelDifferentialParams.torqueRatios[wheel_id] = ratio;
                    drivetrain.multiWheelDifferentialParams.aveWheelSpeedRatios[wheel_id] = ratio;
                }
            }

            PxVehicleFourWheelDriveDifferentialParams& four_wheel = drivetrain.fourWheelDifferentialParams;
            four_wheel.setToDefault();
            differential_type = eDIFFTYPE_MULTIWHEELDRIVE;
            if (nb_axles == 2)
            {
                const uint32 front = config.axles[0].position.z >= config.axles[1].position.z ? 0 : 1;
                const uint32 rear = 1 - front;
                const bool front_driven = config.axles[front].driven;
                const bool rear_driven = config.axles[rear].driven;

                for (uint32 i = 0; i < nb_wheels; ++i)
                {
                    four_wheel.torqueRatios[i] = drivetrain.multiWheelDifferentialParams.torqueRatios[i];
                    four_wheel.aveWheelSpeedRatios[i] = drivetrain.multiWheelDifferentialParams.aveWheelSpeedRatios[i];
                }
                four_wheel.frontWheelIds[0] = get_wheel_id(front, false);
                four_wheel.frontWheelIds[1] = get_wheel_id(front, true);
                four_wheel.rearWheelIds[0] = get_wheel_id(rear, false);
                four_wheel.rearWheelIds[1] = get_wheel_id(rear, true);
                four_wheel.frontBias = front_driven ? DIFF_BIAS : 0.0f;
                four_wheel.frontTarget = DIFF_TARGET;
                four_wheel.rearBias = rear_driven ? DIFF_BIAS : 0.0f;
                four_wheel.rearTarget = DIFF_TARGET;
                four_wheel.centerBias = (front_driven && rear_driven) ? DIFF_BIAS : 0.0f;
                four_wheel.centerTarget = DIFF_TARGET;
                four_wheel.rate = 10.0f;

                differential_type = eDIFFTYPE_FOURWHEELDRIVE;
            }

            drivetrain.tankDifferentialParams.setToDefault();
        }

        if (!base.isValid() || !mPhysXParams.isValid(base.axleDescription) || !mEngineDriveParams.isValid(base.axleDescription))
        {
            error = "Generated PhysX vehicle params are invalid.";
            return false;
        }

        return true;
    }

    bool VehicleSimulation::create(PxPhysics& physics, const PxCookingParams& cooking_params, PxMaterial& material, PxScene& scene)
    {
        if (!initialize(physics, cooking_params, material, differential_type))
        {
            return false;
        }

        anti_roll_torque.setToDefault();
        road_query_filter.self = mPhysXState.physxActor.rigidBody;

        if (collision_hull.size() >= 4)
        {
            replace_chassis_shape_with_hull(physics, cooking_params, material);
        }

        const PxU32 first_gear = mEngineDriveParams.gearBoxParams.neutralGear + 1;
        mEngineDriveState.gearboxState.currentGear = first_gear;
        mEngineDriveState.gearboxState.targetGear = first_gear;
        mTransmissionCommandState.targetGear = PxVehicleEngineDriveTransmissionCommandState::eAUTOMATIC_GEAR;

        context.setToDefault();
        context.frame = mBaseParams.frame;
        context.scale = mBaseParams.scale;
        context.gravity = scene.getGravity();
        context.physxScene = &scene;
        context.physxActorUpdateMode = PxVehiclePhysXActorUpdateMode::eAPPLY_ACCELERATION;

        return true;
    }

    PxShape* VehicleSimulation::create_convex_shape(PxPhysics& physics, const PxCookingParams& cooking_params, PxMaterial& material, const std::vector<PxVec3>& points)
    {
        if (points.size() < 4)
        {
            return nullptr;
        }

        PxConvexMeshDesc desc;
        desc.points.count = static_cast<PxU32>(points.size());
        desc.points.stride = sizeof(PxVec3);
        desc.points.data = points.data();
        desc.flags = PxConvexFlag::eCOMPUTE_CONVEX;

        PxDefaultMemoryOutputStream output;
        if (!PxCookConvexMesh(cooking_params, desc, output))
        {
            return nullptr;
        }

        PxDefaultMemoryInputData input(output.getData(), output.getSize());
        PxConvexMesh* convex_mesh = physics.createConvexMesh(input);
        if (convex_mesh == nullptr)
        {
            return nullptr;
        }

        PxShape* shape = physics.createShape(PxConvexMeshGeometry(convex_mesh, PxMeshScale(), PxConvexMeshGeometryFlag::eTIGHT_BOUNDS), material, true);
        convex_mesh->release();
        shape->setRestOffset(-0.005f);
        return shape;
    }

    void VehicleSimulation::replace_chassis_shape_with_hull(PxPhysics& physics, const PxCookingParams& cooking_params, PxMaterial& material)
    {
        PxShape* hull_shape = create_convex_shape(physics, cooking_params, material, collision_hull);
        if (hull_shape == nullptr)
        {
            LOG_WARNING("Vehicle '%s': failed to cook the collision hull, the box is used.", name.c_str());
            return;
        }

        PxRigidBody* body = get_rigid_body();
        const PxVehiclePhysXActor& actor = mPhysXState.physxActor;

        PxShape* shapes[PxVehicleLimits::eMAX_NB_WHEELS + 1];
        const PxU32 nb_shapes = body->getShapes(shapes, PxVehicleLimits::eMAX_NB_WHEELS + 1);
        for (PxU32 i = 0; i < nb_shapes; ++i)
        {
            if (std::find(std::begin(actor.wheelShapes), std::end(actor.wheelShapes), shapes[i]) == std::end(actor.wheelShapes))
            {
                body->detachShape(*shapes[i]);
            }
        }

        body->attachShape(*hull_shape);
        hull_shape->release();
    }

    void VehicleSimulation::apply_commands(const VehicleCommands& commands)
    {
        mCommandState.brakes[0] = commands.brake;
        mCommandState.brakes[1] = commands.handbrake;
        mCommandState.nbBrakes = 2;
        mCommandState.throttle = commands.throttle;
        mCommandState.steer = commands.steer;

        mTransmissionCommandState.clutch = commands.clutch;
        mTransmissionCommandState.targetGear = commands.reverse
                                                   ? 0
                                                   : PxVehicleEngineDriveTransmissionCommandState::eAUTOMATIC_GEAR;
    }

    void VehicleSimulation::simulate(float dt)
    {
        const PxU8 nb_substeps = PxAbs(get_forward_speed()) < LOW_SPEED_SUBSTEPS_THRESHOLD ? 3 : 2;
        mComponentSequence.setSubsteps(mComponentSequenceSubstepGroupHandle, nb_substeps);
        step(dt, context);
    }

    void VehicleSimulation::apply_stability_assist(float strength)
    {
        PxRigidDynamic* body = get_rigid_body()->is<PxRigidDynamic>();
        if (strength <= 0.0f || wheel_base <= 0.0f || body->isSleeping())
        {
            return;
        }

        const float speed = get_forward_speed();
        if (PxAbs(speed) < STABILITY_MIN_SPEED || get_nb_grounded_wheels() * 2 < get_nb_wheels())
        {
            return;
        }

        const float max_yaw_rate = max_grip * context.gravity.magnitude() / PxAbs(speed);
        const float target_yaw_rate = PxClamp(speed * PxTan(get_steer_angle()) / wheel_base, -max_yaw_rate, max_yaw_rate);

        const PxVec3 up = body->getGlobalPose().q.rotate(context.frame.getVrtAxis());
        const float yaw_rate = body->getAngularVelocity().dot(up);

        const float lower = PxMin(0.0f, target_yaw_rate) - STABILITY_YAW_MARGIN;
        const float upper = PxMax(0.0f, target_yaw_rate) + STABILITY_YAW_MARGIN;
        const float excess = yaw_rate - PxClamp(yaw_rate, lower, upper);

        if (excess != 0.0f)
        {
            body->addTorque(up * (-excess * strength * STABILITY_GAIN), PxForceMode::eACCELERATION);
        }
    }

    float VehicleSimulation::get_forward_speed() const
    {
        const PxRigidBody* body = get_rigid_body();
        return body->getLinearVelocity().dot(body->getGlobalPose().q.rotate(context.frame.getLngAxis()));
    }

    float VehicleSimulation::get_engine_rpm() const
    {
        return mEngineDriveState.engineState.rotationSpeed * 60.0f / PxTwoPi;
    }

    float VehicleSimulation::get_steer_angle() const
    {
        float angle = 0.0f;
        uint32 nb_steered = 0;
        for (uint32 i = 0; i < get_nb_wheels(); ++i)
        {
            if (mBaseParams.steerResponseParams.wheelResponseMultipliers[i] != 0.0f)
            {
                angle += mBaseState.steerCommandResponseStates[i];
                ++nb_steered;
            }
        }
        return nb_steered > 0 ? angle / nb_steered : 0.0f;
    }

    int32 VehicleSimulation::get_current_gear() const
    {
        const int32 gear = static_cast<int32>(mEngineDriveState.gearboxState.currentGear);
        const int32 neutral = static_cast<int32>(mEngineDriveParams.gearBoxParams.neutralGear);
        return gear < neutral ? -1 : gear - neutral;
    }

    uint32 VehicleSimulation::get_nb_wheels() const
    {
        return mBaseParams.axleDescription.nbWheels;
    }

    bool VehicleSimulation::is_wheel_grounded(uint32 wheel_id) const
    {
        return mBaseState.roadGeomStates[wheel_id].hitState && mBaseState.suspensionStates[wheel_id].separation <= 0.0f;
    }

    uint32 VehicleSimulation::get_nb_grounded_wheels() const
    {
        uint32 result = 0;
        for (uint32 i = 0; i < get_nb_wheels(); ++i)
        {
            result += is_wheel_grounded(i) ? 1 : 0;
        }
        return result;
    }

    PxTransform VehicleSimulation::get_wheel_local_pose(uint32 wheel_id, bool should_spin) const
    {
        PxTransform pose = mBaseState.wheelLocalPoses[wheel_id].localPose;
        if (!should_spin)
        {
            const PxQuat rotation = PxVehicleComputeRotation(context.frame, 0.0f, mBaseState.wheelRigidBody1dStates[wheel_id].rotationAngle, 0.0f);
            pose.q = pose.q * rotation.getConjugate();
        }
        return mPhysXParams.physxActorCMassLocalPose * pose;
    }

    PxRigidBody* VehicleSimulation::get_rigid_body() const
    {
        return mPhysXState.physxActor.rigidBody;
    }

    PxShape* VehicleSimulation::get_wheel_shape(uint32 wheel_id) const
    {
        return mPhysXState.physxActor.wheelShapes[wheel_id];
    }

    void VehicleSimulation::getDataForSuspensionComponent(const PxVehicleAxleDescription*& axleDescription,
        const PxVehicleRigidBodyParams*& rigidBodyParams,
        const PxVehicleSuspensionStateCalculationParams*& suspensionStateCalculationParams,
        PxVehicleArrayData<const PxReal>& steerResponseStates,
        const PxVehicleRigidBodyState*& rigidBodyState,
        PxVehicleArrayData<const PxVehicleWheelParams>& wheelParams,
        PxVehicleArrayData<const PxVehicleSuspensionParams>& suspensionParams,
        PxVehicleArrayData<const PxVehicleSuspensionComplianceParams>& suspensionComplianceParams,
        PxVehicleArrayData<const PxVehicleSuspensionForceParams>& suspensionForceParams,
        PxVehicleSizedArrayData<const PxVehicleAntiRollForceParams>& antiRollForceParams,
        PxVehicleArrayData<const PxVehicleRoadGeometryState>& wheelRoadGeomStates,
        PxVehicleArrayData<PxVehicleSuspensionState>& suspensionStates,
        PxVehicleArrayData<PxVehicleSuspensionComplianceState>& suspensionComplianceStates,
        PxVehicleArrayData<PxVehicleSuspensionForce>& suspensionForces,
        PxVehicleAntiRollTorque*& antiRollTorque)
    {
        EngineDriveVehicle::getDataForSuspensionComponent(axleDescription,
            rigidBodyParams,
            suspensionStateCalculationParams,
            steerResponseStates,
            rigidBodyState,
            wheelParams,
            suspensionParams,
            suspensionComplianceParams,
            suspensionForceParams,
            antiRollForceParams,
            wheelRoadGeomStates,
            suspensionStates,
            suspensionComplianceStates,
            suspensionForces,
            antiRollTorque);

        if (nb_anti_roll > 0)
        {
            antiRollForceParams.setDataAndCount(anti_roll_params, nb_anti_roll);
            antiRollTorque = &anti_roll_torque;
        }
    }

    void VehicleSimulation::getDataForRigidBodyComponent(const PxVehicleAxleDescription*& axleDescription,
        const PxVehicleRigidBodyParams*& rigidBodyParams,
        PxVehicleArrayData<const PxVehicleSuspensionForce>& suspensionForces,
        PxVehicleArrayData<const PxVehicleTireForce>& tireForces,
        const PxVehicleAntiRollTorque*& antiRollTorque,
        PxVehicleRigidBodyState*& rigidBodyState)
    {
        EngineDriveVehicle::getDataForRigidBodyComponent(axleDescription,
            rigidBodyParams,
            suspensionForces,
            tireForces,
            antiRollTorque,
            rigidBodyState);

        antiRollTorque = nb_anti_roll > 0 ? &anti_roll_torque : nullptr;
    }
}
