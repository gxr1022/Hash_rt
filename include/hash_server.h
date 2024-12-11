#pragma once
#include <gflags/gflags.h>
#include "chained_hash_rt.h"
#include "hash_request.h"
#include <verona.h>
#include "spsc.h"

#define QUEUE_SIZE 1048576 // 2^20, closest power of 2 above 1024000


class HashTableServer
{
public:
    using Scheduler = ThreadPool<SchedulerThread>;
    size_t MAX_BATCH_SIZE;
    std::atomic<size_t> current_batch_id{0};
    std::atomic<bool> running{true};
    std::atomic<size_t> completed_ops{0};
    size_t target_ops;

    // std::atomic<bool> is_first_batch{false};

    ExtendibleHash *hash_table;

    // Fixed-size array for client queues
    static constexpr size_t MAX_CLIENTS = 20; // Or other suitable size
    std::array<SPSCQueue<HashRequest, QUEUE_SIZE> *, MAX_CLIENTS> client_queues{};
    std::atomic<bool> active_clients[MAX_CLIENTS]{}; // Track active clients

    std::mt19937 rng;

    void generate_random_string(char* buffer, size_t length) 
    {
        static const char charset[] = "abcdefghijklmnopqrstuvwxyz";
        for (size_t i = 0; i < length - 1; ++i) {
            buffer[i] = charset[rng() % (sizeof(charset) - 1)];
        }
        buffer[length - 1] = '\0';
    }


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
        
        while( batch->requests.size() < MAX_BATCH_SIZE)
        {
            char key[MAX_KEY_LENGTH];
            char value[MAX_VALUE_LENGTH];
            // strcpy(key, "1234568"); 
            generate_random_string(key, MAX_KEY_LENGTH);
            generate_random_string(value, MAX_VALUE_LENGTH);
            batch->requests.push_back(HashRequest(key, value, 0));
        }

        return batch;
    }

    void scheduler_loop()
    {
        auto &scheduler = Scheduler::get();
        auto start = std::chrono::high_resolution_clock::now();
        while (running && completed_ops < target_ops)
        {
            auto *batch = prepare_new_batch();
            current_batch.store(batch, std::memory_order_relaxed);
            if (!batch->requests.empty())
            {
                size_t batch_size = batch->requests.size();
                
                for (const auto &req : batch->requests)
                {
                    hash_table->insert(req.key, req.value);
                    completed_ops++;
                }
              
                batch->requests.clear();
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
    HashTableServer(size_t num_ops):rng(std::random_device{}())
    {
        MAX_BATCH_SIZE = Scheduler::get().batch_size;
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
