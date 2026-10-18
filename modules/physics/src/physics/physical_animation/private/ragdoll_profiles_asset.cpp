#include "physics/physical_animation/ragdoll_profiles_asset.h"

#include <core/yaml.h>
#include <core/log.h>
#include <core/traits.h>

#include <ecs/reflection_utils.h>

namespace era_engine::physics
{
    static constexpr uint32 RAGDOLL_PROFILES_ASSET_VERSION = 1;

    static YAML::Node blend_type_to_node(PhysicalLimbBlendType blend_type)
    {
        YAML::Node node;
        node.SetStyle(YAML::EmitterStyle::Flow);
        for (const PhysicalLimbBlendType flag : ReflectionUtils::get_enum_values<PhysicalLimbBlendType>())
        {
            if (flag != PhysicalLimbBlendType::NONE && has_flag(blend_type, flag))
            {
                node.push_back(ReflectionUtils::enum_to_string(flag));
            }
        }
        return node;
    }

    static PhysicalLimbBlendType blend_type_from_node(const YAML::Node& node)
    {
        PhysicalLimbBlendType result = PhysicalLimbBlendType::NONE;
        for (const YAML::Node& flag_node : node)
        {
            const std::string flag_name = flag_node.as<std::string>();

            PhysicalLimbBlendType flag = PhysicalLimbBlendType::NONE;
            if (ReflectionUtils::string_to_enum(flag_name, flag))
            {
                result |= flag;
            }
            else
            {
                LOG_ERROR("Ragdoll profiles> Unknown blend type '%s'.", flag_name.c_str());
            }
        }
        return result;
    }

    static YAML::Node limb_details_to_node(const PhysicalLimbDetails& details)
    {
        YAML::Node node;

        node["BlendType"] = blend_type_to_node(details.blend_type);

        YAML::Node strength_node;
        strength_node.SetStyle(YAML::EmitterStyle::Flow);
        strength_node["Default"] = details.strength_details.default_strength_coeff;
        strength_node["Soft"] = details.strength_details.soft_strength_coeff;
        strength_node["Hard"] = details.strength_details.hard_strength_coeff;
        node["Strength"] = strength_node;

        if (details.motor_drive.has_value())
        {
            const AngularDriveSettings& angular = details.motor_drive->angular;
            YAML::Node angular_node;
            angular_node["Frequency"] = angular.frequency;
            angular_node["DampingRatio"] = angular.damping_ratio;
            angular_node["FarDampingRatio"] = angular.far_damping_ratio;
            angular_node["ErrorRangeDeg"] = angular.error_range_deg;
            angular_node["SaturationAngleDeg"] = angular.saturation_angle_deg;

            const LinearDriveSettings& linear = details.motor_drive->linear;
            YAML::Node linear_node;
            linear_node["Frequency"] = linear.frequency;
            linear_node["DampingRatio"] = linear.damping_ratio;
            linear_node["FarDampingRatio"] = linear.far_damping_ratio;
            linear_node["ErrorRange"] = linear.error_range;
            linear_node["SaturationDistance"] = linear.saturation_distance;

            node["MotorDrive"]["Angular"] = angular_node;
            node["MotorDrive"]["Linear"] = linear_node;
        }

        if (details.drag_force.has_value())
        {
            const DragForceDetails& drag = details.drag_force.value();
            YAML::Node drag_node;
            drag_node["PartialAngularDriveLimit"] = drag.partial_angular_drive_limit;
            drag_node["PartialAngularDrive"] = drag.partial_angular_drive;
            drag_node["PartialVelocityDriveLimit"] = drag.partial_velocity_drive_limit;
            drag_node["PartialVelocityDrive"] = drag.partial_velocity_drive;
            drag_node["AccelerationLimit"] = drag.acceleration_limit;
            drag_node["AccelerationGain"] = drag.acceleration_gain;
            node["DragForce"] = drag_node;
        }

        return node;
    }

