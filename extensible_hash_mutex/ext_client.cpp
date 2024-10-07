
#include <vector>
#include <string>
#include <optional>
#include <cmath>
#include <cstdint>
#include <mutex>
#include <atomic>
#include <fstream>
#include <unistd.h>
#include <cassert>
#include <iomanip>
#include <thread>
#include <gflags/gflags.h>
// #include "hash_split.h"
#include "ext_hash.h"
#include "affinity.hpp"

DEFINE_uint64(str_key_size, 8, "size of key (bytes)");
DEFINE_uint64(str_value_size, 100, "size of value (bytes)");
DEFINE_uint64(num_threads, 32, "the number of threads");
DEFINE_uint64(num_of_ops, 16, "the number of operations");
DEFINE_uint64(time_interval, 10, "the time interval of insert operations");
DEFINE_string(report_prefix, "[report] ", "prefix of report data");
DEFINE_bool(first_mode, true, "fist mode start multiply clients on the same key value server");
DEFINE_string(core_binding, "", "Core Binding, example : 0,1,16,17");

class Client
{
private:
    /* data */
public:
    std::atomic<uint64_t> num_of_ops_;
    uint64_t num_threads_;
    uint64_t time_interval_;
    uint64_t key_size;
    uint64_t value_size;
    std::atomic<bool> stop_flag;
    std::string common_value;
    bool first_mode;
    std::string core_binding;

    std::string load_benchmark_prefix = "load";

    Client(int argc, char **argv);
    ~Client();
    void client_ops_cnt_second(int thread_id, uint64_t core_id);
    void client_ops_cnt_first(ExtendibleHash *hashTable, int thread_id, uint64_t core_id);
    void load_and_run();
    std::string from_uint64_to_string(uint64_t value, uint64_t value_size);
    void standard_report(const std::string &prefix, const std::string &name, const std::string &value);
    void split_string_from_input(std::vector<int> &splited_str, std::string input_str);

    void benchmark_report(const std::string benchmark_prefix, const std::string &name, const std::string &value)
    {
        standard_report(benchmark_prefix, name, value);
    }
};

Client::Client(int argc, char **argv) : stop_flag(false)
{
    google::ParseCommandLineFlags(&argc, &argv, false);

    this->num_threads_ = FLAGS_num_threads;
    // this->num_of_ops_ = 0;
    this->num_of_ops_ = FLAGS_num_of_ops;
    this->time_interval_ = FLAGS_time_interval;
    this->key_size = FLAGS_str_key_size;
    this->value_size = FLAGS_str_value_size;
    this->first_mode = FLAGS_first_mode;
    this->core_binding = FLAGS_core_binding;
    for (int i = 0; i < value_size; i++)
    {
        common_value += (char)('a' + (i % 26));
    }
    // init_myhash();
}

Client::~Client()
{
}

void Client::load_and_run()
{
    // Create and start client threads
    std::vector<std::thread> threads;
    std::vector<int> core_ids;
    split_string_from_input(core_ids, core_binding);

    if (!first_mode)
    {
        for (int i = 0; i < num_threads_; i++)
        {
            uint64_t core_id = core_ids[i];
            threads.emplace_back([this, i, core_id]()
                                 { this->client_ops_cnt_second(i, core_id); });
        }
    }
    else
    {
        auto hashTable = new ExtendibleHash(HASH_INIT_BUCKET_NUM, HASH_ASSOC_NUM);
        for (int i = 0; i < num_threads_; i++)
        {
            uint64_t core_id = core_ids[i];
            threads.emplace_back([this, hashTable, i, core_id]()
                                 { this->client_ops_cnt_first(hashTable, i, core_id); });
        }
    }

    // std::this_thread::sleep_for(std::chrono::seconds(time_interval_));
    // stop_flag.store(true);

    // Wait for all client threads to finish
    auto start_time = std::chrono::steady_clock::now();
    for (auto &thread : threads)
    {
        thread.join();
    }
    auto current_time = std::chrono::steady_clock::now();
    auto duration_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(current_time - start_time).count();
    double throughput = num_of_ops_ / duration_ns;
   
    // double duration_s = double(time_interval_);
    // double duration_ns = duration_s * 1000000000;
    // double throughput = num_of_ops_.load() / duration_s;
    // double average_latency_ns = (double)duration_ns / num_of_ops_.load();

    benchmark_report(load_benchmark_prefix, "overall_duration_ns", std::to_string(duration_ns));
    // benchmark_report(load_benchmark_prefix, "overall_duration_s", std::to_string(duration_s));
    benchmark_report(load_benchmark_prefix, "overall_operation_number", std::to_string(num_of_ops_.load()));
    benchmark_report(load_benchmark_prefix, "overall_throughput", std::to_string(throughput));
    // benchmark_report(load_benchmark_prefix, "overall_duration_ns", std::to_string(duration_ns));
}

void Client::client_ops_cnt_second(int thread_id, uint64_t core_id)
{
    set_affinity(core_id);
    auto hashTable = std::make_unique<ExtendibleHash>(HASH_INIT_BUCKET_NUM, HASH_ASSOC_NUM);
    uint64_t rand = 0;
    std::string key;
    uint64_t ops_per_thread = num_of_ops_ / num_threads_;
    while (rand < ops_per_thread)
    {
        key = from_uint64_to_string(rand, key_size);
        hashTable->insert(key, common_value);
        rand++;
    }

    // while (!stop_flag.load())
    // {
    //     key = from_uint64_to_string(rand, key_size);
    //     hashTable->insert(key, common_value);
    //     rand++;
    // }
    // num_of_ops_ += rand;
    // num_of_ops_.fetch_add(rand);
    return;
}

void Client::client_ops_cnt_first(ExtendibleHash *hashTable, int thread_id, uint64_t core_id)
{
    set_affinity(core_id);
    uint64_t rand = 0;
    std::string key;
    uint64_t ops_per_thread = num_of_ops_ / num_threads_;
    while (rand < ops_per_thread)
    {
        key = from_uint64_to_string(rand, key_size);
        hashTable->insert(key, common_value);
        rand++;
    }

    // while (!stop_flag.load())
    // {
    //     key = from_uint64_to_string(rand, key_size);
    //     hashTable->insert(key, common_value);
    //     rand++;
    // }
    // num_of_ops_ += rand;
    // num_of_ops_.fetch_add(rand);
    return;
}

void Client::standard_report(const std::string &prefix, const std::string &name, const std::string &value)
{
    std::cout << FLAGS_report_prefix << prefix + "_" << name << " : " << value << std::endl;
}

std::string Client::from_uint64_to_string(uint64_t value, uint64_t value_size)
{
    std::stringstream ss;
    ss << std::setfill('0') << std::setw(value_size) << std::hex << value;
    std::string str = ss.str();
    if (str.length() > value_size)
    {
        str = str.substr(str.length() - value_size);
    }
    return ss.str();
}

void Client::split_string_from_input(std::vector<int> &splited_str, std::string input_str)
{
    if (input_str.size() != 0)
    {
        std::stringstream ss(input_str);
        std::string item;
        while (std::getline(ss, item, ','))
        {
            splited_str.push_back(std::stoi(item));
            // std::cout<<std::stoi(item)<<" ";
        }
    }
}

int main(int argc, char *argv[])
{
    google::ParseCommandLineFlags(&argc, &argv, false);
    Client kv_bench(argc, argv);
    kv_bench.load_and_run();
    return 0;
}
