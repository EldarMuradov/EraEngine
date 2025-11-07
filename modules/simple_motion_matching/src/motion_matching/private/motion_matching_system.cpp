#include "motion_matching/motion_matching_system.h"
#include "animation/animation.h"
#include "animation/skinning.h"

#include "rendering/ecs/renderer_holder_root_component.h"
#include "rendering/debug_visualization.h"

#include "core/cpu_profiling.h"
#include "core/memory.h"
#include "core/string.h"
#include "core/ecs/input_reciever_component.h"
#include "core/debug/debug_var.h"

#include "engine/engine.h"

#include "ecs/update_groups.h"
#include "ecs/rendering/mesh_component.h"
#include "ecs/base_components/transform_component.h"

#include "motion_matching/database.h"

#include "motion_matching/common.h"
#include "motion_matching/vec.h"
#include "motion_matching/quat.h"
#include "motion_matching/spring.h"
#include "motion_matching/array.h"
#include "motion_matching/controller.h"

#include <rttr/policy.h>
#include <rttr/registration>

namespace era_engine
{
    static DebugVar<bool> ik_enabled = DebugVar<bool>("motion_matching.ik_enabled", true);
    static DebugVar<bool> adjustment_enabled = DebugVar<bool>("motion_matching.adjustment_enabled", true);
    static DebugVar<bool> clamping_enabled = DebugVar<bool>("motion_matching.clamping_enabled", true);

    static DebugVar<bool> synchronization_enabled = DebugVar<bool>("motion_matching.synchronization_enabled", false);

    static void desired_gait_update(
        float& desired_gait,
        float& desired_gait_velocity,
        const float dt,
        const float gait_change_halflife = 0.1f)
    {
        simple_spring_damper_exact(
            desired_gait,
            desired_gait_velocity,
            0.0f, // TODO
            gait_change_halflife,
            dt);
    }

    static Vec3 desired_velocity_update(
        const Vec3& gamepadstick_left,
        const Quat& simulation_rotation,
        const float fwrd_speed,
        const float side_speed,
        const float back_speed)
    {
        // Scale stick by forward, sideways and backwards speeds
        Vec3 local_desired_velocity = gamepadstick_left.z > 0.0 ?
            Vec3(side_speed, 0.0f, fwrd_speed) * gamepadstick_left :
            Vec3(side_speed, 0.0f, back_speed) * gamepadstick_left;

        return local_desired_velocity;
    }

    static Quat desired_rotation_update(
        const Quat& desired_rotation,
        const Vec3& gamepadstick_left,
        const Vec3& gamepadstick_right,
        const float strafe_direction,
        const bool desired_strafe,
        const Vec3& desired_velocity)
    {
        Quat desired_rotation_curr = desired_rotation;

        // If strafe is active then desired direction is coming from right
        // stick as long as that stick is being used, otherwise we assume
        // forward facing
        if (desired_strafe)
        {
            Vec3 desired_direction = quat_mul_vec3(quat_from_angle_axis(strafe_direction, Vec3(0, 1, 0)), Vec3(0, 0, -1));

            if (length(gamepadstick_right) > 0.01f)
            {
                desired_direction = quat_mul_vec3(quat_from_angle_axis(strafe_direction, Vec3(0, 1, 0)), normalize(gamepadstick_right));
            }

            return quat_from_angle_axis(atan2f(desired_direction.x, desired_direction.z), Vec3(0, 1, 0));
        }

        // If strafe is not active the desired direction comes from the left 
        // stick as long as that stick is being used
        else if (length(gamepadstick_left) > 0.01f)
        {

            Vec3 desired_direction = normalize(desired_velocity);
            return quat_from_angle_axis(atan2f(desired_direction.x, desired_direction.z), Vec3(0, 1, 0));
        }

        // Otherwise desired direction remains the same
        else
        {
            return desired_rotation_curr;
        }
    }

    //--------------------------------------

    // Moving the root is a little bit difficult when we have the
    // inertializer set up in the way we do. Essentially we need
    // to also make sure to adjust all of the locations where 
    // we are transforming the data to and from as well as the 
    // offsets being blended out
    static void inertialize_root_adjust(
        Vec3& offset_position,
        Vec3& transition_src_position,
        Quat& transition_src_rotation,
        Vec3& transition_dst_position,
        Quat& transition_dst_rotation,
        Vec3& position,
        Quat& rotation,
        const Vec3& input_position,
        const Quat& input_rotation)
    {
        // Find the position difference and add it to the state and transition location
        Vec3 position_difference = input_position - position;
        position = position_difference + position;
        transition_dst_position = position_difference + transition_dst_position;

        // Find the point at which we want to now transition from in the src data
        transition_src_position = transition_src_position + quat_mul_vec3(transition_src_rotation,
            quat_inv_mul_vec3(transition_dst_rotation, position - offset_position - transition_dst_position));
        transition_dst_position = position;
        offset_position = Vec3();

        // Find the rotation difference. We need to normalize here or some error can accumulate 
        // over time during adjustment.
        Quat rotation_difference = quat_normalize(quat_mul_inv(input_rotation, rotation));

        // Apply the rotation difference to the current rotation and transition location
        rotation = quat_mul(rotation_difference, rotation);
        transition_dst_rotation = quat_mul(rotation_difference, transition_dst_rotation);
    }

    static void inertialize_pose_reset(
        slice1d<Vec3> bone_offset_positions,
        slice1d<Vec3> bone_offset_velocities,
        slice1d<Quat> bone_offset_rotations,
        slice1d<Vec3> bone_offset_angular_velocities,
        Vec3& transition_src_position,
        Quat& transition_src_rotation,
        Vec3& transition_dst_position,
        Quat& transition_dst_rotation,
        const Vec3& root_position,
        const Quat& root_rotation)
    {
        bone_offset_positions.zero();
        bone_offset_velocities.zero();
        bone_offset_rotations.set(Quat());
        bone_offset_angular_velocities.zero();

        transition_src_position = root_position;
        transition_src_rotation = root_rotation;
        transition_dst_position = Vec3();
        transition_dst_rotation = Quat();
    }

    // This function transitions the inertializer for 
    // the full character. It takes as input the current 
    // offsets, as well as the root transition locations,
    // current root state, and the full pose information 
    // for the pose being transitioned from (src) as well 
    // as the pose being transitioned to (dst) in their
    // own animation spaces.
    static void inertialize_pose_transition(
        slice1d<Vec3> bone_offset_positions,
        slice1d<Vec3> bone_offset_velocities,
        slice1d<Quat> bone_offset_rotations,
        slice1d<Vec3> bone_offset_angular_velocities,
        Vec3& transition_src_position,
        Quat& transition_src_rotation,
        Vec3& transition_dst_position,
        Quat& transition_dst_rotation,
        const Vec3& root_position,
        const Vec3& root_velocity,
        const Quat& root_rotation,
        const Vec3& root_angular_velocity,
        const slice1d<Vec3>& bone_src_positions,
        const slice1d<Vec3>& bone_src_velocities,
        const slice1d<Quat>& bone_src_rotations,
        const slice1d<Vec3>& bone_src_angular_velocities,
        const slice1d<Vec3>& bone_dst_positions,
        const slice1d<Vec3>& bone_dst_velocities,
        const slice1d<Quat>& bone_dst_rotations,
        const slice1d<Vec3>& bone_dst_angular_velocities)
    {
        // First we record the root position and rotation
        // in the animation data for the source and destination
        // animation
        transition_dst_position = root_position;
        transition_dst_rotation = root_rotation;
        transition_src_position = bone_dst_positions(0);
        transition_src_rotation = bone_dst_rotations(0);

        // We then find the velocities so we can transition the 
        // root inertiaizers
        Vec3 world_space_dst_velocity = quat_mul_vec3(transition_dst_rotation,
            quat_inv_mul_vec3(transition_src_rotation, bone_dst_velocities(0)));

        Vec3 world_space_dst_angular_velocity = quat_mul_vec3(transition_dst_rotation,
            quat_inv_mul_vec3(transition_src_rotation, bone_dst_angular_velocities(0)));

        // Transition inertializers recording the offsets for 
        // the root joint
        inertialize_transition(
            bone_offset_positions(0),
            bone_offset_velocities(0),
            root_position,
            root_velocity,
            root_position,
            world_space_dst_velocity);

        inertialize_transition(
            bone_offset_rotations(0),
            bone_offset_angular_velocities(0),
            root_rotation,
            root_angular_velocity,
            root_rotation,
            world_space_dst_angular_velocity);

        // Transition all the inertializers for each other bone
        for (int i = 1; i < bone_offset_positions.size; i++)
        {
            inertialize_transition(
                bone_offset_positions(i),
                bone_offset_velocities(i),
                bone_src_positions(i),
                bone_src_velocities(i),
                bone_dst_positions(i),
                bone_dst_velocities(i));

            inertialize_transition(
                bone_offset_rotations(i),
                bone_offset_angular_velocities(i),
                bone_src_rotations(i),
                bone_src_angular_velocities(i),
                bone_dst_rotations(i),
                bone_dst_angular_velocities(i));
        }
    }

