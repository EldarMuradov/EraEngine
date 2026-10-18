#include <gtest/gtest.h>

#include <motion_matching/motion_matching_database.h>
#include <motion_matching/motion_matching_knn_structure.h>
#include <motion_matching/hnsw/hnsw_knn_structure.h>
#include <motion_matching/features/motion_matching_feature.h>
#include <motion_matching/features/motion_matching_feature_set.h>
#include <motion_matching/features/trajectory_feature.h>
#include <motion_matching/features/pose_feature.h>
#include <motion_matching/trajectory/trajectory_component.h>

#include <animation/animation_clip.h>
#include <animation/animation_clip_utils.h>

#include <ecs/world.h>
#include <ecs/base_components/transform_component.h>

#include <cmath>
#include <sstream>

namespace
{
    using namespace era_engine;
    using namespace era_engine::animation;

    constexpr float clip_sample_rate = 30.0f;

    ref<AnimationAssetClip> make_clip(float duration)
    {
        ClipInfo info;
        info.sample_rate = clip_sample_rate;
        info.num_samples = uint32(std::lround(duration * clip_sample_rate)) + 1;

        TrackInfo track;
        track.joint_id = "Root";
        track.joint_transforms.assign(info.num_samples, trs::identity);
        info.tracks.push_back(track);

        return AnimationAssetClipUtils::make_clip(info);
    }

    std::vector<float> synthetic_features(uint32 anim_index, float anim_position)
    {
        return {float(anim_index) * 4.0f, anim_position, std::cos(anim_position * 3.0f + float(anim_index))};
    }

    class SyntheticFeature final : public MotionMatchingFeature
    {
    public:
        uint32 get_feature_size() const override
        {
            return 3;
        }

        bool sample_animation(const Skeleton* skeleton, ref<AnimationAssetClip> clip, float sample_rate, std::vector<ref<MotionMatchingDatabase::Sample>>& out_samples) const override
        {
            for (const ref<MotionMatchingDatabase::Sample>& sample : out_samples)
            {
                const std::vector<float> values = synthetic_features(sample->anim_index, sample->anim_position);
                sample->features.insert(sample->features.end(), values.begin(), values.end());
            }
            return true;
        }
    };

    class ConstantFeature final : public MotionMatchingFeature
    {
    public:
        uint32 get_feature_size() const override
        {
            return 1;
        }

        bool sample_animation(const Skeleton* skeleton, ref<AnimationAssetClip> clip, float sample_rate, std::vector<ref<MotionMatchingDatabase::Sample>>& out_samples) const override
        {
            for (const ref<MotionMatchingDatabase::Sample>& sample : out_samples)
            {
                sample->features.push_back(7.0f);
            }
            return true;
        }
    };

    class MotionDatabaseTest : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            database = make_ref<MotionMatchingDatabase>();
            database->database_id = "UnitTestDatabase";
            database->sample_rate = 10.0f;
            database->max_broardphase_candidates = 4;
            database->narrow_phase_params.flags = NarrowPhaseFlags::NONE;

            first_clip = make_clip(1.0f);
            second_clip = make_clip(2.0f);
            first_clip->handle = AssetHandle(101);
            second_clip->handle = AssetHandle(202);

            database->add_animation(first_clip);
            database->add_animation(second_clip);
            database->features.push_back(new SyntheticFeature());
            database->features.push_back(new ConstantFeature());
        }

        void bake(uint32 search_dimension = 0)
        {
            database->search_dimension = search_dimension;
            database->weights = {1.0f, 1.0f, 1.0f, 1.0f};
            database->generate_initial_data();
            database->bake(nullptr);
        }

        std::vector<float> query_for(uint32 anim_index, float anim_position) const
        {
            std::vector<float> query = synthetic_features(anim_index, anim_position);
            query.push_back(7.0f);
            return query;
        }

        ref<MotionMatchingDatabase> database;
        ref<AnimationAssetClip> first_clip;
        ref<AnimationAssetClip> second_clip;
    };
}

TEST(MotionMatching_Clips, SyntheticClipDuration)
{
    const ref<AnimationAssetClip> clip = make_clip(1.5f);

    ASSERT_NE(clip, nullptr);
    EXPECT_NEAR(clip->get_duration(), 1.5f, 1e-4f);
    EXPECT_FLOAT_EQ(clip->get_sample_rate(), clip_sample_rate);
}

