#pragma once
#include <gflags/gflags.h>
#include "chained_hash_rt.h"
#include "hash_request.h"
#include <verona.h>
#include "spsc.h"

#define QUEUE_SIZE 1048576 // 2^20, closest power of 2 above 1024000

DEFINE_uint64(batch_size, 32, "the size of batch");

class HashTableServer
{
public:
    size_t MAX_BATCH_SIZE;
    std::atomic<size_t> current_batch_id{0};
    std::atomic<bool> running{true};
    std::atomic<size_t> completed_ops{0};
    size_t target_ops;

    std::atomic<bool> is_first_batch{false};

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

        return batch;
    }

    void scheduler_loop()
    {
        auto start = std::chrono::high_resolution_clock::now();
        while (running && completed_ops < target_ops)
        {
            auto *batch = prepare_new_batch(); // prepare a new batch of requests from clients
            current_batch.store(batch, std::memory_order_relaxed);
            if (!batch->requests.empty())
            {
                size_t cur_batch_size = batch->requests.size();
                char *key[cur_batch_size];
                char *value[cur_batch_size];
                for (size_t i = 0; i < cur_batch_size; i++)
                {
                    key[i] = batch->requests[i].key;
                    value[i] = batch->requests[i].value;
                }
                hash_table->insert_batch(key, value, cur_batch_size);
                completed_ops += cur_batch_size;
                batch->requests.clear();
                if (current_batch_id.load() == 1)
                {
                    is_first_batch.store(true, std::memory_order_relaxed);
                }
            }
            else
            {
                delete batch;
                continue;
            }
        }
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        std::cout << "scheduler loop time: " << duration << std::endl;
        Scheduler::get().has_scheduling = false;
        running = false;
    }

public:
    HashTableServer(size_t num_ops)
    {
        MAX_BATCH_SIZE = FLAGS_batch_size;
        target_ops = num_ops;
        hash_table = new ExtendibleHash(HASH_INIT_BUCKET_NUM, HASH_ASSOC_NUM);
    }

    size_t register_client_queue(SPSCQueue<HashRequest, QUEUE_SIZE> *queue, size_t client_id)
    {

        bool expected = false;
        if (active_clients[client_id].compare_exchange_strong(expected, true))
        {
            client_queues[client_id] = queue;
            return client_id;
        }
        return -1;
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
    ~HashTableServer()
    {
        delete hash_table;
    }
};
