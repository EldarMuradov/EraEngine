#pragma once

#include "core_api.h"

#include "asset/game_asset.h"

#include <rttr/type>

#include <optional>

namespace era_engine
{
    struct AssetReference
    {
        std::string path;
        uint32 flags = 0;

        bool operator==(const AssetReference& other) const = default;
    };

    struct AssetReferenceHash
    {
        size_t operator()(const AssetReference& reference) const
        {
            return std::hash<std::string>()(reference.path) ^ (std::hash<uint32>()(reference.flags) << 1);
        }
    };

    struct AssetTypeInfo
    {
        rttr::type ref_type;
        std::string extension;

        ref<GameAsset> (*load)(const fs::path& path, uint32 flags) = nullptr;
        // Returns an empty ref<T> if the asset has another type.
        rttr::variant (*to_variant)(const ref<GameAsset>& asset) = nullptr;
        ref<GameAsset> (*from_variant)(const rttr::variant& value) = nullptr;
    };

    class ERA_CORE_API AssetTypeRegistry final
    {
        AssetTypeRegistry() = delete;

    public:
        template <typename Asset_>
        static void register_asset_type()
        {
            static_assert(std::is_base_of_v<GameAsset, Asset_>);

            AssetTypeInfo info{rttr::type::get<ref<Asset_>>(), AssetExtension<Asset_>::get_asset_type()};
            info.load = [](const fs::path& path, uint32 flags) -> ref<GameAsset>
            {
                GameAssetsProvider provider;
                return provider.load_game_asset_from_file<Asset_>(path, true, {}, flags);
            };
            info.to_variant = [](const ref<GameAsset>& asset)
            {
                return rttr::variant(std::dynamic_pointer_cast<Asset_>(asset));
            };
            info.from_variant = [](const rttr::variant& value) -> ref<GameAsset>
            {
                return value.get_value<ref<Asset_>>();
            };

            register_type(info);
        }

        static const AssetTypeInfo* find(const rttr::type& ref_type);
        static const AssetTypeInfo* find_by_extension(std::string_view extension);

        static fs::path get_assets_root();
        static void set_assets_root(const fs::path& root);

        static std::optional<std::string> to_relative_path(const fs::path& path);
        static fs::path to_absolute_path(std::string_view relative_path);

    private:
        static void register_type(const AssetTypeInfo& info);
    };
}
