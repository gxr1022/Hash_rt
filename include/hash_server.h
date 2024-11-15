#pragma once
#include <gflags/gflags.h>
#include "chained_hash_rt.h"
#include "hash_request.h"
#include <verona.h>

DEFINE_uint64(num_ops, 100000, "the number of insert operations");

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
public:
    ExtendibleHash* hash_table;
private:
    static constexpr size_t MAX_BATCH_SIZE = 1000;
    SystematicTestHarness& harness;
    std::atomic<bool> running{true};
    std::atomic<size_t> completed_ops{0};
    const size_t target_ops = FLAGS_num_ops;

    std::vector<HashRequest> request_queue;

    std::mutex queue_mutex;
    std::condition_variable cv;

    std::atomic<bool> scheduler_started{false};
    std::atomic<size_t> pending_when_ops{0};

    void scheduler_loop()
    {
        std::vector<HashRequest> batch;
        size_t batch_size = 0;
        
   
        while (running && completed_ops < target_ops)
        {
            std::cout << "Pending when ops: " << pending_when_ops.load() 
                      << ", Queue size: " << request_queue.size() 
                      << std::endl;
            {
                std::unique_lock<std::mutex> lock(queue_mutex);
                if (request_queue.empty())
                {
                    cv.wait_for(lock, std::chrono::milliseconds(10));
                    continue;
                }
                         
                std::cout << "Processing batch, queue size: " 
                          << request_queue.size() << std::endl;
                batch_size = std::min<size_t>(request_queue.size(), MAX_BATCH_SIZE);
                batch.insert(batch.end(),
                             request_queue.begin(),
                             request_queue.begin() + batch_size);
                request_queue.erase(
                    request_queue.begin(),
                    request_queue.begin() + batch_size);
            }
     
            if (!batch.empty())
            {
                for (auto& req : batch)
                {
                    hash_table->insert(req.key, req.value, req.work_usec);
                }
                completed_ops += batch_size;
            }
            
            batch.clear();
        }
        
        std::cout << "completed_ops: " << hash_table->get_completed_inserts() << std::endl;
        running = false;
    }

    void start_scheduler() {
        bool expected = false;
        if (scheduler_started.compare_exchange_strong(expected, true)) {
            harness.external_thread([this]() {
                scheduler_loop();
            });
        }
    }

public:
    HashTableServer(SystematicTestHarness& h) : harness(h)
    {
        hash_table = new ExtendibleHash(HASH_INIT_BUCKET_NUM, HASH_ASSOC_NUM);
    }

    ~HashTableServer() {
        std::cout<< "completed_ops: " << hash_table->get_completed_inserts() << std::endl;
        delete hash_table;
    }

    void handle_insert(const char* key, const char* value, 
                      size_t client_id, uint64_t work_usec)
    {
        if (!running.load()) return;
        
        if (!scheduler_started.load()) {
            start_scheduler();
        }

        try {
            std::lock_guard<std::mutex> lock(queue_mutex);
            if (!running) return;
            
            // std::cout << "Adding work to queue: " << key << std::endl;
            
            request_queue.emplace_back(key, value, client_id, work_usec);
            cv.notify_one();
        } catch (const std::system_error& e) {
            return;
        }
    }

    bool is_running() const { return running; }

    bool has_pending_work() const {
        return pending_when_ops > 0 || !request_queue.empty();
    }
};