    // This function updates the inertializer states. Here 
    // it outputs the smoothed animation (input plus offset) 
    // as well as updating the offsets themselves. It takes 
    // as input the current playing animation as well as the 
    // root transition locations, a halflife, and a dt
    static void inertialize_pose_update(
        slice1d<Vec3> bone_positions,
        slice1d<Vec3> bone_velocities,
        slice1d<Quat> bone_rotations,
        slice1d<Vec3> bone_angular_velocities,
        slice1d<Vec3> bone_offset_positions,
        slice1d<Vec3> bone_offset_velocities,
        slice1d<Quat> bone_offset_rotations,
        slice1d<Vec3> bone_offset_angular_velocities,
        const slice1d<Vec3>& bone_input_positions,
        const slice1d<Vec3>& bone_input_velocities,
        const slice1d<Quat>& bone_input_rotations,
        const slice1d<Vec3>& bone_input_angular_velocities,
        const Vec3& transition_src_position,
        const Quat& transition_src_rotation,
        const Vec3& transition_dst_position,
        const Quat& transition_dst_rotation,
        const float halflife,
        const float dt)
    {
        // First we find the next root position, velocity, rotation
        // and rotational velocity in the world space by transforming 
        // the input animation from it's animation space into the 
        // space of the currently playing animation.
        Vec3 world_space_position = quat_mul_vec3(transition_dst_rotation,
            quat_inv_mul_vec3(transition_src_rotation,
                bone_input_positions(0) - transition_src_position)) + transition_dst_position;

        Vec3 world_space_velocity = quat_mul_vec3(transition_dst_rotation,
            quat_inv_mul_vec3(transition_src_rotation, bone_input_velocities(0)));

        // Normalize here because Quat inv mul can sometimes produce 
        // unstable returns when the two rotations are very close.
        Quat world_space_rotation = quat_normalize(quat_mul(transition_dst_rotation,
            quat_inv_mul(transition_src_rotation, bone_input_rotations(0))));

        Vec3 world_space_angular_velocity = quat_mul_vec3(transition_dst_rotation,
            quat_inv_mul_vec3(transition_src_rotation, bone_input_angular_velocities(0)));

        // Then we update these two inertializers with these new world space inputs
        inertialize_update(
            bone_positions(0),
            bone_velocities(0),
            bone_offset_positions(0),
            bone_offset_velocities(0),
            world_space_position,
            world_space_velocity,
            halflife,
            dt);

        inertialize_update(
            bone_rotations(0),
            bone_angular_velocities(0),
            bone_offset_rotations(0),
            bone_offset_angular_velocities(0),
            world_space_rotation,
            world_space_angular_velocity,
            halflife,
            dt);

        // Then we update the inertializers for the rest of the bones
        for (int i = 1; i < bone_positions.size; i++)
        {
            inertialize_update(
                bone_positions(i),
                bone_velocities(i),
                bone_offset_positions(i),
                bone_offset_velocities(i),
                bone_input_positions(i),
                bone_input_velocities(i),
                halflife,
                dt);

            inertialize_update(
                bone_rotations(i),
                bone_angular_velocities(i),
                bone_offset_rotations(i),
                bone_offset_angular_velocities(i),
                bone_input_rotations(i),
                bone_input_angular_velocities(i),
                halflife,
                dt);
        }
    }

    //--------------------------------------

    // Copy a part of a feature vector from the 
    // matching database into the query feature vector
    static void query_copy_denormalized_feature(
        slice1d<float> query,
        int& offset,
        const int size,
        const slice1d<float>& features,
        const slice1d<float>& features_offset,
        const slice1d<float>& features_scale)
    {
        for (int i = 0; i < size; i++)
        {
            query(offset + i) = features(offset + i) * features_scale(offset + i) + features_offset(offset + i);
        }

        offset += size;
    }

    // Compute the query feature vector for the current 
    // trajectory controlled by the gamepad.
    static void query_compute_trajectory_position_feature(
        slice1d<float> query,
        int& offset,
        const Vec3& root_position,
        const Quat& root_rotation,
        const slice1d<Vec3>& trajectory_positions)
    {
        Vec3 traj0 = quat_inv_mul_vec3(root_rotation, trajectory_positions(1) - root_position);
        Vec3 traj1 = quat_inv_mul_vec3(root_rotation, trajectory_positions(2) - root_position);
        Vec3 traj2 = quat_inv_mul_vec3(root_rotation, trajectory_positions(3) - root_position);

        query(offset + 0) = traj0.x;
        query(offset + 1) = traj0.z;
        query(offset + 2) = traj1.x;
        query(offset + 3) = traj1.z;
        query(offset + 4) = traj2.x;
        query(offset + 5) = traj2.z;

        offset += 6;
    }

    // Same but for the trajectory direction
    static void query_compute_trajectory_direction_feature(
        slice1d<float> query,
        int& offset,
        const Quat& root_rotation,
        const slice1d<Quat>& trajectory_rotations)
    {
        Vec3 traj0 = quat_inv_mul_vec3(root_rotation, quat_mul_vec3(trajectory_rotations(1), Vec3(0, 0, 1)));
        Vec3 traj1 = quat_inv_mul_vec3(root_rotation, quat_mul_vec3(trajectory_rotations(2), Vec3(0, 0, 1)));
        Vec3 traj2 = quat_inv_mul_vec3(root_rotation, quat_mul_vec3(trajectory_rotations(3), Vec3(0, 0, 1)));

        query(offset + 0) = traj0.x;
        query(offset + 1) = traj0.z;
        query(offset + 2) = traj1.x;
        query(offset + 3) = traj1.z;
        query(offset + 4) = traj2.x;
        query(offset + 5) = traj2.z;

        offset += 6;
    }

    //--------------------------------------

    // Collide against the obscales which are
    // essentially bounding boxes of a given size
    static Vec3 simulation_collide_obstacles(
        const Vec3& prev_pos,
        const Vec3& next_pos,
        const slice1d<Vec3>& obstacles_positions,
        const slice1d<Vec3>& obstacles_scales,
        const float radius = 0.6f)
    {
        Vec3 dx = next_pos - prev_pos;
        Vec3 proj_pos = prev_pos;

        // Substep because I'm too lazy to implement CCD
        int substeps = 1 + (int)(length(dx) * 5.0f);

        for (int j = 0; j < substeps; j++)
        {
            proj_pos = proj_pos + dx / substeps;

            for (int i = 0; i < obstacles_positions.size; i++)
            {
                // Find nearest point inside obscale and push out
                Vec3 nearest = clamp_Vec3(proj_pos,
                    obstacles_positions(i) - 0.5f * obstacles_scales(i),
                    obstacles_positions(i) + 0.5f * obstacles_scales(i));

                if (length(nearest - proj_pos) < radius)
                {
                    proj_pos = radius * normalize(proj_pos - nearest) + nearest;
                }
            }
        }

        return proj_pos;
    }

