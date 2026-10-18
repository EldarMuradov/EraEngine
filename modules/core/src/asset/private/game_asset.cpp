#include "asset/game_asset.h"
#include "asset/asset.h"

namespace era_engine
{
    struct AssetKey
    {
        std::wstring path;
        uint32 flags = 0;

        bool operator==(const AssetKey& other) const = default;
    };

    struct AssetKeyHash
    {
        size_t operator()(const AssetKey& key) const
        {
            size_t seed = 0;
            hash_combine(seed, key.path);
            hash_combine(seed, key.flags);
            return seed;
        }
    };

    static std::unordered_map<AssetKey, weakref<GameAsset>, AssetKeyHash> ASSET_CACHE;
    static std::mutex ASSET_CACHE_MUTEX;

    static AssetKey make_asset_key(const fs::path& filename, uint32 flags)
    {
        return AssetKey{fs::absolute(filename).lexically_normal().make_preferred().wstring(), flags};
    }

    ref<GameAsset> GameAssetCache::try_get_asset_from_cache(const fs::path& filename, uint32 flags)
    {
        const AssetKey key = make_asset_key(filename, flags);

        std::lock_guard _lock{ASSET_CACHE_MUTEX};

        const auto iter = ASSET_CACHE.find(key);
        if (iter == ASSET_CACHE.end())
        {
            return nullptr;
        }

        ref<GameAsset> result = iter->second.lock();
        if (result == nullptr)
        {
            ASSET_CACHE.erase(iter);
        }
        return result;
    }

    void GameAssetCache::try_add_asset_to_cache(ref<GameAsset> asset, const fs::path& filename, uint32 flags)
    {
        AssetKey key = make_asset_key(filename, flags);

        std::lock_guard _lock{ASSET_CACHE_MUTEX};

        weakref<GameAsset>& cached = ASSET_CACHE[std::move(key)];
        if (cached.expired())
        {
            cached = asset;
        }
    }

    void GameAssetCache::remove_asset_from_cache(const fs::path& filename, uint32 flags)
    {
        const AssetKey key = make_asset_key(filename, flags);

        std::lock_guard _lock{ASSET_CACHE_MUTEX};
        ASSET_CACHE.erase(key);
    }
}
