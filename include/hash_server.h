#pragma once
#include <gflags/gflags.h>
#include "chained_hash_rt.h"
#include "hash_request.h"
#include <verona.h>

DEFINE_uint64(num_ops, 100000, "the number of insert operations");

class BatchTracker
{
public:
    explicit BatchTracker(size_t size) : remaining_(size) {}
    void on_schedule() {}
    void on_complete() { remaining_--; }
    bool is_complete() const { return remaining_ == 0; }

private:
    std::atomic<size_t> remaining_;
};

class HashTableServer
{
private:
    static constexpr size_t BATCH_SIZE = 1000;
    static constexpr size_t MAX_PENDING_BATCHES = 3;

    cown_ptr<ExtendibleHash> hash_table;
    std::atomic<bool> running{true};
    std::atomic<size_t> completed_ops{0};
    size_t target_ops{FLAGS_num_ops};

    std::queue<HashRequest> request_queue;

    std::mutex queue_mutex;
    std::condition_variable cv;

    std::vector<std::thread> external_threads; // 改用vector存储
    std::atomic<size_t> next_thread_id{0};
    size_t scheduler_core_start;
    size_t scheduler_core_count;

public:
    HashTableServer() : hash_table(make_cown<ExtendibleHash>(HASH_INIT_BUCKET_NUM, HASH_ASSOC_NUM)) {}

    template <typename F>
    void external_thread(F &&f)
    {
        size_t thread_id = next_thread_id.fetch_add(1);

        external_threads.emplace_back([this, thread_id, f = std::forward<F>(f)]()
                                      {
            size_t core_id = scheduler_core_start + (thread_id % scheduler_core_count);
            cpu::set_affinity(core_id);
            f(); });
    }

    void start(size_t cores)
    {
        auto &sched = verona::rt::Scheduler::get();
        sched.init(cores);

        when(hash_table) << [this](acquired_cown<ExtendibleHash> hash_table_acq)
        {
            verona::rt::ThreadPool<verona::rt::SchedulerThread>::add_external_event_source();
        };

        sched.run();
        // while(!sched.has_started()) {
        //     std::this_thread::yield();
        // }

        external_thread([this]()
                        { scheduler_loop(); });
    }

    ~HashTableServer()
    {
        running = false;

        for (auto &thread : external_threads)
        {
            if (thread.joinable())
            {
                thread.join();
            }
        }
    }

    bool is_running() const
    {
        return running && completed_ops < target_ops;
    }

    void handle_insert(const char *key, const char *value, size_t client_id, uint64_t &work_usec);

private:
    void scheduler_loop()
    {
        std::vector<HashRequest> current_batch;

        while (is_running())
        {
            // 收集请求形成批次
            {
                std::unique_lock<std::mutex> lock(queue_mutex);
                while (!request_queue.empty() &&
                       current_batch.size() < BATCH_SIZE)
                {
                    current_batch.push_back(request_queue.front());
                    request_queue.pop();
                }
            }

            if (!current_batch.empty())
            {
                // 创建when语句
                when(hash_table) << [this, batch = std::move(current_batch)](acquired_cown<ExtendibleHash> hash_table_acq)
                {
                    for (auto req : batch)
                    {
                        hash_table_acq->insert(req.key, req.value, req.work_usec);
                        completed_ops.fetch_add(1);
                    }
                };

                current_batch.clear();
            }

            // 避免空转
            if (current_batch.empty())
            {
                std::this_thread::yield();
            }
        }
    }
};

void HashTableServer::handle_insert(const char *key, const char *value, size_t client_id, uint64_t &work_usec)
{
    HashRequest request{key, value, client_id, work_usec};
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        request_queue.push(request);
    }
    cv.notify_one();
}