    // Taken from https://theorangeduck.com/page/spring-roll-call#controllers
    static void simulation_positions_update(
        Vec3& position,
        Vec3& velocity,
        Vec3& acceleration,
        const Vec3& desired_velocity,
        const float halflife,
        const float dt,
        const slice1d<Vec3>& obstacles_positions,
        const slice1d<Vec3>& obstacles_scales)
    {
        float y = halflife_to_damping(halflife) / 2.0f;
        Vec3 j0 = velocity - desired_velocity;
        Vec3 j1 = acceleration + j0 * y;
        float eydt = fast_negexpf(y * dt);

        Vec3 position_prev = position;

        position = eydt * (((-j1) / (y * y)) + ((-j0 - j1 * dt) / y)) +
            (j1 / (y * y)) + j0 / y + desired_velocity * dt + position_prev;
        velocity = eydt * (j0 + j1 * dt) + desired_velocity;
        acceleration = eydt * (acceleration - j1 * y * dt);

        position = simulation_collide_obstacles(
            position_prev,
            position,
            obstacles_positions,
            obstacles_scales);
    }

    static void simulation_rotations_update(
        Quat& rotation,
        Vec3& angular_velocity,
        const Quat& desired_rotation,
        const float halflife,
        const float dt)
    {
        simple_spring_damper_exact(
            rotation,
            angular_velocity,
            desired_rotation,
            halflife, dt);
    }

    // Predict what the desired velocity will be in the 
    // future. Here we need to use the future trajectory 
    // rotation as well as predicted future camera 
    // position to find an accurate desired velocity in 
    // the world space
    static void trajectory_desired_velocities_predict(
        slice1d<Vec3> desired_velocities,
        const slice1d<Quat>& trajectory_rotations,
        const Vec3& desired_velocity,
        const Vec3& gamepadstick_left,
        const Vec3& gamepadstick_right,
        const bool desired_strafe,
        const float fwrd_speed,
        const float side_speed,
        const float back_speed,
        const float dt)
    {
        desired_velocities(0) = desired_velocity;

        for (int i = 1; i < desired_velocities.size; i++)
        {
            desired_velocities(i) = desired_velocity_update(
                gamepadstick_left,
                trajectory_rotations(i),
                fwrd_speed,
                side_speed,
                back_speed);
        }
    }

    static void trajectory_positions_predict(
        slice1d<Vec3> positions,
        slice1d<Vec3> velocities,
        slice1d<Vec3> accelerations,
        const Vec3& position,
        const Vec3& velocity,
        const Vec3& acceleration,
        const slice1d<Vec3>& desired_velocities,
        const float halflife,
        const float dt,
        const slice1d<Vec3>& obstacles_positions,
        const slice1d<Vec3>& obstacles_scales)
    {
        positions(0) = position;
        velocities(0) = velocity;
        accelerations(0) = acceleration;

        for (int i = 1; i < positions.size; i++)
        {
            positions(i) = positions(i - 1);
            velocities(i) = velocities(i - 1);
            accelerations(i) = accelerations(i - 1);

            simulation_positions_update(
                positions(i),
                velocities(i),
                accelerations(i),
                desired_velocities(i),
                halflife,
                dt,
                obstacles_positions,
                obstacles_scales);
        }
    }

    // Predict desired rotations given the estimated future 
    // camera rotation and other parameters
    static void trajectory_desired_rotations_predict(
        slice1d<Quat> desired_rotations,
        const slice1d<Vec3> desired_velocities,
        const Quat& desired_rotation,
        const Vec3& gamepadstick_left,
        const Vec3& gamepadstick_right,
        const bool desired_strafe,
        const float dt)
    {
        desired_rotations(0) = desired_rotation;

        for (int i = 1; i < desired_rotations.size; i++)
        {
            desired_rotations(i) = desired_rotation_update(
                desired_rotations(i - 1),
                gamepadstick_left,
                gamepadstick_right,
                0.0f,
                desired_strafe,
                desired_velocities(i));
        }
    }

    static void trajectory_rotations_predict(
        slice1d<Quat> rotations,
        slice1d<Vec3> angular_velocities,
        const Quat& rotation,
        const Vec3& angular_velocity,
        const slice1d<Quat>& desired_rotations,
        const float halflife,
        const float dt)
    {
        rotations.set(rotation);
        angular_velocities.set(angular_velocity);

        for (int i = 1; i < rotations.size; i++)
        {
            simulation_rotations_update(
                rotations(i),
                angular_velocities(i),
                desired_rotations(i),
                halflife,
                i * dt);
        }
    }

    static void contact_reset(
        bool& contact_state,
        bool& contact_lock,
        Vec3& contact_position,
        Vec3& contact_velocity,
        Vec3& contact_point,
        Vec3& contact_target,
        Vec3& contact_offset_position,
        Vec3& contact_offset_velocity,
        const Vec3& input_contact_position,
        const Vec3& input_contact_velocity,
        const bool input_contact_state)
    {
        contact_state = false;
        contact_lock = false;
        contact_position = input_contact_position;
        contact_velocity = input_contact_velocity;
        contact_point = input_contact_position;
        contact_target = input_contact_position;
        contact_offset_position = Vec3();
        contact_offset_velocity = Vec3();
    }

    static void contact_update(
        bool& contact_state,
        bool& contact_lock,
        Vec3& contact_position,
        Vec3& contact_velocity,
        Vec3& contact_point,
        Vec3& contact_target,
        Vec3& contact_offset_position,
        Vec3& contact_offset_velocity,
        const Vec3& input_contact_position,
        const bool input_contact_state,
        const float unlock_radius,
        const float foot_height,
        const float halflife,
        const float dt,
        const float eps = 1e-8)
    {
        // First compute the input contact position velocity via finite difference
        Vec3 input_contact_velocity =
            (input_contact_position - contact_target) / (dt + eps);
        contact_target = input_contact_position;

        // Update the inertializer to tick forward in time
        inertialize_update(
            contact_position,
            contact_velocity,
            contact_offset_position,
            contact_offset_velocity,
            // If locked we feed the contact point and zero velocity, 
            // otherwise we feed the input from the animation
            contact_lock ? contact_point : input_contact_position,
            contact_lock ? Vec3() : input_contact_velocity,
            halflife,
            dt);

        // If the contact point is too far from the current input position 
        // then we need to unlock the contact
        bool unlock_contact = contact_lock && (
            length(contact_point - input_contact_position) > unlock_radius);

        // If the contact was previously inactive but is now active we 
        // need to transition to the locked contact state
        if (!contact_state && input_contact_state)
        {
            // Contact point is given by the current position of 
            // the foot projected onto the ground plus foot height
            contact_lock = true;
            contact_point = contact_position;
            contact_point.y = foot_height;

            inertialize_transition(
                contact_offset_position,
                contact_offset_velocity,
                input_contact_position,
                input_contact_velocity,
                contact_point,
                Vec3());
        }

        // Otherwise if we need to unlock or we were previously in 
        // contact but are no longer we transition to just taking 
        // the input position as-is
        else if ((contact_lock && contact_state && !input_contact_state)
            || unlock_contact)
        {
            contact_lock = false;

            inertialize_transition(
                contact_offset_position,
                contact_offset_velocity,
                contact_point,
                Vec3(),
                input_contact_position,
                input_contact_velocity);
        }

        // Update contact state
        contact_state = input_contact_state;
    }

    //--------------------------------------

    // Rotate a joint to look toward some 
    // given target position
    static void ik_look_at(
        Quat& bone_rotation,
        const Quat& global_parent_rotation,
        const Quat& global_rotation,
        const Vec3& global_position,
        const Vec3& child_position,
        const Vec3& target_position,
        const float eps = 1e-5f)
    {
        Vec3 curr_dir = normalize(child_position - global_position);
        Vec3 targ_dir = normalize(target_position - global_position);

        if (fabs(1.0f - dot(curr_dir, targ_dir) > eps))
        {
            bone_rotation = quat_inv_mul(global_parent_rotation,
                quat_mul(quat_between(curr_dir, targ_dir), global_rotation));
        }
    }

