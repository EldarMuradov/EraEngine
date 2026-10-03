#pragma once

#include "physics_api.h"

#include "physics/physical_animation/ragdoll_profile.h"

#include <asset/game_asset.h>

namespace era_engine::physics
{
    struct ERA_PHYSICS_API RagdollProfileSelectionSettings final
    {
        float running_speed = 2.0f;
        float sprint_speed = 4.5f;

        // A threshold must be crossed by hysteresis / 2 to switch, so the profile does not flicker around it.
        float hysteresis = 0.3f;

        float transition_time = 0.3f;
    };

    class ERA_PHYSICS_API RagdollProfilesAsset final : public GameAsset
    {
    public:
        static constexpr uint32 PROFILES_COUNT = static_cast<uint32>(RagdollProfileType::COUNT);

        RagdollProfilesAsset() = default;
        ~RagdollProfilesAsset() override = default;

        static ref<RagdollProfilesAsset> create_default();

        ref<RagdollProfile> get_profile(RagdollProfileType type) const;

        bool serialize(std::ostream& os) const override;
        bool deserialize(std::istream& is) override;

        static std::string get_asset_type_impl();

    public:
        std::array<ref<RagdollProfile>, PROFILES_COUNT> profiles;

        RagdollProfileSelectionSettings selection_settings;

        uint32 revision = 0;
    };
}
