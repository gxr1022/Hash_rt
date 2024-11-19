#pragma once
#include <gflags/gflags.h>
#include "chained_hash_rt.h"
#include "hash_request.h"
#include <verona.h>

DEFINE_uint64(num_ops, 1000000, "the number of insert operations");
DEFINE_uint64(str_key_size, 8, "size of key (bytes)");
DEFINE_uint64(str_value_size, 100, "size of value (bytes)");
DEFINE_uint64(num_threads, 8, "the number of threads");
DEFINE_uint64(time_interval, 10, "the time interval of insert operations");
DEFINE_string(report_prefix, "[report]: ", "prefix of report data");
DEFINE_bool(first_mode, true, "fist mode start multiply clients on the same key value server");
DEFINE_uint64(work_usec, 0, "the time interval of insert operations");

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
private:
    static constexpr size_t MAX_BATCH_SIZE = 1000;
    SystematicTestHarness& harness;
    std::atomic<bool> running{true};
    std::atomic<size_t> completed_ops{0};
    const size_t target_ops = FLAGS_num_ops;
    
    ExtendibleHash* hash_table;
    
    // Fixed-size array for client queues
    static constexpr size_t MAX_CLIENTS = 64;  // Or other suitable size
    std::array<SPSCQueue<HashRequest, 1024>*, MAX_CLIENTS> client_queues{};
    std::atomic<bool> active_clients[MAX_CLIENTS]{};  // Track active clients

    void scheduler_loop()
    {
        std::vector<HashRequest> batch;
        batch.reserve(MAX_BATCH_SIZE);
        size_t current_client = 0;
        
        while (running && completed_ops < target_ops)
        {
            // Round-robin through client slots
            for (size_t i = 0; i < MAX_CLIENTS; i++) 
            {
                current_client = (current_client + 1) % MAX_CLIENTS;
                
                if (!active_clients[current_client]) {
                    continue;  // Skip inactive slots
                }

                auto* queue = client_queues[current_client];
                if (queue) {
                    while (batch.size() < MAX_BATCH_SIZE) {
                        HashRequest req;
                        if (!queue->try_pop(req)) break;
                        batch.push_back(std::move(req));
                    }
                }
            }
            
            if (!batch.empty()) {
                for (const auto& req : batch) {
                    hash_table->insert(req.key, req.value);
                    completed_ops++;
                }
                batch.clear();
            } else {
                // verona::rt::Cown::yield();
            }
        }
        
        running = false;
    }

public:
    HashTableServer(SystematicTestHarness& h) 
        : harness(h)
    {
        hash_table = new ExtendibleHash(HASH_INIT_BUCKET_NUM, HASH_ASSOC_NUM);
        harness.external_thread([this]() {
            scheduler_loop(); 
        });
    }

    size_t register_client_queue(SPSCQueue<HashRequest, 1024>* queue, size_t client_id) {
        for (size_t i = 0; i < MAX_CLIENTS; i++) {
            bool expected = false;
            if (active_clients[i].compare_exchange_strong(expected, true)) {
                client_queues[i] = queue;
                return i; 
            }
        }
        throw std::runtime_error("Max clients reached");
    }

    void unregister_client_queue(size_t client_id) {
        if (client_id < MAX_CLIENTS) {
            client_queues[client_id] = nullptr;
            active_clients[client_id] = false;
        }
    }

    bool is_running() const { return running; }
};