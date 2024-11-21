#include "verona.h"
#include <iostream>
#include <vector>
#include "../include/chained_hash_rt.h"
// #include "../include/slot_hash_rt.h"
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


using namespace verona::rt;
using namespace std;

string load_benchmark_prefix;
char* common_value;

DEFINE_uint64(str_key_size, 8, "size of key (bytes)");
DEFINE_uint64(str_value_size, 100, "size of value (bytes)");
DEFINE_uint64(num_threads, 16, "the number of threads");
DEFINE_uint64(time_interval, 10, "the time interval of insert operations");
DEFINE_uint64(num_ops, 1000000, "the number of insert operations");
DEFINE_string(report_prefix, "[report]: ", "prefix of report data");
DEFINE_bool(first_mode, true, "fist mode start multiply clients on the same key value server");
DEFINE_uint64(work_usec, 0, "the time interval of insert operations");
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

char *from_uint64_to_char(uint64_t value, uint64_t value_size)
{
    char *buffer = new char[value_size + 1];
    snprintf(buffer, value_size + 1, "%0*lx", static_cast<int>(value_size), value);
    buffer[value_size] = '\0';

    size_t len = std::strlen(buffer);
    if (len > value_size)
    {
        std::memmove(buffer, buffer + (len - value_size), value_size);
        buffer[value_size] = '\0';
    }

    return buffer;
}

void standard_report(const std::string &prefix, const std::string &name, const std::string &value)
{
    std::cout << FLAGS_report_prefix << prefix + "_" << name << " : " << value << std::endl;
}

void benchmark_report(const std::string benchmark_prefix, const std::string &name, const std::string &value)
{
    standard_report(benchmark_prefix, name, value);
}

void performInsertions(ExtendibleHash *hashtable, size_t num_of_ops, size_t key_size, size_t value_size, uint64_t work_usec)
{
    for (int i = 0; i < num_of_ops; i++)
    {
        int r = rand() % HASH_INIT_BUCKET_NUM;
        char *key = from_uint64_to_char(r, key_size);
        // std::cout << "Inserting key: " << key << std::endl;
        hashtable->insert(key, common_value);
        // hashtable->printStatus();
    }

    return;
}

void runTest(ExtendibleHash *hashTable, size_t cores, size_t num_of_ops, size_t key_size, size_t value_size, uint64_t work_usec)
{

    auto start_time = std::chrono::steady_clock::now();
    auto &sched = verona::rt::Scheduler::get();
    sched.set_fair(true);
    sched.init(cores);
    

    auto current_time = std::chrono::steady_clock::now();
    auto duration_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(current_time - start_time).count();
    std::cout << "Init scheduler time:" << duration_ns << std::endl;

    start_time = std::chrono::steady_clock::now();
    performInsertions(hashTable, num_of_ops, key_size, value_size, work_usec);

    current_time = std::chrono::steady_clock::now();
    duration_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(current_time - start_time).count();
    std::cout << "Schedule behaviours time:" << duration_ns << std::endl;

    double sched_avg_time = duration_ns / num_of_ops;
    benchmark_report(load_benchmark_prefix, "sched_avg_time", std::to_string(sched_avg_time));
    
    start_time = std::chrono::steady_clock::now();
    sched.run();
    current_time = std::chrono::steady_clock::now();
    duration_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(current_time - start_time).count();
    std::cout << "Run behaviours time:" << duration_ns << std::endl;
    double run_avg_time = duration_ns / num_of_ops;
    benchmark_report(load_benchmark_prefix, "run_avg_time", std::to_string(run_avg_time));
    double throughput = num_of_ops / (duration_ns / 1e9);
    benchmark_report(load_benchmark_prefix, "overall_duration_ns", std::to_string(duration_ns));
    benchmark_report(load_benchmark_prefix, "overall_operation_number", std::to_string(num_of_ops));
    benchmark_report(load_benchmark_prefix, "overall_throughput", std::to_string(throughput));
    return;
}

int main(int argc, char **argv)
{

    // Logging::enable_logging();
    google::ParseCommandLineFlags(&argc, &argv, false);

    srand(time(nullptr));
    const auto cores = FLAGS_num_threads;
    uint64_t num_of_ops = FLAGS_num_ops;
    size_t key_size = FLAGS_str_key_size;
    size_t value_size = FLAGS_str_value_size;
    size_t time_interval = FLAGS_time_interval;
    bool first_mode = FLAGS_first_mode;
    uint64_t work_usec = FLAGS_work_usec;
    load_benchmark_prefix = "insert";

    common_value = new char[value_size + 1];
    for (int j = 0; j < value_size; j++)
    {
        common_value[j] = 'a' + (j % 26);
    }
    common_value[value_size] = '\0';


    std::cout << "Running with " << cores << " cores" << std::endl;

    auto hashTable = new ExtendibleHash(HASH_INIT_BUCKET_NUM, HASH_ASSOC_NUM);

    // auto start_time = std::chrono::high_resolution_clock::now();
    runTest(hashTable, cores, num_of_ops, key_size, value_size, work_usec);
    // double duration_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now() - start_time).count();
    // double throughput = num_of_ops / (duration_ns / 1e9);
    // benchmark_report(load_benchmark_prefix, "overall_duration_ns", std::to_string(duration_ns));
    // benchmark_report(load_benchmark_prefix, "overall_operation_number", std::to_string(num_of_ops));
    // benchmark_report(load_benchmark_prefix, "overall_throughput", std::to_string(throughput));

    return 0;
}