    // Basic two-joint IK in the style of https://theorangeduck.com/page/simple-two-joint
    // Here I add a basic "forward vector" which acts like a kind of pole-vetor
    // to control the bending direction
    static void ik_two_bone(
        Quat& bone_root_lr,
        Quat& bone_mid_lr,
        const Vec3& bone_root,
        const Vec3& bone_mid,
        const Vec3& bone_end,
        const Vec3& target,
        const Vec3& fwd,
        const Quat& bone_root_gr,
        const Quat& bone_mid_gr,
        const Quat& bone_par_gr,
        const float max_length_buffer)
    {

        float max_extension =
            length(bone_root - bone_mid) +
            length(bone_mid - bone_end) -
            max_length_buffer;

        Vec3 target_clamp = target;
        if (length(target - bone_root) > max_extension)
        {
            target_clamp = bone_root + max_extension * normalize(target - bone_root);
        }

        Vec3 axis_dwn = normalize(bone_end - bone_root);
        Vec3 axis_rot = normalize(cross(axis_dwn, fwd));

        Vec3 a = bone_root;
        Vec3 b = bone_mid;
        Vec3 c = bone_end;
        Vec3 t = target_clamp;

        float lab = length(b - a);
        float lcb = length(b - c);
        float lat = length(t - a);

        float ac_ab_0 = acosf(clampf(dot(normalize(c - a), normalize(b - a)), -1.0f, 1.0f));
        float ba_bc_0 = acosf(clampf(dot(normalize(a - b), normalize(c - b)), -1.0f, 1.0f));

        float ac_ab_1 = acosf(clampf((lab * lab + lat * lat - lcb * lcb) / (2.0f * lab * lat), -1.0f, 1.0f));
        float ba_bc_1 = acosf(clampf((lab * lab + lcb * lcb - lat * lat) / (2.0f * lab * lcb), -1.0f, 1.0f));

        Quat r0 = quat_from_angle_axis(ac_ab_1 - ac_ab_0, axis_rot);
        Quat r1 = quat_from_angle_axis(ba_bc_1 - ba_bc_0, axis_rot);

        Vec3 c_a = normalize(bone_end - bone_root);
        Vec3 t_a = normalize(target_clamp - bone_root);

        Quat r2 = quat_from_angle_axis(
            acosf(clampf(dot(c_a, t_a), -1.0f, 1.0f)),
            normalize(cross(c_a, t_a)));

        bone_root_lr = quat_inv_mul(bone_par_gr, quat_mul(r2, quat_mul(r0, bone_root_gr)));
        bone_mid_lr = quat_inv_mul(bone_root_gr, quat_mul(r1, bone_mid_gr));
    }

    static Vec3 adjust_character_position(
        const Vec3& character_position,
        const Vec3& simulation_position,
        const float halflife,
        const float dt)
    {
        // Find the difference in positioning
        Vec3 difference_position = simulation_position - character_position;

        // Damp that difference using the given halflife and dt
        Vec3 adjustment_position = damp_adjustment_exact(
            difference_position,
            halflife,
            dt);

        // Add the damped difference to move the character toward the sim
        return adjustment_position + character_position;
    }

    static Quat adjust_character_rotation(
        const Quat& character_rotation,
        const Quat& simulation_rotation,
        const float halflife,
        const float dt)
    {
        // Find the difference in rotation (from character to simulation).
        // Here `quat_abs` forces the quaternion to take the shortest 
        // path and normalization is required as sometimes taking 
        // the difference between two very similar rotations can 
        // introduce numerical instability
        Quat difference_rotation = quat_abs(quat_normalize(
            quat_mul_inv(simulation_rotation, character_rotation)));

        // Damp that difference using the given halflife and dt
        Quat adjustment_rotation = damp_adjustment_exact(
            difference_rotation,
            halflife,
            dt);

        // Apply the damped adjustment to the character
        return quat_mul(adjustment_rotation, character_rotation);
    }

    static Vec3 adjust_character_position_by_velocity(
        const Vec3& character_position,
        const Vec3& character_velocity,
        const Vec3& simulation_position,
        const float max_adjustment_ratio,
        const float halflife,
        const float dt)
    {
        // Find and damp the desired adjustment
        Vec3 adjustment_position = damp_adjustment_exact(
            simulation_position - character_position,
            halflife,
            dt);

        // If the length of the adjustment is greater than the character velocity 
        // multiplied by the ratio then we need to clamp it to that length
        float max_length = max_adjustment_ratio * length(character_velocity) * dt;

        if (length(adjustment_position) > max_length)
        {
            adjustment_position = max_length * normalize(adjustment_position);
        }

        // Apply the adjustment
        return adjustment_position + character_position;
    }

    static Quat adjust_character_rotation_by_velocity(
        const Quat& character_rotation,
        const Vec3& character_angular_velocity,
        const Quat& simulation_rotation,
        const float max_adjustment_ratio,
        const float halflife,
        const float dt)
    {
        // Find and damp the desired rotational adjustment
        Quat adjustment_rotation = damp_adjustment_exact(
            quat_abs(quat_normalize(quat_mul_inv(
                simulation_rotation, character_rotation))),
            halflife,
            dt);

        // If the length of the adjustment is greater than the angular velocity 
        // multiplied by the ratio then we need to clamp this adjustment
        float max_length = max_adjustment_ratio *
            length(character_angular_velocity) * dt;

        if (length(quat_to_scaled_angle_axis(adjustment_rotation)) > max_length)
        {
            // To clamp can convert to scaled angle axis, rescale, and convert back
            adjustment_rotation = quat_from_scaled_angle_axis(max_length *
                normalize(quat_to_scaled_angle_axis(adjustment_rotation)));
        }

        // Apply the adjustment
        return quat_mul(adjustment_rotation, character_rotation);
    }

    //--------------------------------------

    static Vec3 clamp_character_position(
        const Vec3& character_position,
        const Vec3& simulation_position,
        const float max_distance)
    {
        // If the character deviates too far from the simulation 
        // position we need to clamp it to within the max distance
        if (length(character_position - simulation_position) > max_distance)
        {
            return max_distance *
                normalize(character_position - simulation_position) +
                simulation_position;
        }
        else
        {
            return character_position;
        }
    }

    static Quat clamp_character_rotation(
        const Quat& character_rotation,
        const Quat& simulation_rotation,
        const float max_angle)
    {
        // If the angle between the character rotation and simulation 
        // rotation exceeds the threshold we need to clamp it back
        if (quat_angle_between(character_rotation, simulation_rotation) > max_angle)
        {
            // First, find the rotational difference between the two
            Quat diff = quat_abs(quat_mul_inv(
                character_rotation, simulation_rotation));

            // We can then decompose it into angle and axis
            float diff_angle = 0.0f;
            Vec3 diff_axis;
            quat_to_angle_axis(diff, diff_angle, diff_axis);

            // We then clamp the angle to within our bounds
            diff_angle = clampf(diff_angle, -max_angle, max_angle);

            // And apply back the clamped rotation
            return quat_mul(
                quat_from_angle_axis(diff_angle, diff_axis), simulation_rotation);
        }
        else
        {
            return character_rotation;
        }
    }

	RTTR_REGISTRATION
	{
		using namespace rttr;

		registration::class_<MotionMatchingSystem>("MotionMatchingSystem")
			.constructor<World*>()(policy::ctor::as_raw_ptr, metadata("Tag", std::string("render")))
			.method("update", &MotionMatchingSystem::update)(metadata("update_group", update_types::RENDER));
	}

	MotionMatchingSystem::MotionMatchingSystem(World* _world)
		: System(_world)
	{
		renderer_holder_rc = world->add_root_component<RendererHolderRootComponent>();
		ASSERT(renderer_holder_rc != nullptr);
	}

	MotionMatchingSystem::~MotionMatchingSystem()
	{
		allocator->reset();
	}

	void MotionMatchingSystem::init()
	{
		allocator = get_transient_object<Allocator>();

        database_load(db, get_asset_path("/resources/assets/motion_matching/database.bin").c_str());

        database_build_matching_features(
            db,
            feature_weight_foot_position,
            feature_weight_foot_velocity,
            feature_weight_hip_velocity,
            feature_weight_trajectory_positions,
            feature_weight_trajectory_directions);

        database_save_matching_features(db, get_asset_path("/resources/assets/motion_matching/features.bin").c_str());

        world->get_registry().on_construct<MotionMatchingControllerComponent>().connect<&MotionMatchingSystem::on_controller_created>(this);
	}

