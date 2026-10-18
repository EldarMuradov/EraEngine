#include <gtest/gtest.h>

#include <ecs/world.h>
#include <ecs/system.h>
#include <ecs/update_groups.h>
#include <ecs/world_system_scheduler.h>

#include <rttr/policy.h>
#include <rttr/registration>

#include <algorithm>
#include <chrono>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace
{
    using namespace era_engine;

    class ScheduleLog
    {
    public:
        static void record(const std::string& entry)
        {
            std::lock_guard<std::mutex> lock(mutex);
            entries.push_back(entry);
        }

        static std::vector<std::string> get()
        {
            std::lock_guard<std::mutex> lock(mutex);
            return entries;
        }

        static void clear()
        {
            std::lock_guard<std::mutex> lock(mutex);
            entries.clear();
        }

        static size_t count(const std::string& entry)
        {
            std::lock_guard<std::mutex> lock(mutex);
            return std::count(entries.begin(), entries.end(), entry);
        }

        static ptrdiff_t index_of(const std::string& entry)
        {
            std::lock_guard<std::mutex> lock(mutex);
            const auto iter = std::find(entries.begin(), entries.end(), entry);
            return iter == entries.end() ? -1 : std::distance(entries.begin(), iter);
        }

    private:
        static inline std::mutex mutex;
        static inline std::vector<std::string> entries;
    };

    class UnitTestOmegaSystem final : public System
    {
    public:
        UnitTestOmegaSystem(World* _world)
            : System(_world) {}

        void init() override
        {
            ScheduleLog::record("Omega::init");
        }

        void input(float dt)
        {
            ScheduleLog::record("Omega::input");
        }

        void update(float dt)
        {
            last_dt = dt;
            ScheduleLog::record("Omega::update");
        }

        void render(float dt)
        {
            ScheduleLog::record("Omega::render");
        }

        void fixed_update(float dt)
        {
            last_fixed_dt = dt;
            ScheduleLog::record("Omega::fixed_update");
        }

        static inline float last_dt = 0.0f;
        static inline float last_fixed_dt = 0.0f;

        ERA_VIRTUAL_REFLECT(System)
    };

    class UnitTestAlphaSystem final : public System
    {
    public:
        UnitTestAlphaSystem(World* _world)
            : System(_world) {}

        void update(float dt)
        {
            ScheduleLog::record("Alpha::update");
        }

        ERA_VIRTUAL_REFLECT(System)
    };

    class UnitTestZetaSystem final : public System
    {
    public:
        UnitTestZetaSystem(World* _world)
            : System(_world) {}

        void update(float dt)
        {
            ScheduleLog::record("Zeta::update");
        }

        ERA_VIRTUAL_REFLECT(System)
    };

    class UnitTestConcurrentSystem final : public System
    {
    public:
        UnitTestConcurrentSystem(World* _world)
            : System(_world) {}

        void root(float dt)
        {
            ScheduleLog::record("Concurrent::root");
        }

        void left(float dt)
        {
            ScheduleLog::record("Concurrent::left");
        }

        void right(float dt)
        {
            ScheduleLog::record("Concurrent::right");
        }

        void join(float dt)
        {
            ScheduleLog::record("Concurrent::join");
        }

        ERA_VIRTUAL_REFLECT(System)
    };

    class UnitTestOptionalSystem final : public System
    {
    public:
        UnitTestOptionalSystem(World* _world)
            : System(_world) {}

        void update(float dt)
        {
            ScheduleLog::record("Optional::update");
        }

        ERA_VIRTUAL_REFLECT(System)
    };

    // Overwrites the type list passed to initialize_systems, like a lazily registered RTTR type reallocating the type storage.
    class UnitTestMutatingSystem final : public System
    {
    public:
        UnitTestMutatingSystem(World* _world)
            : System(_world)
        {
            if (types_to_mutate != nullptr)
            {
                std::fill(types_to_mutate->begin(), types_to_mutate->end(), rttr::type::get<int>());
            }
        }

        static inline std::vector<rttr::type>* types_to_mutate = nullptr;

        ERA_VIRTUAL_REFLECT(System)
    };

    const std::string unittest_tag = "unittest";
    const std::string optional_tag = "unittest_optional";

    class SchedulerTest : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            ScheduleLog::clear();

            world = new World("UnitTestSchedulerWorld");
            world->init();
            world->add_tag(unittest_tag);

            scheduler = world->get_system_scheduler();
            scheduler->set_fixed_update_rate(0.001);
        }

        void TearDown() override
        {
            delete world;
        }

        void initialize(std::vector<rttr::type> types)
        {
            scheduler->initialize_systems(rttr::array_range<rttr::type>(types.data(), types.size()));
            scheduler->initialize_all_systems();
        }

        World* world = nullptr;
        WorldSystemScheduler* scheduler = nullptr;
    };
}

