#include <gtest/gtest.h>

#include <core/job_system.h>

namespace
{
    using namespace era_engine;
}

TEST(Core_JobSystem, DefaultJobReleasesCapturedData)
{
    ref<int32> value = make_ref<int32>(0);

    DefaultJob job_object;
    job_object.job_func = [value]()
    {
        *value = 42;
    };

    JobHandle job = schedule_low_priority_job(job_object);
    job_object = {};
    job.submit_now();
    job.wait_for_completion();

    EXPECT_EQ(*value, 42);
    EXPECT_EQ(value.use_count(), 1);
}

TEST(Core_JobSystem, DefaultHandleIsNoOp)
{
    JobHandle job;
    job.submit_now();
    job.wait_for_completion();

    SUCCEED();
}