    static PhysicalLimbDetails limb_details_from_node(const YAML::Node& node)
    {
        PhysicalLimbDetails details;

        if (const YAML::Node blend_node = node["BlendType"])
        {
            details.blend_type = blend_type_from_node(blend_node);
        }

        if (const YAML::Node strength_node = node["Strength"])
        {
            YAML_LOAD(strength_node, details.strength_details.default_strength_coeff, "Default");
            YAML_LOAD(strength_node, details.strength_details.soft_strength_coeff, "Soft");
            YAML_LOAD(strength_node, details.strength_details.hard_strength_coeff, "Hard");
        }

        if (const YAML::Node motor_node = node["MotorDrive"])
        {
            MotorDriveSettings motor_drive;

            if (const YAML::Node angular_node = motor_node["Angular"])
            {
                AngularDriveSettings& angular = motor_drive.angular;
                YAML_LOAD(angular_node, angular.frequency, "Frequency");
                YAML_LOAD(angular_node, angular.damping_ratio, "DampingRatio");
                YAML_LOAD(angular_node, angular.far_damping_ratio, "FarDampingRatio");
                YAML_LOAD(angular_node, angular.error_range_deg, "ErrorRangeDeg");
                YAML_LOAD(angular_node, angular.saturation_angle_deg, "SaturationAngleDeg");
            }

            if (const YAML::Node linear_node = motor_node["Linear"])
            {
                LinearDriveSettings& linear = motor_drive.linear;
                YAML_LOAD(linear_node, linear.frequency, "Frequency");
                YAML_LOAD(linear_node, linear.damping_ratio, "DampingRatio");
                YAML_LOAD(linear_node, linear.far_damping_ratio, "FarDampingRatio");
                YAML_LOAD(linear_node, linear.error_range, "ErrorRange");
                YAML_LOAD(linear_node, linear.saturation_distance, "SaturationDistance");
            }

            details.motor_drive = motor_drive;
        }

        if (const YAML::Node drag_node = node["DragForce"])
        {
            DragForceDetails drag;
            YAML_LOAD(drag_node, drag.partial_angular_drive_limit, "PartialAngularDriveLimit");
            YAML_LOAD(drag_node, drag.partial_angular_drive, "PartialAngularDrive");
            YAML_LOAD(drag_node, drag.partial_velocity_drive_limit, "PartialVelocityDriveLimit");
            YAML_LOAD(drag_node, drag.partial_velocity_drive, "PartialVelocityDrive");
            YAML_LOAD(drag_node, drag.acceleration_limit, "AccelerationLimit");
            YAML_LOAD(drag_node, drag.acceleration_gain, "AccelerationGain");
            details.drag_force = drag;
        }

        return details;
    }

    bool RagdollProfilesAsset::serialize(std::ostream& os) const
    {
        try
        {
            YAML::Node root;
            root["Version"] = RAGDOLL_PROFILES_ASSET_VERSION;

            YAML::Node selection_node;
            selection_node["RunningSpeed"] = selection_settings.running_speed;
            selection_node["SprintSpeed"] = selection_settings.sprint_speed;
            selection_node["Hysteresis"] = selection_settings.hysteresis;
            selection_node["TransitionTime"] = selection_settings.transition_time;
            root["Selection"] = selection_node;

            YAML::Node profiles_node;
            for (uint32 profile_index = 0; profile_index < PROFILES_COUNT; ++profile_index)
            {
                const ref<RagdollProfile>& profile = profiles[profile_index];
                if (profile == nullptr)
                {
                    continue;
                }

                YAML::Node profile_node;
                for (uint32 limb_index = 0; limb_index < RagdollProfile::LIMB_TYPES_COUNT; ++limb_index)
                {
                    const RagdollLimbType limb_type = static_cast<RagdollLimbType>(limb_index);
                    profile_node[ReflectionUtils::enum_to_string(limb_type)] = limb_details_to_node(profile->get_limb_details_by_type(limb_type));
                }
                profiles_node[ReflectionUtils::enum_to_string(static_cast<RagdollProfileType>(profile_index))] = profile_node;
            }
            root["Profiles"] = profiles_node;

            YAML::Emitter out;
            out << root;
            os << out.c_str();
        }
        catch (const std::exception& exception)
        {
            LOG_ERROR("Ragdoll profiles> Failed to serialize: %s", exception.what());
            return false;
        }

        return os.good();
    }