TEST_F(MotionDatabaseTest, GenerateInitialData)
{
    database->generate_initial_data();

    EXPECT_EQ(database->total_features_per_sample, 4u);
    ASSERT_EQ(database->animations_asset_handles.size(), 2u);
    EXPECT_EQ(database->animations_asset_handles[0], 101u);
    EXPECT_EQ(database->animations_asset_handles[1], 202u);
}

TEST_F(MotionDatabaseTest, BakeSamplesAnimations)
{
    bake();

    ASSERT_EQ(database->samples.size(), 30u);
    EXPECT_EQ(database->samples[0]->anim_index, 0u);
    EXPECT_FLOAT_EQ(database->samples[0]->anim_position, 0.0f);
    EXPECT_EQ(database->samples[10]->anim_index, 1u);
    EXPECT_NEAR(database->samples[29]->anim_position, 1.9f, 1e-4f);

    for (const ref<MotionMatchingDatabase::Sample>& sample : database->samples)
    {
        EXPECT_EQ(sample->features.size(), 4u);
    }

    EXPECT_EQ(database->search_dimension, 4u);
    EXPECT_FLOAT_EQ(database->min_values[0], 0.0f);
    EXPECT_FLOAT_EQ(database->max_values[0], 4.0f);
    EXPECT_NEAR(database->mean_values[0], 4.0f * 20.0f / 30.0f, 1e-4f);
    EXPECT_FLOAT_EQ(database->normalize_factors[0], 0.25f);
    EXPECT_FLOAT_EQ(database->normalize_factors[3], 0.0f);
}

TEST_F(MotionDatabaseTest, MissingWeightsDefaultToOne)
{
    database->generate_initial_data();
    database->weights.clear();
    database->bake(nullptr);

    ASSERT_EQ(database->weights.size(), 4u);
    for (float weight : database->weights)
    {
        EXPECT_FLOAT_EQ(weight, 1.0f);
    }
}

TEST_F(MotionDatabaseTest, NormalizeQuery)
{
    bake();

    const std::vector<float> normalized_mean = database->normalize_query(database->mean_values);
    for (float value : normalized_mean)
    {
        EXPECT_FLOAT_EQ(value, 0.0f);
    }

    const std::vector<float> query = query_for(1, 0.5f);
    const std::vector<float> normalized = database->normalize_query(query);

    ASSERT_EQ(normalized.size(), 4u);
    for (size_t i = 0; i < normalized.size(); ++i)
    {
        EXPECT_NEAR(normalized[i], (query[i] - database->mean_values[i]) * database->normalize_factors[i], 1e-5f);
    }
    EXPECT_FLOAT_EQ(normalized[3], 0.0f);
}

TEST_F(MotionDatabaseTest, ProjectionIsOrthonormal)
{
    bake(2);

    const MatrixX& projection = database->transform_matrix;
    ASSERT_EQ(projection.rows(), 2u);
    ASSERT_EQ(projection.cols(), 4u);

    for (uint32 i = 0; i < projection.rows(); ++i)
    {
        for (uint32 j = 0; j < projection.rows(); ++j)
        {
            float dot_product = 0.0f;
            for (uint32 k = 0; k < projection.cols(); ++k)
            {
                dot_product += projection(i, k) * projection(j, k);
            }
            EXPECT_NEAR(dot_product, i == j ? 1.0f : 0.0f, 1e-4f);
        }
    }

    const VectorX packed = database->pack_query(database->normalize_query(query_for(0, 0.3f)));
    EXPECT_EQ(packed.size(), 2u);
}

TEST_F(MotionDatabaseTest, SearchFindsExactSample)
{
    bake();

    for (uint32 anim_index = 0; anim_index < 2; ++anim_index)
    {
        for (float position : {0.0f, 0.3f, 0.7f})
        {
            SearchParams params;
            params.query = query_for(anim_index, position);

            const SearchResult result = database->search(params);

            ASSERT_EQ(result.type, SearchResultType::NEW_ANIMATION);
            EXPECT_EQ(result.animation, anim_index == 0 ? first_clip : second_clip);
            EXPECT_NEAR(result.anim_position, position, 1e-4f);
            EXPECT_EQ(result.database_id, "UnitTestDatabase");
            ASSERT_EQ(result.found_features.size(), params.query.size());
            for (size_t i = 0; i < params.query.size(); ++i)
            {
                EXPECT_NEAR(result.found_features[i], params.query[i], 1e-4f);
            }
        }
    }
}

