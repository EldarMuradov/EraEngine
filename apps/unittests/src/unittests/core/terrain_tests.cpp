#include <gtest/gtest.h>

#include <ecs/world.h>

#include <terrain/terrain.h>

#include <cmath>

namespace
{
    using namespace era_engine;

    constexpr float chunk_size = 8.0f;

    class TerrainTest : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            world = new World("TerrainTestWorld");
            world->init();

            Entity entity = world->create_entity("Terrain");
            terrain = entity.add_component<TerrainComponent>(2u, chunk_size, 10.0f, nullptr, nullptr, nullptr);
        }

        void TearDown() override
        {
            delete world;
        }

        // Ridge along global X = 128 (the chunk border): 0.61 m rise per meter on both sides.
        void fill_ridge()
        {
            const uint32 vertices_per_dim = TerrainComponent::get_vertices_per_chunk_dim();
            for (uint32 cz = 0; cz < 2; ++cz)
            {
                for (uint32 cx = 0; cx < 2; ++cx)
                {
                    std::vector<uint16>& heights = terrain->chunk(cx, cz).heights;
                    heights.resize(vertices_per_dim * vertices_per_dim);
                    for (uint32 z = 0; z < vertices_per_dim; ++z)
                    {
                        for (uint32 x = 0; x < vertices_per_dim; ++x)
                        {
                            const int32 global_x = int32(cx * (vertices_per_dim - 1) + x);
                            heights[z * vertices_per_dim + x] = uint16(40000 - 250 * std::abs(global_x - 128));
                        }
                    }
                }
            }
        }

        float height(uint32 global_x, uint32 global_z) const
        {
            const uint32 segments = TerrainComponent::get_vertices_per_chunk_dim() - 1;
            const uint32 cx = std::min(global_x / segments, 1u);
            const uint32 cz = std::min(global_z / segments, 1u);
            const uint32 x = global_x - cx * segments;
            const uint32 z = global_z - cz * segments;
            return float(terrain->chunk(cx, cz).heights[z * (segments + 1) + x]);
        }

        // Change of slope angle between [x - k, x] and [x, x + k], k = half a meter.
        float bend_degrees(uint32 global_x, uint32 global_z) const
        {
            const float spacing = chunk_size / float(TerrainComponent::get_vertices_per_chunk_dim() - 1);
            const uint32 k = uint32(0.5f / spacing);
            const float to_meters = terrain->amplitude_scale / 65535.0f;
            const float s0 = std::atan((height(global_x, global_z) - height(global_x - k, global_z)) * to_meters / (k * spacing));
            const float s1 = std::atan((height(global_x + k, global_z) - height(global_x, global_z)) * to_meters / (k * spacing));
            return std::abs(s1 - s0) * 180.0f / 3.14159265f;
        }

        World* world = nullptr;
        TerrainComponent* terrain = nullptr;
    };
}

TEST_F(TerrainTest, SmoothingRoundsSharpRidge)
{
    fill_ridge();
    const float bend_before = bend_degrees(128, 64);
    ASSERT_GT(bend_before, 60.0f);

    terrain->gen_settings.smoothing_sigma = 1.0f;
    terrain->smooth_heights();

    EXPECT_LT(bend_degrees(128, 64), 20.0f);

    EXPECT_NEAR(height(64, 64), 40000.0f - 250.0f * 64.0f, 1.0f);
    EXPECT_NEAR(height(192, 200), 40000.0f - 250.0f * 64.0f, 1.0f);
}

TEST_F(TerrainTest, SmoothingKeepsChunkBordersShared)
{
    fill_ridge();
    terrain->gen_settings.smoothing_sigma = 1.0f;
    terrain->smooth_heights();

    const uint32 vertices_per_dim = TerrainComponent::get_vertices_per_chunk_dim();
    for (uint32 i = 0; i < vertices_per_dim; ++i)
    {
        EXPECT_EQ(terrain->chunk(0, 0).heights[i * vertices_per_dim + vertices_per_dim - 1], terrain->chunk(1, 0).heights[i * vertices_per_dim]);
        EXPECT_EQ(terrain->chunk(0, 0).heights[(vertices_per_dim - 1) * vertices_per_dim + i], terrain->chunk(0, 1).heights[i]);
    }
}

TEST_F(TerrainTest, ZeroSigmaKeepsHeights)
{
    fill_ridge();
    const std::vector<uint16> before = terrain->chunk(0, 0).heights;

    terrain->gen_settings.smoothing_sigma = 0.0f;
    terrain->smooth_heights();

    EXPECT_EQ(terrain->chunk(0, 0).heights, before);
}
