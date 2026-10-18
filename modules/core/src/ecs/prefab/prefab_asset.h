#pragma once

#include "core_api.h"

#include "asset/game_asset.h"

#include "core/math.h"

namespace era_engine
{
    class ERA_CORE_API PrefabAsset final : public GameAsset
    {
    public:
        PrefabAsset();
        ~PrefabAsset() override;

        bool serialize(std::ostream& os) const override;
        bool deserialize(std::istream& is) override;

        bool is_ready() const;
        bool has_failed() const;
        void wait_until_loaded() const;

        size_t get_entity_count() const;
        trs get_local_transform(uint32 local_id) const;

        static std::string get_asset_type_impl();

    private:
        struct Impl;
        ref<Impl> impl;

        friend class PrefabUtils;
    };
}