TEST_F(MotionDatabaseTest, SearchWithPcaFindsExactSample)
{
    bake(3);

    SearchParams params;
    params.query = query_for(1, 1.2f);

    const SearchResult result = database->search(params);

    ASSERT_EQ(result.type, SearchResultType::NEW_ANIMATION);
    EXPECT_EQ(result.animation, second_clip);
    EXPECT_NEAR(result.anim_position, 1.2f, 1e-4f);
}

TEST_F(MotionDatabaseTest, SearchFindsNearestSample)
{
    bake();

    SearchParams params;
    params.query = query_for(0, 0.42f);

    const SearchResult result = database->search(params);

    ASSERT_EQ(result.type, SearchResultType::NEW_ANIMATION);
    EXPECT_EQ(result.animation, first_clip);
    EXPECT_NEAR(result.anim_position, 0.4f, 1e-4f);
}

TEST_F(MotionDatabaseTest, SearchRejectsWrongQuerySize)
{
    bake();

    SearchParams params;
    params.query = {1.0f, 2.0f};

    EXPECT_EQ(database->search(params).type, SearchResultType::FAILED);
}

TEST_F(MotionDatabaseTest, SearchIgnoresAnimationEnd)
{
    database->narrow_phase_params.animation_end_ignore_window = 0.35f;
    database->max_broardphase_candidates = 30;
    bake();

    SearchParams params;
    params.query = query_for(0, 0.9f);

    const SearchResult result = database->search(params);

    ASSERT_NE(result.type, SearchResultType::FAILED);
    EXPECT_LT(result.anim_position + 0.35f, result.animation->get_duration());
}

TEST_F(MotionDatabaseTest, SameFrameDetection)
{
    database->narrow_phase_params.flags = NarrowPhaseFlags::SAME_FRAME_CHECK;
    database->narrow_phase_params.same_frame_time_threshold = 0.15f;
    bake();

    SearchParams params;
    params.query = query_for(0, 0.5f);
    params.current_animation = first_clip;
    params.current_anim_position = 0.45f;

    const SearchResult same_frame = database->search(params);
    EXPECT_EQ(same_frame.type, SearchResultType::SAME_FRAME);
    EXPECT_EQ(same_frame.animation, first_clip);
    EXPECT_FLOAT_EQ(same_frame.anim_position, 0.45f);

    params.current_anim_position = 0.1f;
    const SearchResult same_animation = database->search(params);
    EXPECT_EQ(same_animation.type, SearchResultType::SAME_ANIMATION);
    EXPECT_NEAR(same_animation.anim_position, 0.5f, 1e-4f);

    params.current_animation = second_clip;
    const SearchResult new_animation = database->search(params);
    EXPECT_EQ(new_animation.type, SearchResultType::NEW_ANIMATION);
    EXPECT_EQ(new_animation.animation, first_clip);
}

TEST_F(MotionDatabaseTest, SameAnimationUsesAssetHandle)
{
    database->narrow_phase_params.flags = NarrowPhaseFlags::SAME_FRAME_CHECK;
    bake();

    ref<AnimationAssetClip> reloaded_clip = make_clip(1.0f);
    reloaded_clip->handle = first_clip->handle;

    SearchParams params;
    params.query = query_for(0, 0.5f);
    params.current_animation = reloaded_clip;
    params.current_anim_position = 0.5f;

    EXPECT_EQ(database->search(params).type, SearchResultType::SAME_FRAME);
}

TEST_F(MotionDatabaseTest, ClipsWithoutHandlesAreDistinguished)
{
    first_clip->handle = AssetHandle();
    second_clip->handle = AssetHandle();
    database->narrow_phase_params.flags = NarrowPhaseFlags::SAME_FRAME_CHECK;
    bake();

    SearchParams params;
    params.query = query_for(0, 0.5f);
    params.current_animation = second_clip;
    params.current_anim_position = 0.5f;

    EXPECT_EQ(database->search(params).type, SearchResultType::NEW_ANIMATION);
}

TEST_F(MotionDatabaseTest, EuclideanDistanceCheck)
{
    database->narrow_phase_params.flags = NarrowPhaseFlags::EUCLIDIAN_DISTANCE_CHECK;
    database->narrow_phase_params.max_euclidean_distance = 0.05f;
    bake();

    SearchParams params;
    params.query = query_for(0, 0.3f);
    EXPECT_EQ(database->search(params).type, SearchResultType::NEW_ANIMATION);

    params.query = query_for(0, 0.35f);
    EXPECT_EQ(database->search(params).type, SearchResultType::FAILED);
}