RTTR_REGISTRATION
{
    using namespace rttr;

    registration::class_<UnitTestOmegaSystem>("UnitTestOmegaSystem")
        .constructor<World*>()(policy::ctor::as_raw_ptr, metadata("Tag", unittest_tag))
        .method("input", &UnitTestOmegaSystem::input)(metadata("update_group", update_types::INPUT))
        .method("update", &UnitTestOmegaSystem::update)(metadata("update_group", update_types::GAMEPLAY_NORMAL))
        .method("render", &UnitTestOmegaSystem::render)(metadata("update_group", update_types::RENDER))
        .method("fixed_update", &UnitTestOmegaSystem::fixed_update)(metadata("update_group", update_types::PHYSICS));

    registration::class_<UnitTestAlphaSystem>("UnitTestAlphaSystem")
        .constructor<World*>()(policy::ctor::as_raw_ptr, metadata("Tag", unittest_tag))
        .method("update", &UnitTestAlphaSystem::update)(metadata("update_group", update_types::GAMEPLAY_NORMAL),
            metadata("After", std::vector<std::string>{"UnitTestOmegaSystem::update"}));

    registration::class_<UnitTestZetaSystem>("UnitTestZetaSystem")
        .constructor<World*>()(policy::ctor::as_raw_ptr, metadata("Tag", unittest_tag))
        .method("update", &UnitTestZetaSystem::update)(metadata("update_group", update_types::GAMEPLAY_NORMAL),
            metadata("Before", std::string("UnitTestOmegaSystem::update")));

    registration::class_<UnitTestConcurrentSystem>("UnitTestConcurrentSystem")
        .constructor<World*>()(policy::ctor::as_raw_ptr, metadata("Tag", unittest_tag))
        .method("root", &UnitTestConcurrentSystem::root)(metadata("update_group", update_types::GAMEPLAY_NORMAL_CONCURRENT))
        .method("left", &UnitTestConcurrentSystem::left)(metadata("update_group", update_types::GAMEPLAY_NORMAL_CONCURRENT),
            metadata("After", std::string("UnitTestConcurrentSystem::root")))
        .method("right", &UnitTestConcurrentSystem::right)(metadata("update_group", update_types::GAMEPLAY_NORMAL_CONCURRENT),
            metadata("After", std::string("UnitTestConcurrentSystem::root")))
        .method("join", &UnitTestConcurrentSystem::join)(metadata("update_group", update_types::GAMEPLAY_NORMAL_CONCURRENT),
            metadata("After", std::vector<std::string>{"UnitTestConcurrentSystem::left", "UnitTestConcurrentSystem::right"}));

    registration::class_<UnitTestMutatingSystem>("UnitTestMutatingSystem")
        .constructor<World*>()(policy::ctor::as_raw_ptr, metadata("Tag", unittest_tag));

    registration::class_<UnitTestOptionalSystem>("UnitTestOptionalSystem")
        .constructor<World*>()(policy::ctor::as_raw_ptr, metadata("Tag", optional_tag))
        .method("update", &UnitTestOptionalSystem::update)(metadata("update_group", update_types::GAMEPLAY_NORMAL));
}

