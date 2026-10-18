#include "asset/asset_type_registry.h"

#include "core/string.h"
#include "core/log.h"

#include "geometry/mesh.h"
#include "animation/skeleton.h"
#include "animation/animation_clip.h"

#include <rttr/registration>

#include <deque>

namespace era_engine
{
    RTTR_REGISTRATION
    {
        AssetTypeRegistry::register_asset_type<MultiMesh>();
        AssetTypeRegistry::register_asset_type<animation::Skeleton>();
        AssetTypeRegistry::register_asset_type<animation::AnimationAssetClip>();
    }

    struct AssetTypeRegistryData
    {
        std::mutex sync;
        std::deque<AssetTypeInfo> infos;
        std::unordered_map<rttr::type::type_id, const AssetTypeInfo*> by_type;
        std::unordered_map<std::string, const AssetTypeInfo*> by_extension;

        std::optional<fs::path> assets_root;
    };

    static AssetTypeRegistryData& get_registry_data()
    {
        static AssetTypeRegistryData data;
        return data;
    }

    static fs::path normalize_path(const fs::path& path)
    {
        return fs::absolute(path).lexically_normal().make_preferred();
    }

    const AssetTypeInfo* AssetTypeRegistry::find(const rttr::type& ref_type)
    {
        AssetTypeRegistryData& data = get_registry_data();
        std::lock_guard _lock{data.sync};

        const auto iter = data.by_type.find(ref_type.get_id());
        return iter != data.by_type.end() ? iter->second : nullptr;
    }

    const AssetTypeInfo* AssetTypeRegistry::find_by_extension(std::string_view extension)
    {
        AssetTypeRegistryData& data = get_registry_data();
        std::lock_guard _lock{data.sync};

        const auto iter = data.by_extension.find(std::string(extension));
        return iter != data.by_extension.end() ? iter->second : nullptr;
    }

    fs::path AssetTypeRegistry::get_assets_root()
    {
        AssetTypeRegistryData& data = get_registry_data();
        std::lock_guard _lock{data.sync};

        if (data.assets_root.has_value())
        {
            return *data.assets_root;
        }
        return normalize_path(get_asset_path(L"/resources/assets"));
    }

    void AssetTypeRegistry::set_assets_root(const fs::path& root)
    {
        AssetTypeRegistryData& data = get_registry_data();
        std::lock_guard _lock{data.sync};

        data.assets_root = normalize_path(root);
    }

    std::optional<std::string> AssetTypeRegistry::to_relative_path(const fs::path& path)
    {
        if (path.empty())
        {
            return std::nullopt;
        }

        const fs::path relative_path = normalize_path(path).lexically_relative(get_assets_root());
        if (relative_path.empty() || *relative_path.begin() == "..")
        {
            return std::nullopt;
        }
        return relative_path.generic_string();
    }

    fs::path AssetTypeRegistry::to_absolute_path(std::string_view relative_path)
    {
        return (get_assets_root() / fs::path(relative_path)).lexically_normal().make_preferred();
    }

    void AssetTypeRegistry::register_type(const AssetTypeInfo& info)
    {
        AssetTypeRegistryData& data = get_registry_data();
        std::lock_guard _lock{data.sync};

        if (data.by_type.contains(info.ref_type.get_id()) || data.by_extension.contains(info.extension))
        {
            LOG_ERROR("AssetTypeRegistry> Asset type %s is registered twice.", info.extension.c_str());
            return;
        }

        const AssetTypeInfo& stored_info = data.infos.emplace_back(info);
        data.by_type.emplace(info.ref_type.get_id(), &stored_info);
        data.by_extension.emplace(info.extension, &stored_info);
    }
}