	void MotionMatchingSystem::update(float dt)
	{
		using namespace animation;

        for (auto [handle, transform_component, reciever_component, controller] : world->group(components_group<TransformComponent, InputRecieverComponent, MotionMatchingControllerComponent>).each())
        {
            // Get gamepad stick states
            Vec3 gamepadstick_left = Vec3(reciever_component.get_current_input().x, reciever_component.get_current_input().y, reciever_component.get_current_input().z);
            Vec3 gamepadstick_right = Vec3();

            // Get if strafe is desired
            bool desired_strafe = reciever_component.get_frame_input().keyboard[key_ctrl].down;

            // Get the desired gait (walk / run)
            desired_gait_update(
                controller.desired_gait,
                controller.desired_gait_velocity,
                dt);

            // Get the desired simulation speeds based on the gait
            float simulation_fwrd_speed = lerpf(controller.simulation_run_fwrd_speed, controller.simulation_walk_fwrd_speed, controller.desired_gait);
            float simulation_side_speed = lerpf(controller.simulation_run_side_speed, controller.simulation_walk_side_speed, controller.desired_gait);
            float simulation_back_speed = lerpf(controller.simulation_run_back_speed, controller.simulation_walk_back_speed, controller.desired_gait);

            // Get the desired velocity
            Vec3 desired_velocity_curr = desired_velocity_update(
                gamepadstick_left,
                controller.simulation_rotation,
                simulation_fwrd_speed,
                simulation_side_speed,
                simulation_back_speed);

            // Get the desired rotation/direction
            Quat desired_rotation_curr = desired_rotation_update(
                controller.desired_rotation,
                gamepadstick_left,
                gamepadstick_right,
                0.0f,
                desired_strafe,
                desired_velocity_curr);

            // Check if we should force a search because input changed quickly
            controller.desired_velocity_change_prev = controller.desired_velocity_change_curr;
            controller.desired_velocity_change_curr = (desired_velocity_curr - controller.desired_velocity) / dt;
            controller.desired_velocity = desired_velocity_curr;

            controller.desired_rotation_change_prev = controller.desired_rotation_change_curr;
            controller.desired_rotation_change_curr = quat_to_scaled_angle_axis(quat_abs(quat_mul_inv(desired_rotation_curr, controller.desired_rotation))) / dt;
            controller.desired_rotation = desired_rotation_curr;

            bool force_search = false;

            if (controller.force_search_timer <= 0.0f && (
                (length(controller.desired_velocity_change_prev) >= controller.desired_velocity_change_threshold &&
                    length(controller.desired_velocity_change_curr) < controller.desired_velocity_change_threshold)
                || (length(controller.desired_rotation_change_prev) >= controller.desired_rotation_change_threshold &&
                    length(controller.desired_rotation_change_curr) < controller.desired_rotation_change_threshold)))
            {
                force_search = true;
                controller.force_search_timer = controller.search_time;
            }
            else if (controller.force_search_timer > 0)
            {
                controller.force_search_timer -= dt;
            }

            // Predict Future Trajectory

            trajectory_desired_rotations_predict(
                controller.trajectory_desired_rotations,
                controller.trajectory_desired_velocities,
                controller.desired_rotation,
                gamepadstick_left,
                gamepadstick_right,
                desired_strafe,
                20.0f * dt);

            trajectory_rotations_predict(
                controller.trajectory_rotations,
                controller.trajectory_angular_velocities,
                controller.simulation_rotation,
                controller.simulation_angular_velocity,
                controller.trajectory_desired_rotations,
                controller.simulation_rotation_halflife,
                20.0f * dt);

            trajectory_desired_velocities_predict(
                controller.trajectory_desired_velocities,
                controller.trajectory_rotations,
                controller.desired_velocity,
                gamepadstick_left,
                gamepadstick_right,
                desired_strafe,
                simulation_fwrd_speed,
                simulation_side_speed,
                simulation_back_speed,
                20.0f * dt);

            trajectory_positions_predict(
                controller.trajectory_positions,
                controller.trajectory_velocities,
                controller.trajectory_accelerations,
                controller.simulation_position,
                controller.simulation_velocity,
                controller.simulation_acceleration,
                controller.trajectory_desired_velocities,
                controller.simulation_velocity_halflife,
                20.0f * dt,
                obstacles_positions,
                obstacles_scales);

            // Make query vector for search.
            // In theory this only needs to be done when a search is 
            // actually required however for visualization purposes it
            // can be nice to do it every frame
            array1d<float> query(db.nfeatures());

            // Compute the features of the query vector
            slice1d<float> query_features = db.features(controller.frame_index);

            int offset = 0;
            query_copy_denormalized_feature(query, offset, 3, query_features, db.features_offset, db.features_scale); // Left Foot Position
            query_copy_denormalized_feature(query, offset, 3, query_features, db.features_offset, db.features_scale); // Right Foot Position
            query_copy_denormalized_feature(query, offset, 3, query_features, db.features_offset, db.features_scale); // Left Foot Velocity
            query_copy_denormalized_feature(query, offset, 3, query_features, db.features_offset, db.features_scale); // Right Foot Velocity
            query_copy_denormalized_feature(query, offset, 3, query_features, db.features_offset, db.features_scale); // Hip Velocity
            query_compute_trajectory_position_feature(query, offset, controller.bone_positions(0), controller.bone_rotations(0), controller.trajectory_positions);
            query_compute_trajectory_direction_feature(query, offset, controller.bone_rotations(0), controller.trajectory_rotations);

            assert(offset == db.nfeatures());

            // Check if we reached the end of the current anim
            bool end_of_anim = database_trajectory_index_clamp(db, controller.frame_index, 1) == controller.frame_index;

            if (force_search || controller.search_timer <= 0.0f || end_of_anim)
            {
                // Search

                int best_index = end_of_anim ? -1 : controller.frame_index;
                float best_cost = FLT_MAX;

                database_search(
                    best_index,
                    best_cost,
                    db,
                    query);

                // Transition if better frame found

                if (best_index != controller.frame_index && best_index != -1)
                {
                    controller.trns_bone_positions = db.bone_positions(best_index);
                    controller.trns_bone_velocities = db.bone_velocities(best_index);
                    controller.trns_bone_rotations = db.bone_rotations(best_index);
                    controller.trns_bone_angular_velocities = db.bone_angular_velocities(best_index);

                    inertialize_pose_transition(
                        controller.bone_offset_positions,
                        controller.bone_offset_velocities,
                        controller.bone_offset_rotations,
                        controller.bone_offset_angular_velocities,
                        controller.transition_src_position,
                        controller.transition_src_rotation,
                        controller.transition_dst_position,
                        controller.transition_dst_rotation,
                        controller.bone_positions(0),
                        controller.bone_velocities(0),
                        controller.bone_rotations(0),
                        controller.bone_angular_velocities(0),
                        controller.curr_bone_positions,
                        controller.curr_bone_velocities,
                        controller.curr_bone_rotations,
                        controller.curr_bone_angular_velocities,
                        controller.trns_bone_positions,
                        controller.trns_bone_velocities,
                        controller.trns_bone_rotations,
                        controller.trns_bone_angular_velocities);

                    controller.frame_index = best_index;
                }
                // Reset search timer
                controller.search_timer = controller.search_time;
            }

            // Tick down search timer
            controller.search_timer -= dt;

            // Tick frame
            controller.frame_index++; // Assumes dt is fixed to 60fps

            // Look-up Next Pose
            controller.curr_bone_positions = db.bone_positions(controller.frame_index);
            controller.curr_bone_velocities = db.bone_velocities(controller.frame_index);
            controller.curr_bone_rotations = db.bone_rotations(controller.frame_index);
            controller.curr_bone_angular_velocities = db.bone_angular_velocities(controller.frame_index);
            controller.curr_bone_contacts = db.contact_states(controller.frame_index);

            // Update inertializer

            inertialize_pose_update(
                controller.bone_positions,
                controller.bone_velocities,
                controller.bone_rotations,
                controller.bone_angular_velocities,
                controller.bone_offset_positions,
                controller.bone_offset_velocities,
                controller.bone_offset_rotations,
                controller.bone_offset_angular_velocities,
                controller.curr_bone_positions,
                controller.curr_bone_velocities,
                controller.curr_bone_rotations,
                controller.curr_bone_angular_velocities,
                controller.transition_src_position,
                controller.transition_src_rotation,
                controller.transition_dst_position,
                controller.transition_dst_rotation,
                controller.inertialize_blending_halflife,
                dt);

            // Update Simulation

            Vec3 simulation_position_prev = controller.simulation_position;

            simulation_positions_update(
                controller.simulation_position,
                controller.simulation_velocity,
                controller.simulation_acceleration,
                controller.desired_velocity,
                controller.simulation_velocity_halflife,
                dt,
                obstacles_positions,
                obstacles_scales);

            simulation_rotations_update(
                controller.simulation_rotation,
                controller.simulation_angular_velocity,
                controller.desired_rotation,
                controller.simulation_rotation_halflife,
                dt);

            // Synchronization 

            if (synchronization_enabled)
            {
                Vec3 synchronized_position = lerp(
                    controller.simulation_position,
                    controller.bone_positions(0),
                    controller.synchronization_data_factor);

                Quat synchronized_rotation = quat_nlerp_shortest(
                    controller.simulation_rotation,
                    controller.bone_rotations(0),
                    controller.synchronization_data_factor);

                synchronized_position = simulation_collide_obstacles(
                    simulation_position_prev,
                    synchronized_position,
                    obstacles_positions,
                    obstacles_scales);

                controller.simulation_position = synchronized_position;
                controller.simulation_rotation = synchronized_rotation;

                inertialize_root_adjust(
                    controller.bone_offset_positions(0),
                    controller.transition_src_position,
                    controller.transition_src_rotation,
                    controller.transition_dst_position,
                    controller.transition_dst_rotation,
                    controller.bone_positions(0),
                    controller.bone_rotations(0),
                    synchronized_position,
                    synchronized_rotation);
            }

            // Adjustment 

            if (!synchronization_enabled && adjustment_enabled)
            {
                Vec3 adjusted_position = controller.bone_positions(0);
                Quat adjusted_rotation = controller.bone_rotations(0);

                if (controller.adjustment_by_velocity_enabled)
                {
                    adjusted_position = adjust_character_position_by_velocity(
                        controller.bone_positions(0),
                        controller.bone_velocities(0),
                        controller.simulation_position,
                        controller.adjustment_position_max_ratio,
                        controller.adjustment_position_halflife,
                        dt);

                    adjusted_rotation = adjust_character_rotation_by_velocity(
                        controller.bone_rotations(0),
                        controller.bone_angular_velocities(0),
                        controller.simulation_rotation,
                        controller.adjustment_rotation_max_ratio,
                        controller.adjustment_rotation_halflife,
                        dt);
                }
                else
                {
                    adjusted_position = adjust_character_position(
                        controller.bone_positions(0),
                        controller.simulation_position,
                        controller.adjustment_position_halflife,
                        dt);

                    adjusted_rotation = adjust_character_rotation(
                        controller.bone_rotations(0),
                        controller.simulation_rotation,
                        controller.adjustment_rotation_halflife,
                        dt);
                }

                inertialize_root_adjust(
                    controller.bone_offset_positions(0),
                    controller.transition_src_position,
                    controller.transition_src_rotation,
                    controller.transition_dst_position,
                    controller.transition_dst_rotation,
                    controller.bone_positions(0),
                    controller.bone_rotations(0),
                    adjusted_position,
                    adjusted_rotation);
            }

            // Clamping

            if (!synchronization_enabled && clamping_enabled)
            {
                Vec3 adjusted_position = controller.bone_positions(0);
                Quat adjusted_rotation = controller.bone_rotations(0);

                adjusted_position = clamp_character_position(
                    adjusted_position,
                    controller.simulation_position,
                    controller.clamping_max_distance);

                adjusted_rotation = clamp_character_rotation(
                    adjusted_rotation,
                    controller.simulation_rotation,
                    controller.clamping_max_angle);

                inertialize_root_adjust(
                    controller.bone_offset_positions(0),
                    controller.transition_src_position,
                    controller.transition_src_rotation,
                    controller.transition_dst_position,
                    controller.transition_dst_rotation,
                    controller.bone_positions(0),
                    controller.bone_rotations(0),
                    adjusted_position,
                    adjusted_rotation);
            }

            // Contact fixup with foot locking and IK

            controller.adjusted_bone_positions = controller.bone_positions;
            controller.adjusted_bone_rotations = controller.bone_rotations;

            if (ik_enabled)
            {
                for (int i = 0; i < controller.contact_bones.size; i++)
                {
                    // Find all the relevant bone indices
                    int toe_bone = controller.contact_bones(i);
                    int heel_bone = db.bone_parents(toe_bone);
                    int knee_bone = db.bone_parents(heel_bone);
                    int hip_bone = db.bone_parents(knee_bone);
                    int root_bone = db.bone_parents(hip_bone);

                    // Compute the world space position for the toe
                    controller.global_bone_computed.zero();

                    forward_kinematics_partial(
                        controller.global_bone_positions,
                        controller.global_bone_rotations,
                        controller.global_bone_computed,
                        controller.bone_positions,
                        controller.bone_rotations,
                        db.bone_parents,
                        toe_bone);

                    // Update the contact state
                    contact_update(
                        controller.contact_states(i),
                        controller.contact_locks(i),
                        controller.contact_positions(i),
                        controller.contact_velocities(i),
                        controller.contact_points(i),
                        controller.contact_targets(i),
                        controller.contact_offset_positions(i),
                        controller.contact_offset_velocities(i),
                        controller.global_bone_positions(toe_bone),
                        controller.curr_bone_contacts(i),
                        controller.ik_unlock_radius,
                        controller.ik_foot_height,
                        controller.ik_blending_halflife,
                        dt);

                    // Ensure contact position never goes through floor
                    Vec3 contact_position_clamp = controller.contact_positions(i);
                    contact_position_clamp.y = maxf(contact_position_clamp.y, controller.ik_foot_height);

                    // Re-compute toe, heel, knee, hip, and root bone positions
                    for (int bone : {heel_bone, knee_bone, hip_bone, root_bone})
                    {
                        forward_kinematics_partial(
                            controller.global_bone_positions,
                            controller.global_bone_rotations,
                            controller.global_bone_computed,
                            controller.bone_positions,
                            controller.bone_rotations,
                            db.bone_parents,
                            bone);
                    }

                    // Perform simple two-joint IK to place heel
                    ik_two_bone(
                        controller.adjusted_bone_rotations(hip_bone),
                        controller.adjusted_bone_rotations(knee_bone),
                        controller.global_bone_positions(hip_bone),
                        controller.global_bone_positions(knee_bone),
                        controller.global_bone_positions(heel_bone),
                        contact_position_clamp + (controller.global_bone_positions(heel_bone) - controller.global_bone_positions(toe_bone)),
                        quat_mul_vec3(controller.global_bone_rotations(knee_bone), Vec3(0.0f, 1.0f, 0.0f)),
                        controller.global_bone_rotations(hip_bone),
                        controller.global_bone_rotations(knee_bone),
                        controller.global_bone_rotations(root_bone),
                        controller.ik_max_length_buffer);

                    // Re-compute toe, heel, and knee positions 
                    controller.global_bone_computed.zero();

                    for (int bone : {toe_bone, heel_bone, knee_bone})
                    {
                        forward_kinematics_partial(
                            controller.global_bone_positions,
                            controller.global_bone_rotations,
                            controller.global_bone_computed,
                            controller.adjusted_bone_positions,
                            controller.adjusted_bone_rotations,
                            db.bone_parents,
                            bone);
                    }

                    // Rotate heel so toe is facing toward contact point
                    ik_look_at(
                        controller.adjusted_bone_rotations(heel_bone),
                        controller.global_bone_rotations(knee_bone),
                        controller.global_bone_rotations(heel_bone),
                        controller.global_bone_positions(heel_bone),
                        controller.global_bone_positions(toe_bone),
                        contact_position_clamp);

                    // Re-compute toe and heel positions
                    controller.global_bone_computed.zero();

                    for (int bone : {toe_bone, heel_bone})
                    {
                        forward_kinematics_partial(
                            controller.global_bone_positions,
                            controller.global_bone_rotations,
                            controller.global_bone_computed,
                            controller.adjusted_bone_positions,
                            controller.adjusted_bone_rotations,
                            db.bone_parents,
                            bone);
                    }

                    // Rotate toe bone so that the end of the toe 
                    // does not intersect with the ground
                    Vec3 toe_end_curr = quat_mul_vec3(
                        controller.global_bone_rotations(toe_bone), Vec3(controller.ik_toe_length, 0.0f, 0.0f)) +
                        controller.global_bone_positions(toe_bone);

                    Vec3 toe_end_targ = toe_end_curr;
                    toe_end_targ.y = maxf(toe_end_targ.y, controller.ik_foot_height);

                    ik_look_at(
                        controller.adjusted_bone_rotations(toe_bone),
                        controller.global_bone_rotations(heel_bone),
                        controller.global_bone_rotations(toe_bone),
                        controller.global_bone_positions(toe_bone),
                        toe_end_curr,
                        toe_end_targ);
                }
            }

            // Full pass of forward kinematics to compute 
            // all bone positions and rotations in the world
            // space ready for rendering

            forward_kinematics_full(
                controller.global_bone_positions,
                controller.global_bone_rotations,
                controller.adjusted_bone_positions,
                controller.adjusted_bone_rotations,
                db.bone_parents);

            renderWireSphere(vec3(
                controller.simulation_position.x, 
                controller.simulation_position.y, 
                controller.simulation_position.z), 0.05f, vec4(0.0f, 1.0f, 0.0f, 1.0f), renderer_holder_rc->ldrRenderPass);
            
            
            Vec3 sim_dir_ = (
                controller.simulation_position + 0.6f * quat_mul_vec3(controller.simulation_rotation, Vec3(0.0f, 0.0f, 1.0f)));
            renderLine(vec3(
                    controller.simulation_position.x, 
                    controller.simulation_position.y,
                    controller.simulation_position.z), vec3(sim_dir_.x, sim_dir_.y, sim_dir_.z), vec4(0.0f, 1.0f, 0.0f, 1.0f), renderer_holder_rc->ldrRenderPass);

            transform_component.set_world_transform(
                trs(vec3(controller.simulation_position.x, controller.simulation_position.y, controller.simulation_position.z),
                    quat(controller.simulation_rotation.x, controller.simulation_rotation.y, controller.simulation_rotation.z, controller.simulation_rotation.w)));

            if (clamping_enabled)
            {
                Quat rotation_clamp_0 = quat_mul(quat_from_angle_axis(+controller.clamping_max_angle, Vec3(0.0f, 1.0f, 0.0f)), controller.simulation_rotation);
                Quat rotation_clamp_1 = quat_mul(quat_from_angle_axis(-controller.clamping_max_angle, Vec3(0.0f, 1.0f, 0.0f)), controller.simulation_rotation);

                Vec3 rotation_clamp_0_dir = controller.simulation_position + 0.6f * quat_mul_vec3(rotation_clamp_0, Vec3(0.0f, 0.0f, 1.0f));
                Vec3 rotation_clamp_1_dir = controller.simulation_position + 0.6f * quat_mul_vec3(rotation_clamp_1, Vec3(0.0f, 0.0f, 1.0f));

                renderLine(vec3(controller.simulation_position.x, controller.simulation_position.y, controller.simulation_position.z),
                    vec3(rotation_clamp_0_dir.x, rotation_clamp_0_dir.y, rotation_clamp_0_dir.z), vec4(1.0f, 0.0f, 1.0f, 1.0f), renderer_holder_rc->ldrRenderPass);
                renderLine(vec3(controller.simulation_position.x, controller.simulation_position.y, controller.simulation_position.z),
                    vec3(rotation_clamp_1_dir.x, rotation_clamp_1_dir.y, rotation_clamp_1_dir.z), vec4(1.0f, 0.0f, 1.0f, 1.0f), renderer_holder_rc->ldrRenderPass);
            }

            // Draw IK foot lock positions

            if (ik_enabled)
            {
                for (int i = 0; i < controller.contact_positions.size; i++)
                {
                    if (controller.contact_locks(i))
                    {
                        renderWireSphere(vec3(controller.contact_positions(i).x, controller.contact_positions(i).y, controller.contact_positions(i).z), 0.05f, vec4(0.0f, 1.0f, 0.0f, 1.0f), renderer_holder_rc->ldrRenderPass);
                    }
                }
            }

            draw_trajectory(
                controller.trajectory_positions,
                controller.trajectory_rotations,
                vec4(1.0f, 0.0f, 0.0f, 1.0f));

            // Draw matched features

            array1d<float> current_features =  db.features(controller.frame_index);
            denormalize_features(current_features, db.features_offset, db.features_scale);
            draw_features(current_features, controller.bone_positions(0), controller.bone_rotations(0), vec4(0.0f, 1.0f, 1.0f, 1.0f));

            for (int i = 0; i < controller.global_bone_positions.size; ++i)
            {
                const Vec3& pos = controller.global_bone_positions(i);

                const int bone_parent = db.bone_parents(i);
                if (bone_parent != -1)
                {
                    const Vec3& parent_pos = controller.global_bone_positions(bone_parent);
                    renderLine(vec3(pos.x, pos.y, pos.z), vec3(parent_pos.x, parent_pos.y, parent_pos.z), vec4(1.0f, 1.0f, 1.0f, 1.0f), renderer_holder_rc->ldrRenderPass);
                }
            }
        }
	}