TEST_F(SchedulerTest, InitIsCalledOnce)
{
    initialize({rttr::type::get<UnitTestOmegaSystem>()});

    EXPECT_EQ(ScheduleLog::count("Omega::init"), 1u);

    scheduler->initialize_all_systems();

    EXPECT_EQ(ScheduleLog::count("Omega::init"), 1u);
}

TEST_F(SchedulerTest, UpdateGroupsRunInOrder)
{
    initialize({rttr::type::get<UnitTestOmegaSystem>()});
    ScheduleLog::clear();

    scheduler->update_normal(0.25f);

    const std::vector<std::string> expected = {"Omega::input", "Omega::update", "Omega::render"};
    EXPECT_EQ(ScheduleLog::get(), expected);
    EXPECT_FLOAT_EQ(UnitTestOmegaSystem::last_dt, 0.25f);
}

TEST_F(SchedulerTest, AfterAndBeforeDependencies)
{
    initialize({rttr::type::get<UnitTestAlphaSystem>(), rttr::type::get<UnitTestOmegaSystem>(), rttr::type::get<UnitTestZetaSystem>()});

    for (int32 frame = 0; frame < 3; ++frame)
    {
        ScheduleLog::clear();
        scheduler->update_normal(0.016f);

        const ptrdiff_t zeta = ScheduleLog::index_of("Zeta::update");
        const ptrdiff_t omega = ScheduleLog::index_of("Omega::update");
        const ptrdiff_t alpha = ScheduleLog::index_of("Alpha::update");

        ASSERT_GE(zeta, 0);
        ASSERT_GE(omega, 0);
        ASSERT_GE(alpha, 0);
        EXPECT_LT(zeta, omega);
        EXPECT_LT(omega, alpha);
    }
}

TEST_F(SchedulerTest, ConcurrentGroupRespectsDependencies)
{
    initialize({rttr::type::get<UnitTestConcurrentSystem>()});

    for (int32 frame = 0; frame < 20; ++frame)
    {
        ScheduleLog::clear();
        scheduler->update_normal(0.016f);

        ASSERT_EQ(ScheduleLog::get().size(), 4u);

        const ptrdiff_t root = ScheduleLog::index_of("Concurrent::root");
        const ptrdiff_t left = ScheduleLog::index_of("Concurrent::left");
        const ptrdiff_t right = ScheduleLog::index_of("Concurrent::right");
        const ptrdiff_t join = ScheduleLog::index_of("Concurrent::join");

        EXPECT_EQ(root, 0);
        EXPECT_GT(left, root);
        EXPECT_GT(right, root);
        EXPECT_EQ(join, 3);
    }
}

TEST_F(SchedulerTest, SystemsWithMissingTagDoNotRun)
{
    initialize({rttr::type::get<UnitTestOmegaSystem>(), rttr::type::get<UnitTestOptionalSystem>()});

    scheduler->update_normal(0.016f);

    EXPECT_EQ(ScheduleLog::count("Omega::update"), 1u);
    EXPECT_EQ(ScheduleLog::count("Optional::update"), 0u);
}

TEST_F(SchedulerTest, TagAddedBeforeInitialization)
{
    world->add_tag(optional_tag);

    initialize({rttr::type::get<UnitTestOptionalSystem>()});

    scheduler->update_normal(0.016f);

    EXPECT_EQ(ScheduleLog::count("Optional::update"), 1u);
}

TEST_F(SchedulerTest, TagAddedAfterInitialization)
{
    initialize({rttr::type::get<UnitTestOptionalSystem>()});

    scheduler->update_normal(0.016f);
    EXPECT_EQ(ScheduleLog::count("Optional::update"), 0u);

    world->add_tag(optional_tag);
    scheduler->update_normal(0.016f);

    EXPECT_EQ(ScheduleLog::count("Optional::update"), 1u);
}

