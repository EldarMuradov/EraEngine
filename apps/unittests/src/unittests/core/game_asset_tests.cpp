#include <gtest/gtest.h>

#include <asset/game_asset.h>

#include <fstream>
#include <string>

namespace
{
    using namespace era_engine;

    class TestTextAsset final : public GameAsset
    {
    public:
        bool serialize(std::ostream& os) const override
        {
            os << text;
            return true;
        }

        bool deserialize(std::istream& is) override
        {
            std::getline(is, text);
            return text != "invalid";
        }

        static std::string get_asset_type_impl()
        {
            return ".etest";
        }

        std::string text;
    };

    class GameAssetTest : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            directory = fs::temp_directory_path() / "era_engine_unittests" / "game_assets";
            fs::create_directories(directory);
        }

        void TearDown() override
        {
            std::error_code error;
            fs::remove_all(directory, error);
        }

        fs::path write_file(const std::string& name, const std::string& content) const
        {
            const fs::path path = directory / name;
            std::ofstream os(path, std::ios_base::binary | std::ios_base::trunc);
            os << content;
            return path;
        }

        static ref<TestTextAsset> load(const fs::path& path, uint32 flags = 0)
        {
            GameAssetsProvider provider;
            ref<TestTextAsset> asset = provider.load_game_asset_from_file<TestTextAsset>(path, true, {}, flags);
            asset->load_job.wait_for_completion();
            return asset;
        }

        fs::path directory;
    };
}

TEST_F(GameAssetTest, LoadedAssetHasDataAndSourcePath)
{
    const fs::path path = write_file("loaded.etest", "content");

    ref<TestTextAsset> asset = load(path);

    EXPECT_EQ(asset->load_state.load(), AssetLoadState::LOADED);
    EXPECT_EQ(asset->text, "content");
    EXPECT_EQ(asset->source_path, path.lexically_normal().make_preferred());
}

TEST_F(GameAssetTest, LoadedAssetIsReleasedWhenUnreferenced)
{
    const fs::path path = write_file("released.etest", "content");

    weakref<TestTextAsset> weak_asset = load(path);

    EXPECT_TRUE(weak_asset.expired());
}

TEST_F(GameAssetTest, CacheReturnsSameInstanceForSamePathAndFlags)
{
    const fs::path path = write_file("cached.etest", "content");

    ref<TestTextAsset> first = load(path);
    ref<TestTextAsset> second = load(directory / "." / "cached.etest");
    ref<TestTextAsset> other_flags = load(path, 1);

    EXPECT_EQ(first, second);
    EXPECT_NE(first, other_flags);
}

TEST_F(GameAssetTest, DifferentPathsAreNotSharedInCache)
{
    const fs::path first_path = write_file("first.etest", "first");
    const fs::path second_path = write_file("second.etest", "second");

    ref<TestTextAsset> first = load(first_path);
    ref<TestTextAsset> second = load(second_path);

    EXPECT_NE(first, second);
    EXPECT_EQ(first->text, "first");
    EXPECT_EQ(second->text, "second");
}

TEST_F(GameAssetTest, MissingFileFailsAndIsNotCached)
{
    const fs::path path = directory / "missing.etest";

    ref<TestTextAsset> first = load(path);
    EXPECT_EQ(first->load_state.load(), AssetLoadState::FAILED);

    write_file("missing.etest", "content");

    ref<TestTextAsset> second = load(path);
    EXPECT_NE(first, second);
    EXPECT_EQ(second->load_state.load(), AssetLoadState::LOADED);
}

TEST_F(GameAssetTest, DeserializationErrorFails)
{
    const fs::path path = write_file("invalid.etest", "invalid");

    ref<TestTextAsset> asset = load(path);

    EXPECT_EQ(asset->load_state.load(), AssetLoadState::FAILED);
    EXPECT_EQ(GameAssetCache::try_get_asset_from_cache(path, 0), nullptr);
}

TEST_F(GameAssetTest, RemovedAssetIsReloaded)
{
    const fs::path path = write_file("removed.etest", "content");

    ref<TestTextAsset> first = load(path);
    GameAssetCache::remove_asset_from_cache(path, 0);
    ref<TestTextAsset> second = load(path);

    EXPECT_NE(first, second);
}

TEST_F(GameAssetTest, SaveWritesSerializedAsset)
{
    TestTextAsset asset;
    asset.text = "saved";

    GameAssetsProvider provider;
    provider.save_game_asset_to_file_async(directory / "saved", &asset).wait_for_completion();

    ref<TestTextAsset> loaded = load(directory / "saved.etest");
    EXPECT_EQ(loaded->text, "saved");
}