TEST_F(MotionDatabaseTest, StaticThresholds)
{
    database->narrow_phase_params.flags = NarrowPhaseFlags::STATIC_THRESHOLDS_CHECK;
    database->static_thresholds.resize(4);
    database->static_thresholds[0].enabled = true;
    database->static_thresholds[0].min_diff = -0.5f;
    database->static_thresholds[0].max_diff = 0.5f;
    database->max_broardphase_candidates = 30;
    bake();

    SearchParams params;
    params.query = query_for(1, 0.3f);
    params.query[0] = 0.2f;

    const SearchResult result = database->search(params);

    ASSERT_NE(result.type, SearchResultType::FAILED);
    EXPECT_EQ(result.animation, first_clip);
}

TEST_F(MotionDatabaseTest, DeserializeRejectsInvalidData)
{
    std::stringstream stream;
    stream << "not a motion database";

    MotionMatchingDatabase loaded;
    EXPECT_FALSE(loaded.deserialize(stream));
}

TEST_F(MotionDatabaseTest, Registry)
{
    static uint32 registry_test_index = 0;
    database->database_id = "UnitTestRegistryDatabase" + std::to_string(registry_test_index++);
    MotionDatabaseRegistry::add_to_registry(database);

    EXPECT_EQ(MotionDatabaseRegistry::get_by_id(database->database_id), database);
    EXPECT_EQ(MotionDatabaseRegistry::get_by_id("UnitTestMissingDatabase"), nullptr);
}

TEST(MotionMatching_Knn, HnswFindsNearestNeighbours)
{
    MotionMatchingDatabase database;
    database.search_dimension = 2;

    const uint32 grid_size = 10;
    MatrixX points(grid_size * grid_size, 2);
    for (uint32 x = 0; x < grid_size; ++x)
    {
        for (uint32 y = 0; y < grid_size; ++y)
        {
            const uint32 index = x * grid_size + y;
            points(index, 0) = float(x);
            points(index, 1) = float(y);

            ref<MotionMatchingDatabase::Sample> sample = make_ref<MotionMatchingDatabase::Sample>();
            sample->anim_index = index;
            database.samples.push_back(sample);
        }
    }

    HnswKnnStructure knn;
    knn.search_exploration_factor = 64;
    knn.build_structure_from_matrix(database, points);

    EXPECT_FALSE(knn.writable.empty());

    float query[2] = {3.2f, 6.9f};
    const std::vector<ref<MotionMatchingDatabase::Sample>> found = knn.search_knn(query, 2, 3, database);

    ASSERT_EQ(found.size(), 3u);
    EXPECT_EQ(found[0]->anim_index, 3u * grid_size + 7u);

    const auto distance = [&](const ref<MotionMatchingDatabase::Sample>& sample)
    {
        const float dx = points(sample->anim_index, 0) - query[0];
        const float dy = points(sample->anim_index, 1) - query[1];
        return dx * dx + dy * dy;
    };

    EXPECT_LE(distance(found[0]), distance(found[1]));
    EXPECT_LE(distance(found[1]), distance(found[2]));
}

TEST(MotionMatching_Knn, HnswSerializationRoundTrip)
{
    MotionMatchingDatabase database;
    database.search_dimension = 3;

    MatrixX points(50, 3);
    for (uint32 i = 0; i < points.rows(); ++i)
    {
        points(i, 0) = std::sin(float(i));
        points(i, 1) = std::cos(float(i) * 0.7f);
        points(i, 2) = float(i) * 0.1f;

        ref<MotionMatchingDatabase::Sample> sample = make_ref<MotionMatchingDatabase::Sample>();
        sample->anim_index = i;
        database.samples.push_back(sample);
    }

    HnswKnnStructure source;
    source.search_exploration_factor = 32;
    source.build_structure_from_matrix(database, points);

    std::stringstream stream;
    ASSERT_TRUE(source.serialize(stream, database));

    HnswKnnStructure loaded;
    loaded.writable = source.writable;
    ASSERT_TRUE(loaded.deserialize(stream, database));
    EXPECT_EQ(loaded.search_exploration_factor, 32u);

    for (uint32 i = 0; i < points.rows(); i += 7)
    {
        const auto expected = source.search_knn(points.row(i), 3, 1, database);
        const auto actual = loaded.search_knn(points.row(i), 3, 1, database);

        ASSERT_EQ(actual.size(), 1u);
        EXPECT_EQ(actual[0]->anim_index, i);
        EXPECT_EQ(actual[0], expected[0]);
    }
}