TEST_F(SchedulerTest, TagRemovedAfterInitialization)
{
    world->add_tag(optional_tag);

    initialize({rttr::type::get<UnitTestOptionalSystem>()});

    scheduler->update_normal(0.016f);
    EXPECT_EQ(ScheduleLog::count("Optional::update"), 1u);

    world->remove_tag(optional_tag);
    scheduler->update_normal(0.016f);

    EXPECT_EQ(ScheduleLog::count("Optional::update"), 1u);
}

TEST_F(SchedulerTest, FixedUpdateRunsFixedGroups)
{
    initialize({rttr::type::get<UnitTestOmegaSystem>()});
    ScheduleLog::clear();

    scheduler->update_normal(0.016f);
    EXPECT_EQ(ScheduleLog::count("Omega::fixed_update"), 0u);

    const uint64 frame_id = world->get_fixed_frame_id();

    scheduler->update_fixed(0.5f);

    EXPECT_EQ(ScheduleLog::count("Omega::fixed_update"), 1u);
    EXPECT_EQ(ScheduleLog::count("Omega::update"), 1u);
    EXPECT_FLOAT_EQ(UnitTestOmegaSystem::last_fixed_dt, 0.5f);
    EXPECT_EQ(world->get_fixed_frame_id(), frame_id + 1);
    EXPECT_EQ(scheduler->get_stats().fixed_steps, 1u);

    scheduler->reset_stats();

    EXPECT_EQ(scheduler->get_stats().fixed_steps, 0u);
}

TEST_F(SchedulerTest, FixedUpdateRate)
{
    scheduler->set_fixed_update_rate(60.0);

    EXPECT_DOUBLE_EQ(scheduler->get_fixed_update_rate(), 60.0);
    EXPECT_FLOAT_EQ(world->get_fixed_update_dt(), 1.0f / 60.0f);

    scheduler->set_fixed_update_rate(0.0);
    scheduler->set_fixed_update_rate(-10.0);

    EXPECT_DOUBLE_EQ(scheduler->get_fixed_update_rate(), 60.0);
    EXPECT_FLOAT_EQ(world->get_fixed_update_dt(), 1.0f / 60.0f);
}

TEST_F(SchedulerTest, PumpedFixedStepsAreCapped)
{
    initialize({rttr::type::get<UnitTestOmegaSystem>()});

    EXPECT_EQ(scheduler->get_fixed_step_mode(), FixedStepMode::MAIN_THREAD_PUMP);

    scheduler->set_fixed_update_rate(1000.0);
    scheduler->set_max_fixed_steps_per_wakeup(2);
    scheduler->set_fixed_step_mode(FixedStepMode::MAIN_THREAD_PUMP);
    scheduler->reset_stats();
    ScheduleLog::clear();

    EXPECT_EQ(scheduler->pump_fixed_steps(), 0u);

    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    EXPECT_EQ(scheduler->pump_fixed_steps(), 2u);
    EXPECT_EQ(ScheduleLog::count("Omega::fixed_update"), 2u);

    const WorldSystemScheduler::Stats stats = scheduler->get_stats();
    EXPECT_EQ(stats.fixed_steps, 2u);
    EXPECT_GE(stats.fixed_overruns, 1u);
    EXPECT_GE(stats.dropped_fixed_steps, 1u);
}

TEST_F(SchedulerTest, DescribeSchedule)
{
    EXPECT_NE(scheduler->describe_schedule(UpdateType::NORMAL).find("not built yet"), std::string::npos);

    initialize({rttr::type::get<UnitTestOmegaSystem>(), rttr::type::get<UnitTestAlphaSystem>()});

    const std::string normal = scheduler->describe_schedule(UpdateType::NORMAL);
    EXPECT_NE(normal.find("UnitTestOmegaSystem::update"), std::string::npos);
    EXPECT_NE(normal.find("UnitTestAlphaSystem::update"), std::string::npos);
    EXPECT_EQ(normal.find("UnitTestOmegaSystem::fixed_update"), std::string::npos);

    const std::string fixed = scheduler->describe_schedule(UpdateType::FIXED);
    EXPECT_NE(fixed.find("UnitTestOmegaSystem::fixed_update"), std::string::npos);
}