    void MotionMatchingSystem::on_controller_created(entt::registry& registry, entt::entity entity)
    {
        MotionMatchingControllerComponent& controller = registry.get<MotionMatchingControllerComponent>(entity);
        TransformComponent& transform = registry.get<TransformComponent>(entity);

        controller.frame_index = db.range_starts(0);

        const trs& world_transform = transform.get_world_transform();

        const vec3& current_pos = world_transform.position;
        const quat& current_rot = world_transform.rotation;

        controller.simulation_position = Vec3(current_pos.x, current_pos.y, current_pos.z);
        controller.simulation_rotation = Quat(current_rot.w, current_rot.x, current_rot.y, current_rot.z);

        controller.curr_bone_positions = db.bone_positions(controller.frame_index);
        controller.curr_bone_velocities = db.bone_velocities(controller.frame_index);
        controller.curr_bone_rotations = db.bone_rotations(controller.frame_index);
        controller.curr_bone_angular_velocities = db.bone_angular_velocities(controller.frame_index);
        controller.curr_bone_contacts = db.contact_states(controller.frame_index);

        controller.trns_bone_positions = db.bone_positions(controller.frame_index);
        controller.trns_bone_velocities = db.bone_velocities(controller.frame_index);
        controller.trns_bone_rotations = db.bone_rotations(controller.frame_index);
        controller.trns_bone_angular_velocities = db.bone_angular_velocities(controller.frame_index);
        controller.trns_bone_contacts = db.contact_states(controller.frame_index);

        controller.bone_positions = db.bone_positions(controller.frame_index);
        controller.bone_velocities = db.bone_velocities(controller.frame_index);
        controller.bone_rotations = db.bone_rotations(controller.frame_index);
        controller.bone_angular_velocities = db.bone_angular_velocities(controller.frame_index);

        controller.bone_offset_positions.resize(db.nbones());
        controller.bone_offset_velocities.resize(db.nbones());
        controller.bone_offset_rotations.resize(db.nbones());
        controller.bone_offset_angular_velocities.resize(db.nbones());

        controller.global_bone_positions.resize(db.nbones());
        controller.global_bone_velocities.resize(db.nbones());
        controller.global_bone_rotations.resize(db.nbones());
        controller.global_bone_angular_velocities.resize(db.nbones());
        controller.global_bone_computed.resize(db.nbones());

        inertialize_pose_reset(
            controller.bone_offset_positions,
            controller.bone_offset_velocities,
            controller.bone_offset_rotations,
            controller.bone_offset_angular_velocities,
            controller.transition_src_position,
            controller.transition_src_rotation,
            controller.transition_dst_position,
            controller.transition_dst_rotation,
            controller.bone_positions(0),
            controller.bone_rotations(0));

        inertialize_pose_update(
            controller.bone_positions,
            controller.bone_velocities,
            controller.bone_rotations,
            controller.bone_angular_velocities,
            controller.bone_offset_positions,
            controller.bone_offset_velocities,
            controller.bone_offset_rotations,
            controller.bone_offset_angular_velocities,
            db.bone_positions(controller.frame_index),
            db.bone_velocities(controller.frame_index),
            db.bone_rotations(controller.frame_index),
            db.bone_angular_velocities(controller.frame_index),
            controller.transition_src_position,
            controller.transition_src_rotation,
            controller.transition_dst_position,
            controller.transition_dst_rotation,
            controller.inertialize_blending_halflife,
            0.0f);

        controller.trajectory_desired_velocities.resize(4);
        controller.trajectory_desired_rotations.resize(4);
        controller.trajectory_positions.resize(4);
        controller.trajectory_velocities.resize(4);
        controller.trajectory_accelerations.resize(4);
        controller.trajectory_rotations.resize(4);
        controller.trajectory_angular_velocities.resize(4);

        controller.contact_bones.resize(2);
        controller.contact_bones(0) = Bone_LeftToe;
        controller.contact_bones(1) = Bone_RightToe;

        controller.contact_states.resize(controller.contact_bones.size);
        controller.contact_locks.resize(controller.contact_bones.size);
        controller.contact_positions.resize(controller.contact_bones.size);
        controller.contact_velocities.resize(controller.contact_bones.size);
        controller.contact_points.resize(controller.contact_bones.size);
        controller.contact_targets.resize(controller.contact_bones.size);
        controller.contact_offset_positions.resize(controller.contact_bones.size);
        controller.contact_offset_velocities.resize(controller.contact_bones.size);

        for (int i = 0; i < controller.contact_bones.size; i++)
        {
            Vec3 bone_position = Vec3();
            Vec3 bone_velocity = Vec3();
            Quat bone_rotation = Quat();
            Vec3 bone_angular_velocity = Vec3();

            forward_kinematics_velocity(
                bone_position,
                bone_velocity,
                bone_rotation,
                bone_angular_velocity,
                controller.bone_positions,
                controller.bone_velocities,
                controller.bone_rotations,
                controller.bone_angular_velocities,
                db.bone_parents,
                controller.contact_bones(i));

            contact_reset(
                controller.contact_states(i),
                controller.contact_locks(i),
                controller.contact_positions(i),
                controller.contact_velocities(i),
                controller.contact_points(i),
                controller.contact_targets(i),
                controller.contact_offset_positions(i),
                controller.contact_offset_velocities(i),
                bone_position,
                bone_velocity,
                false);
        }

        controller.adjusted_bone_positions = controller.bone_positions;
        controller.adjusted_bone_rotations = controller.bone_rotations;
    }

