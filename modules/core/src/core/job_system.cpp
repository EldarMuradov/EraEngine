// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "core/job_system.h"
#include "core/math.h"
#include "core/imgui.h"

namespace era_engine
{
    void JobQueue::initialize(uint32 num_threads, uint32 thread_offset, int thread_priority, const wchar* description)
    {
        queue = moodycamel::ConcurrentQueue<int32>(capacity);

        for (uint32 i = 0; i < num_threads; ++i)
        {
            std::thread thread([this, i]() { thread_func(i); });

            HANDLE handle = (HANDLE)thread.native_handle();
            SetThreadPriority(handle, thread_priority);

            uint64 affinityMask = 1ull << (i + thread_offset);
            SetThreadAffinityMask(handle, affinityMask);
            SetThreadDescription(handle, description);

            thread.detach();
        }
    }

    void JobQueue::add_continuation(int32 first, JobHandle second)
    {
        JobQueueEntry& first_job = all_jobs[first];
        //ASSERT(firstJob.continuation.index == -1);

        int32 unfinished = first_job.num_unfinished_jobs++;
        if (unfinished == 0)
        {
            // First job was finished before adding continuation -> just submit second.
            second.queue->submit(second.index);
        }
        else
        {
            // First job hadn't finished before -> add second as continuation and then finish first (which decrements numUnfinished again).
            first_job.continuation = second;
            finish_job(first);
        }
    }

    void JobQueue::submit(int32 handle)
    {
        if (handle != -1)
        {
            while (!queue.try_enqueue(handle))
            {
                execute_next_job();
            }

            ++running_jobs;

            wake_condition.notify_one();
        }
    }

    void JobQueue::wait_for_completion()
    {
        while (running_jobs)
        {
            execute_next_job();
        }
    }

    void JobQueue::wait_for_completion(int32 handle)
    {
        if (handle != -1)
        {
            JobQueueEntry& job = all_jobs[handle];

            while (job.num_unfinished_jobs > 0)
            {
                execute_next_job();
            }
        }
    }

    int32 JobQueue::allocate_job()
    {
        return (int32)(next_free_job++ & index_mask);
    }

    void JobQueue::finish_job(int32 handle)
    {
        JobQueueEntry& job = all_jobs[handle];
        int32 num_unfinished_jobs = --job.num_unfinished_jobs;
        ASSERT(num_unfinished_jobs >= 0);
        if (num_unfinished_jobs == 0)
        {
            --running_jobs;

            if (job.parent != -1)
            {
                finish_job(job.parent);
            }

            if (job.continuation.index != -1)
            {
                job.continuation.queue->submit(job.continuation.index);
            }
        }
    }

    bool JobQueue::execute_next_job()
    {
        int32 handle = -1;
        if (queue.try_dequeue(handle))
        {
            JobQueueEntry& job = all_jobs[handle];
            job.function(job.templated_function, job.data, { handle, this });

            finish_job(handle);

            return true;
        }

        return false;
    }

    void JobQueue::thread_func(int32 thread_index)
    {
        while (true)
        {
            if (!execute_next_job())
            {
                std::unique_lock<std::mutex> lock(wake_mutex);
                wake_condition.wait(lock);
            }
        }
    }

    void JobHandle::submit_now()
    {
        queue->submit(index);
    }

    void JobHandle::submit_after(JobHandle before)
    {
        before.queue->add_continuation(before.index, *this);
    }

    void JobHandle::wait_for_completion()
    {
        queue->wait_for_completion(index);
    }

    JobQueue high_priority_job_queue;
    JobQueue low_priority_job_queue;
    JobQueue main_thread_job_queue;

    void initialize_job_system()
    {
        HANDLE handle = GetCurrentThread();
        SetThreadAffinityMask(handle, 1);
        SetThreadPriority(handle, THREAD_PRIORITY_HIGHEST);
        CloseHandle(handle);

        high_priority_job_queue.initialize(4, 1, THREAD_PRIORITY_NORMAL, L"High priority worker");
        low_priority_job_queue.initialize(4, 5, THREAD_PRIORITY_BELOW_NORMAL, L"Low priority worker");
        main_thread_job_queue.initialize(0, 0, 0, 0);
    }

    void execute_main_thread_jobs()
    {
        main_thread_job_queue.wait_for_completion();
    }

}