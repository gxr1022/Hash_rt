#pragma once
#include <gflags/gflags.h>
#include "chained_hash_rt.h"
#include "hash_request.h"
#include <verona.h>
#include "spsc.h"

#define QUEUE_SIZE 1048576  // 2^20, closest power of 2 above 1024000

DEFINE_uint64(num_ops, 2000000, "the number of insert operations");
DEFINE_uint64(str_key_size, 8, "size of key (bytes)");
DEFINE_uint64(str_value_size, 100, "size of value (bytes)");
DEFINE_uint64(num_threads, 8, "the number of threads");
DEFINE_uint64(time_interval, 10, "the time interval of insert operations");
DEFINE_string(report_prefix, "[report]: ", "prefix of report data");
DEFINE_bool(first_mode, true, "fist mode start multiply clients on the same key value server");
DEFINE_uint64(work_usec, 0, "the time interval of insert operations");
DEFINE_uint64(batch_size, 1000, "the size of batch");

void standard_report(const std::string &prefix, const std::string &name, const std::string &value)
{
    std::cout << FLAGS_report_prefix << prefix + "_" << name << " : " << value << std::endl;
}

void benchmark_report(const std::string benchmark_prefix, const std::string &name, const std::string &value)
{
    standard_report(benchmark_prefix, name, value);
}

class HashTableServer
{
public:
    size_t MAX_BATCH_SIZE;
    std::atomic<size_t> current_batch_id{0};

    SystematicTestHarness &harness;
    std::atomic<bool> running{true};
    std::atomic<size_t> completed_ops{0};
    size_t target_ops;

    ExtendibleHash *hash_table;

    // Fixed-size array for client queues
    static constexpr size_t MAX_CLIENTS = 20; // Or other suitable size
    std::array<SPSCQueue<HashRequest, QUEUE_SIZE> *, MAX_CLIENTS> client_queues{};
    std::atomic<bool> active_clients[MAX_CLIENTS]{}; // Track active clients

    struct Batch
    {
        std::vector<HashRequest> requests;
        size_t id;
        std::atomic<size_t> remaining_requests{0};

        Batch(size_t batch_id) : id(batch_id) {}
    };

    std::atomic<Batch *> current_batch{nullptr};

    Batch *prepare_new_batch()
    {
        auto *batch = new Batch(current_batch_id.fetch_add(1));
        
        static size_t start_idx = 0;
        size_t current_idx = start_idx;
        size_t clients_checked = 0;

        while (batch->requests.size() < MAX_BATCH_SIZE && clients_checked < MAX_CLIENTS)
        {
            if (active_clients[current_idx])
            {
                auto *queue = client_queues[current_idx];
                if (queue)
                {
                    HashRequest req;
                    if (queue->try_pop(req))
                    {
                        batch->requests.push_back(std::move(req));
                    }
                }
            }
            
            current_idx = (current_idx + 1) % MAX_CLIENTS;
            clients_checked++;
            
            if (clients_checked == MAX_CLIENTS && batch->requests.size() < MAX_BATCH_SIZE)
            {
                clients_checked = 0;
            }
        }
        
        start_idx = (current_idx + 1) % MAX_CLIENTS;
        
        batch->remaining_requests.store(batch->requests.size(), std::memory_order_relaxed);
        return batch;
    }

    void scheduler_loop()
    {
        size_t total_duration = 0;
        while (running && completed_ops < target_ops)
        {
            auto *batch = prepare_new_batch();
            current_batch.store(batch, std::memory_order_relaxed);
            if (!batch->requests.empty())
            {
                auto start_time = std::chrono::steady_clock::now();
                for (const auto &req : batch->requests)
                {
                    hash_table->insert(req.key, req.value);
                    completed_ops++;
                }
                auto end_time = std::chrono::steady_clock::now();
                auto duration_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();
                total_duration += duration_ns;
                // std::cout << "Insert time of current batch:" << duration_ns << std::endl;
                batch->requests.clear();
            }
            else
            {
                delete batch;
                continue;
            }
        }
        std::cout << "Total scheduler time:" << total_duration << std::endl;
        Scheduler::get().has_scheduling = false;
        running = false;
    }

public:
    HashTableServer(SystematicTestHarness &h)
        : harness(h)
    {
        MAX_BATCH_SIZE = FLAGS_batch_size;
        target_ops = FLAGS_num_ops;
        hash_table = new ExtendibleHash(HASH_INIT_BUCKET_NUM, HASH_ASSOC_NUM);
        harness.external_thread([this]()
                                { scheduler_loop();
                                });
    }

    size_t register_client_queue(SPSCQueue<HashRequest, QUEUE_SIZE> *queue, size_t client_id)
    {
        for (size_t i = 0; i < MAX_CLIENTS; i++)
        {
            bool expected = false;
            if (active_clients[i].compare_exchange_strong(expected, true))
            {
                client_queues[i] = queue;
                return i;
            }
        }
        throw std::runtime_error("Max clients reached");
    }

    void unregister_client_queue(size_t client_id)
    {
        if (client_id < MAX_CLIENTS)
        {
            client_queues[client_id] = nullptr;
            active_clients[client_id] = false;
        }
    }

    bool is_running() const { return running; }
};