    void MotionMatchingSystem::draw_features(const slice1d<float> features, const Vec3 pos, const Quat rot, const vec4 color)
    {
        Vec3 lfoot_pos = quat_mul_vec3(rot, Vec3(features(0), features(1), features(2))) + pos;
        Vec3 rfoot_pos = quat_mul_vec3(rot, Vec3(features(3), features(4), features(5))) + pos;
        Vec3 lfoot_vel = quat_mul_vec3(rot, Vec3(features(6), features(7), features(8)));
        Vec3 rfoot_vel = quat_mul_vec3(rot, Vec3(features(9), features(10), features(11)));
        //vec3 hip_vel   = quat_mul_vec3(rot, vec3(features(12), features(13), features(14)));
        Vec3 traj0_pos = quat_mul_vec3(rot, Vec3(features(15), 0.0f, features(16))) + pos;
        Vec3 traj1_pos = quat_mul_vec3(rot, Vec3(features(17), 0.0f, features(18))) + pos;
        Vec3 traj2_pos = quat_mul_vec3(rot, Vec3(features(19), 0.0f, features(20))) + pos;
        Vec3 traj0_dir = quat_mul_vec3(rot, Vec3(features(21), 0.0f, features(22)));
        Vec3 traj1_dir = quat_mul_vec3(rot, Vec3(features(23), 0.0f, features(24)));
        Vec3 traj2_dir = quat_mul_vec3(rot, Vec3(features(25), 0.0f, features(26)));

        renderWireSphere(vec3(lfoot_pos.x, lfoot_pos.y, lfoot_pos.z), 0.05f, color, renderer_holder_rc->ldrRenderPass);
        renderWireSphere(vec3(rfoot_pos.x, rfoot_pos.y, rfoot_pos.z), 0.05f, color, renderer_holder_rc->ldrRenderPass);
        renderWireSphere(vec3(traj0_pos.x, traj0_pos.y, traj0_pos.z), 0.05f, color, renderer_holder_rc->ldrRenderPass);
        renderWireSphere(vec3(traj1_pos.x, traj1_pos.y, traj1_pos.z), 0.05f, color, renderer_holder_rc->ldrRenderPass);
        renderWireSphere(vec3(traj2_pos.x, traj2_pos.y, traj2_pos.z), 0.05f, color, renderer_holder_rc->ldrRenderPass);

        renderLine(vec3(lfoot_pos.x, lfoot_pos.y, lfoot_pos.z), vec3(lfoot_pos.x, lfoot_pos.y, lfoot_pos.z) + 0.1f * vec3(lfoot_vel.x, lfoot_vel.y, lfoot_vel.z), color, renderer_holder_rc->ldrRenderPass);
        renderLine(vec3(rfoot_pos.x, rfoot_pos.y, rfoot_pos.z), vec3(rfoot_pos.x, rfoot_pos.y, rfoot_pos.z) + 0.1f * vec3(rfoot_vel.x, rfoot_vel.y, rfoot_vel.z), color, renderer_holder_rc->ldrRenderPass);

        renderLine(vec3(traj0_pos.x, traj0_pos.y, traj0_pos.z), vec3(traj0_pos.x, traj0_pos.y, traj0_pos.z) + 0.25f * vec3(traj0_dir.x, traj0_dir.y, traj0_dir.z), color, renderer_holder_rc->ldrRenderPass);
        renderLine(vec3(traj1_pos.x, traj1_pos.y, traj1_pos.z), vec3(traj1_pos.x, traj1_pos.y, traj1_pos.z) + 0.25f * vec3(traj1_dir.x, traj1_dir.y, traj1_dir.z), color, renderer_holder_rc->ldrRenderPass);
        renderLine(vec3(traj2_pos.x, traj2_pos.y, traj2_pos.z), vec3(traj2_pos.x, traj2_pos.y, traj2_pos.z) + 0.25f * vec3(traj2_dir.x, traj2_dir.y, traj2_dir.z), color, renderer_holder_rc->ldrRenderPass);
    }

    void MotionMatchingSystem::draw_trajectory(const slice1d<Vec3> trajectory_positions, const slice1d<Quat> trajectory_rotations, const vec4 color)
    {
        for (int i = 1; i < trajectory_positions.size; i++)
        {
            vec3 point = vec3(trajectory_positions(i).x, trajectory_positions(i).y, trajectory_positions(i).z);
            renderWireSphere(point, 0.05f, color, renderer_holder_rc->ldrRenderPass);

            Vec3 dir_ = quat_mul_vec3(trajectory_rotations(i), Vec3(0.0f, 0.0f, 1.0f));
            vec3 dir = vec3(dir_.x, dir_.y, dir_.z);
            renderLine(point, point + 0.6f * dir, color, renderer_holder_rc->ldrRenderPass);
            renderLine(vec3(trajectory_positions(i - 1).x, trajectory_positions(i - 1).y, trajectory_positions(i - 1).z), point, color, renderer_holder_rc->ldrRenderPass);
        }
    }

}