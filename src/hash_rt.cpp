#include "verona.h"
#include <iostream>
#include <vector>
#include "../include/chained_hash_rt.h"
#include "debug/harness.h"
#include "test/opt.h"
#include "test/xoroshiro.h"
#include "verona.h"
#include "test/opt.h"
#include <unordered_map>
#include <vector>
#include <gflags/gflags.h>
#include <random>
#include <stdlib.h>
#include <time.h>

#define HASH_INIT_BUCKET_NUM (100000000)
#define HASH_ASSOC_NUM (1)

using namespace verona::rt;
using namespace std;

string load_benchmark_prefix;
string common_value;
// int seed=65536; 

DEFINE_uint64(str_key_size, 8, "size of key (bytes)");
DEFINE_uint64(str_value_size, 100, "size of value (bytes)");
DEFINE_uint64(num_threads, 1, "the number of threads");
DEFINE_uint64(time_interval, 10, "the time interval of insert operations");
DEFINE_uint64(num_ops, 1000000, "the number of insert operations");
DEFINE_string(report_prefix, "[report] ", "prefix of report data");
DEFINE_bool(first_mode, true, "fist mode start multiply clients on the same key value server");

enum OperationType
{
    INSERT,
    UPDATE,
    DELETE,
    FIND
};

std::string from_uint64_to_string(uint64_t value, uint64_t value_size)
{
    std::stringstream ss;
    ss << std::setfill('0') << std::setw(value_size) << std::hex << value;
    std::string str = ss.str();

    if (str.length() > value_size)
    {
        str = str.substr(str.length() - value_size);
    }

    return str;
}

void standard_report(const std::string &prefix, const std::string &name, const std::string &value)
{
    std::cout << FLAGS_report_prefix << prefix + "_" << name << " : " << value << std::endl;
}

void benchmark_report(const std::string benchmark_prefix, const std::string &name, const std::string &value)
{
    standard_report(benchmark_prefix, name, value);
}

void performInsertions(ExtendibleHash *hashtable, size_t num_of_ops, size_t key_size, size_t value_size)
{
    for (int i = 0; i < num_of_ops; i++)
    {
        int r = rand() % HASH_INIT_BUCKET_NUM;
        string key = from_uint64_to_string(r, key_size);
        hashtable->insert(key, common_value);
    }
    return;
}

void runTest(ExtendibleHash* hashTable, size_t cores, size_t num_of_ops, size_t key_size, size_t value_size)
{
    
    auto start_time = std::chrono::steady_clock::now();
    auto &sched = verona::rt::Scheduler::get();
    sched.set_fair(true);
    sched.init(cores);
    
    auto current_time = std::chrono::steady_clock::now();
    auto duration_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(current_time - start_time).count();
    std::cout<<"Schedule time:"<<duration_ns<<std::endl;

    start_time = std::chrono::steady_clock::now();
    performInsertions(hashTable, num_of_ops, key_size, value_size);
    

    current_time = std::chrono::steady_clock::now();
    duration_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(current_time - start_time).count();
    std::cout<<"Insert time:"<<duration_ns<<std::endl;

    // for (int i = 0; i < 10; i++) {
    //     schedule_lambda(hashTable, [i, value = i * 100](acquired_cown<ExtendibleHash> ht_acq) mutable {
    //     int result = ht_acq->find(i);  // 使用 acquired_cown 来访问 hashTable
    //     if (result != -1)
    //         std::cout << "Found key: " << i << " value: " << result << std::endl;
    //     else
    //         std::cout << "Key not found: " << i << std::endl;
    // }

    // schedule_lambda([=]() mutable {
    //     for (int i = 0; i < 10; i++) {
    //         schedule_lambda(hashTable, [=](acquired_cown<ExtendibleHash> ht_acq) mutable {
    //             ht_acq->erase(i);
    //             std::cout << "Erased key: " << i << std::endl;
    //         });
    //     }
    // });

    // schedule_lambda([=]() mutable {
    //     schedule_lambda(hashTable, [=](acquired_cown<ExtendibleHash> ht_acq) mutable {
    //         ht_acq->printStatus();
    //     });
    // });

    start_time = std::chrono::steady_clock::now();
    sched.run();
    current_time = std::chrono::steady_clock::now();
    duration_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(current_time - start_time).count();
    std::cout<<"Run time:"<<duration_ns<<std::endl;
}

int main(int argc, char **argv)
{
    // opt::Opt opt(argc, argv);
    // const auto cores = opt.is<size_t>("--cores", 4);
    // size_t num_of_ops = opt.is<size_t>("--number_of_ops", 1000000);

    google::ParseCommandLineFlags(&argc, &argv, false);
    
    srand(time(nullptr));
    const auto cores = FLAGS_num_threads;
    uint64_t num_of_ops = FLAGS_num_ops;
    size_t key_size = FLAGS_str_key_size;
    size_t value_size = FLAGS_str_value_size;
    size_t time_interval = FLAGS_time_interval;
    bool first_mode = FLAGS_first_mode;
    load_benchmark_prefix = FLAGS_report_prefix;
    for (int j = 0; j < value_size; j++)
    {
        common_value += (char)('a' + (j % 26));
    }

    std::cout << "Running with " << cores << " cores" << std::endl;
    if (first_mode)
    {
        
        auto hashTable = new ExtendibleHash(HASH_INIT_BUCKET_NUM, HASH_ASSOC_NUM);
        
        auto start_time = std::chrono::high_resolution_clock::now();
        runTest(hashTable, cores, num_of_ops, key_size, value_size);
        double duration_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now() - start_time).count();
        double throughput = num_of_ops / (duration_ns / 1e9);
        benchmark_report(load_benchmark_prefix, "overall_duration_ns", std::to_string(duration_ns));
        benchmark_report(load_benchmark_prefix, "overall_operation_number", std::to_string(num_of_ops));
        benchmark_report(load_benchmark_prefix, "overall_throughput", std::to_string(throughput));
    }
    // else
    // {
    //     int num_of_ops_per_ins = num_of_ops / cores;
    //     auto start_time = std::chrono::steady_clock::now();
    //     for (int i = 1; i <= cores; i++)
    //     {
    //         runTest(1, num_of_ops_per_ins, key_size, value_size);
    //     }
    //     auto current_time = std::chrono::steady_clock::now();
    //     auto duration_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(current_time - start_time).count();
    //     double throughput = num_of_ops / (duration_ns / 1e9);
    //     benchmark_report(load_benchmark_prefix, "overall_duration_ns", std::to_string(duration_ns));
    //     benchmark_report(load_benchmark_prefix, "overall_operation_number", std::to_string(num_of_ops));
    //     benchmark_report(load_benchmark_prefix, "overall_throughput", std::to_string(throughput));
    // }
    return 0;
}
