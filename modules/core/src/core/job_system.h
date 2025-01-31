// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "core_api.h"

#include <concurrentqueue/concurrentqueue.h>

namespace era_engine
{
    struct ERA_CORE_API JobHandle
    {
        void submit_now();
        void submit_after(JobHandle before);
        void wait_for_completion();

        int32 index = -1;
        struct JobQueue* queue;
    };

    template <typename Data_>
    using JobFunction = void (*)(Data_&, JobHandle);

    struct JobQueue
    {
        struct JobQueueEntry
        {
            void (*function)(void*, void*, JobHandle);
            void* templated_function;

            std::atomic<int32> num_unfinished_jobs;
            int32 parent;
            JobHandle continuation;

            static constexpr uint64 SIZE = sizeof(function) + sizeof(templated_function) + sizeof(num_unfinished_jobs) + sizeof(parent) + sizeof(continuation);
            static constexpr uint64 DATA_SIZE = (3 * 64) - SIZE;

            uint8 data[DATA_SIZE];
        };

        template<typename T_>
        using ValidJobDataType = std::enable_if_t<sizeof(T_) <= JobQueueEntry::DATA_SIZE, bool>;

        static_assert(sizeof(JobQueueEntry) % 64 == 0);

        void initialize(uint32 num_threads, uint32 thread_offset, int thread_priority, const wchar* description);

        template <typename Data_,
            ValidJobDataType<Data_> = true>
        JobHandle createJob(JobFunction<Data_> function, const Data_& data, JobHandle parent = {})
        {
            int32 handle = allocate_job();
            auto& job = all_jobs[handle];
            job.num_unfinished_jobs = 1;
            job.parent = parent.index;
            job.continuation.index = -1;

            if (parent.index != -1)
            {
                ++all_jobs[parent.index].num_unfinished_jobs;
            }

            job.templated_function = function;
            job.function = [](void* templated_function, void* raw_data, JobHandle job)
                {
                    Data_& data = *(Data_*)raw_data;

                    auto function = (JobFunction<Data_>)templated_function;
                    function(data, job);

                    data.~Data_();
                };

            new(job.data) Data_(data);

            return JobHandle{ handle, this };
        }

        void wait_for_completion();

    private:
        friend struct JobHandle;

        void add_continuation(int32 first, JobHandle second);
        void submit(int32 handle);
        void wait_for_completion(int32 handle);

        int32 allocate_job();
        void finish_job(int32 handle);
        bool execute_next_job();
        void thread_func(int32 thread_index);

        moodycamel::ConcurrentQueue<int32> queue;
        std::atomic<uint32> running_jobs = 0;

        static constexpr uint32 capacity = 4096;
        static constexpr uint32 index_mask = capacity - 1;

        JobQueueEntry all_jobs[capacity];
        std::atomic<uint32> next_free_job = 0;

        std::condition_variable wake_condition;
        std::mutex wake_mutex;
    };

    extern JobQueue high_priority_job_queue;
    extern JobQueue low_priority_job_queue;
    extern JobQueue main_thread_job_queue;

    void initialize_job_system();
    void execute_main_thread_jobs();
}