TEST(MotionMatching_Features, FeatureSet)
{
    MotionMatchingFeatureSet feature_set;

    feature_set.add_feature(rttr::type::get<TrajectoryFeature>(), {1.0f, 2.0f});
    feature_set.add_feature(rttr::type::get<PoseFeature>(), {3.0f});

    const std::vector<float> all_values = feature_set.get_all_feature_values();
    EXPECT_EQ(all_values, std::vector<float>({1.0f, 2.0f, 3.0f}));

    ASSERT_NE(feature_set.get_feature_values<PoseFeature>(), nullptr);
    EXPECT_EQ(*feature_set.get_feature_values<PoseFeature>(), std::vector<float>({3.0f}));
    EXPECT_EQ(feature_set.get_feature_values<MotionMatchingFeature>(), nullptr);

    const std::vector<rttr::type> types = feature_set.get_feature_types();
    ASSERT_EQ(types.size(), 2u);
    EXPECT_EQ(types[0], rttr::type::get<TrajectoryFeature>());

    feature_set.clear();
    EXPECT_TRUE(feature_set.get_all_feature_values().empty());
}

TEST(MotionMatching_Features, TrajectoryFeatureSize)
{
    TrajectoryFeature feature;
    EXPECT_EQ(feature.get_feature_size(), 0u);

    for (int32 i = 0; i < 3; ++i)
    {
        feature.descriptors.push_back(make_ref<TrajectoryFeatureDesc>());
    }

    EXPECT_EQ(feature.get_feature_size(), 6u);
    EXPECT_EQ(feature.get_descriptors().size(), 3u);
}

TEST(MotionMatching_Features, TrajectoryFeatureComputesLocalValues)
{
    World* world = new World("GameWorld");
    world->init();

    Entity entity = world->create_entity("Character");
    entity.get_component<TransformComponent>()->set_world_transform(trs(vec3(10.0f, 0.0f, 5.0f), quat(vec3(0.0f, 1.0f, 0.0f), M_PI_OVER_2)));

    TrajectoryComponent* trajectory = entity.add_component<TrajectoryComponent>();
    ASSERT_EQ(trajectory->number_of_trajectories, 5u);
    trajectory->time_offsets = {0.0f, 0.2f, 0.4f, 0.6f, 0.8f};
    trajectory->trajectory_positions[2] = vec3(12.0f, 0.0f, 5.0f);
    trajectory->trajectory_rotations[2] = quat(vec3(0.0f, 1.0f, 0.0f), M_PI_OVER_2);

    TrajectoryFeature feature;

    ref<TrajectoryFeatureDesc> location = make_ref<TrajectoryFeatureDesc>();
    location->type = FeatureDescType::LOCATION;
    location->time_offset = 0.4f;
    feature.descriptors.push_back(location);

    ref<TrajectoryFeatureDesc> direction = make_ref<TrajectoryFeatureDesc>();
    direction->type = FeatureDescType::DIRECTION;
    direction->time_offset = 0.4f;
    feature.descriptors.push_back(direction);

    ref<TrajectoryFeatureDesc> missing = make_ref<TrajectoryFeatureDesc>();
    missing->type = FeatureDescType::LOCATION;
    missing->time_offset = 1.5f;
    feature.descriptors.push_back(missing);

    FeatureComputationContext context;
    context.world = world;
    context.entity = entity;
    context.trajectory_component = trajectory;
    context.dt = 1.0f / 60.0f;

    const std::vector<float> values = feature.compute_features(context);

    ASSERT_EQ(values.size(), 6u);
    EXPECT_NEAR(values[0], 0.0f, 1e-4f);
    EXPECT_NEAR(values[1], 2.0f, 1e-4f);
    EXPECT_NEAR(values[2], 0.0f, 1e-4f);
    EXPECT_NEAR(values[3], 1.0f, 1e-4f);
    EXPECT_FLOAT_EQ(values[4], 0.0f);
    EXPECT_FLOAT_EQ(values[5], 0.0f);

    delete world;
}