    bool RagdollProfilesAsset::deserialize(std::istream& is)
    {
        try
        {
            const YAML::Node root = YAML::Load(is);

            uint32 version = 0;
            YAML_LOAD(root, version, "Version");
            if (version != RAGDOLL_PROFILES_ASSET_VERSION)
            {
                LOG_ERROR("Ragdoll profiles> Unsupported version %u (expected %u).", version, RAGDOLL_PROFILES_ASSET_VERSION);
                return false;
            }

            if (const YAML::Node selection_node = root["Selection"])
            {
                YAML_LOAD(selection_node, selection_settings.running_speed, "RunningSpeed");
                YAML_LOAD(selection_node, selection_settings.sprint_speed, "SprintSpeed");
                YAML_LOAD(selection_node, selection_settings.hysteresis, "Hysteresis");
                YAML_LOAD(selection_node, selection_settings.transition_time, "TransitionTime");
            }

            profiles = {};
            if (const YAML::Node profiles_node = root["Profiles"])
            {
                for (uint32 profile_index = 0; profile_index < PROFILES_COUNT; ++profile_index)
                {
                    const YAML::Node profile_node = profiles_node[ReflectionUtils::enum_to_string(static_cast<RagdollProfileType>(profile_index))];
                    if (!profile_node)
                    {
                        continue;
                    }

                    ref<RagdollProfile> profile = make_ref<RagdollProfile>();
                    profile->type = static_cast<RagdollProfileType>(profile_index);

                    for (uint32 limb_index = 0; limb_index < RagdollProfile::LIMB_TYPES_COUNT; ++limb_index)
                    {
                        const RagdollLimbType limb_type = static_cast<RagdollLimbType>(limb_index);
                        if (const YAML::Node limb_node = profile_node[ReflectionUtils::enum_to_string(limb_type)])
                        {
                            profile->get_limb_details_by_type(limb_type) = limb_details_from_node(limb_node);
                        }
                    }

                    profiles[profile_index] = profile;
                }
            }
        }
        catch (const std::exception& exception)
        {
            LOG_ERROR("Ragdoll profiles> Failed to deserialize: %s", exception.what());
            return false;
        }

        ++revision;
        return true;
    }

    std::string RagdollProfilesAsset::get_asset_type_impl()
    {
        return ".eragprof";
    }

    ref<RagdollProfile> RagdollProfilesAsset::get_profile(RagdollProfileType type) const
    {
        const uint32 index = static_cast<uint32>(type);
        if (index >= PROFILES_COUNT)
        {
            return nullptr;
        }
        return profiles[index];
    }

    static PhysicalLimbDetails make_driven_limb(float frequency,
        float damping_ratio,
        float linear_frequency)
    {
        PhysicalLimbDetails details;
        details.blend_type = PhysicalLimbBlendType::BLEND_WITH_PREV_POSE;

        MotorDriveSettings motor_drive;

        motor_drive.angular.frequency = frequency;
        motor_drive.angular.damping_ratio = damping_ratio;
        motor_drive.angular.far_damping_ratio = 0.5f * damping_ratio;
        motor_drive.angular.saturation_angle_deg = rad2deg(1.0f);

        motor_drive.linear.frequency = linear_frequency;
        motor_drive.linear.damping_ratio = 1.0f;
        motor_drive.linear.far_damping_ratio = 0.5f;
        motor_drive.linear.saturation_distance = 0.3f;

        details.motor_drive = motor_drive;

        return details;
    }