TEST_F(SchedulerTest, StoppedSchedulerDoesNotUpdate)
{
    initialize({rttr::type::get<UnitTestOmegaSystem>()});
    ScheduleLog::clear();

    EXPECT_TRUE(scheduler->is_running());

    scheduler->stop();

    EXPECT_FALSE(scheduler->is_running());

    scheduler->update_normal(0.016f);
    scheduler->update_fixed(0.016f);

    EXPECT_TRUE(ScheduleLog::get().empty());
}

TEST_F(SchedulerTest, GroupOverlap)
{
    const std::string group_name = update_types::GAMEPLAY_NORMAL_CONCURRENT.name;

    EXPECT_FALSE(scheduler->get_group_overlap(group_name));

    scheduler->set_group_overlap(group_name, true);
    EXPECT_TRUE(scheduler->get_group_overlap(group_name));

    scheduler->set_group_overlap(group_name, false);
    EXPECT_FALSE(scheduler->get_group_overlap(group_name));

    scheduler->set_overlapping_groups({group_name});
    EXPECT_TRUE(scheduler->get_group_overlap(group_name));

    scheduler->set_overlapping_groups({});
    EXPECT_FALSE(scheduler->get_group_overlap(group_name));
}

TEST(ECS_UpdateGroups, DefaultGroupsAreRegistered)
{
    World* world = new World("GameWorld");

    const UpdateGroup* gameplay = find_group("GAMEPLAY_NORMAL");
    ASSERT_NE(gameplay, nullptr);
    EXPECT_EQ(gameplay->update_type, UpdateType::NORMAL);
    EXPECT_TRUE(gameplay->main_thread);

    const UpdateGroup* concurrent = find_group("GAMEPLAY_NORMAL_CONCURRENT");
    ASSERT_NE(concurrent, nullptr);
    EXPECT_FALSE(concurrent->main_thread);

    const UpdateGroup* physics = find_group("PHYSICS");
    ASSERT_NE(physics, nullptr);
    EXPECT_EQ(physics->update_type, UpdateType::FIXED);

    EXPECT_EQ(find_group("UNITTEST_MISSING_GROUP"), nullptr);

    const std::vector<std::string>& order = UpdatesHolder::update_order;
    const auto position = [&order](const char* name)
    {
        return std::distance(order.begin(), std::find(order.begin(), order.end(), std::string(name)));
    };

    EXPECT_LT(position("INPUT"), position("GAMEPLAY_NORMAL"));
    EXPECT_LT(position("GAMEPLAY_NORMAL"), position("GAMEPLAY_NORMAL_CONCURRENT"));
    EXPECT_LT(position("GAMEPLAY_NORMAL_CONCURRENT"), position("RENDER"));
    EXPECT_LT(position("BEFORE_PHYSICS"), position("PHYSICS"));
    EXPECT_LT(position("PHYSICS"), position("AFTER_PHYSICS"));
    EXPECT_LT(position("END"), static_cast<ptrdiff_t>(order.size()));

    delete world;
}

TEST_F(SchedulerTest, TypeRangeChangesDuringInitializationAreIgnored)
{
    std::vector<rttr::type> types = {rttr::type::get<UnitTestMutatingSystem>(), rttr::type::get<UnitTestOmegaSystem>()};

    UnitTestMutatingSystem::types_to_mutate = &types;
    scheduler->initialize_systems(rttr::array_range<rttr::type>(types.data(), types.size()));
    UnitTestMutatingSystem::types_to_mutate = nullptr;

    scheduler->initialize_all_systems();

    EXPECT_EQ(ScheduleLog::count("Omega::init"), 1u);
}