    static ref<RagdollProfile> make_driven_profile(RagdollProfileType type, float frequency_scale, float damping_scale)
    {
        ref<RagdollProfile> profile = make_ref<RagdollProfile>();
        profile->type = type;

        const float f = frequency_scale;
        const float z = damping_scale;

        profile->body_lower_limb_details = make_driven_limb(7.5f * f, 1.0f * z, 4.5f * f);
        profile->body_middle_limb_details = make_driven_limb(7.5f * f, 1.0f * z, 4.5f * f);
        profile->body_upper_limb_details = make_driven_limb(7.5f * f, 1.0f * z, 4.5f * f);

        profile->neck_limb_details = make_driven_limb(5.5f * f, 0.6f * z, 4.0f * f);
        profile->head_limb_details = make_driven_limb(5.5f * f, 0.6f * z, 4.0f * f);

        profile->clavicle_limb_details = make_driven_limb(10.0f * f, 0.65f * z, 4.0f * f);
        profile->arm_limb_details = make_driven_limb(6.8f * f, 0.6f * z, 4.0f * f);
        profile->forearm_limb_details = make_driven_limb(8.2f * f, 0.65f * z, 4.25f * f);
        profile->hand_limb_details = make_driven_limb(10.0f * f, 0.55f * z, 4.25f * f);

        profile->leg_limb_details = make_driven_limb(4.0f * f, 0.5f * z, 4.0f * f);
        profile->calf_limb_details = make_driven_limb(4.5f * f, 0.55f * z, 4.0f * f);
        profile->foot_limb_details = make_driven_limb(6.5f * f, 0.65f * z, 4.25f * f);

        return profile;
    }

    ref<RagdollProfilesAsset> RagdollProfilesAsset::create_default()
    {
        ref<RagdollProfilesAsset> asset = make_ref<RagdollProfilesAsset>();

        asset->profiles[static_cast<uint32>(RagdollProfileType::IDLE)] = make_driven_profile(RagdollProfileType::IDLE, 1.0f, 1.0f);
        asset->profiles[static_cast<uint32>(RagdollProfileType::RUNNING)] = make_driven_profile(RagdollProfileType::RUNNING, 1.07f, 0.85f);
        asset->profiles[static_cast<uint32>(RagdollProfileType::SPRINT)] = make_driven_profile(RagdollProfileType::SPRINT, 1.12f, 0.8f);

        ref<RagdollProfile> ragdoll_profile = make_ref<RagdollProfile>();
        ragdoll_profile->type = RagdollProfileType::RAGDOLL;
        ragdoll_profile->head_limb_details.blend_type = PhysicalLimbBlendType::BLEND_WITH_PREV_POSE;
        ragdoll_profile->arm_limb_details.blend_type = PhysicalLimbBlendType::BLEND_WITH_PREV_POSE;
        ragdoll_profile->body_upper_limb_details.blend_type = PhysicalLimbBlendType::BLEND_WITH_PREV_POSE;
        ragdoll_profile->body_middle_limb_details.blend_type = PhysicalLimbBlendType::BLEND_WITH_PREV_POSE;
        ragdoll_profile->forearm_limb_details.blend_type = PhysicalLimbBlendType::BLEND_WITH_PREV_POSE;
        ragdoll_profile->hand_limb_details.blend_type = PhysicalLimbBlendType::BLEND_WITH_PREV_POSE;
        ragdoll_profile->leg_limb_details.blend_type = PhysicalLimbBlendType::BLEND_WITH_PREV_POSE;
        ragdoll_profile->calf_limb_details.blend_type = PhysicalLimbBlendType::BLEND_WITH_PREV_POSE;
        ragdoll_profile->foot_limb_details.blend_type = PhysicalLimbBlendType::BLEND_WITH_PREV_POSE;
        asset->profiles[static_cast<uint32>(RagdollProfileType::RAGDOLL)] = ragdoll_profile;

        return asset;
    }
}
