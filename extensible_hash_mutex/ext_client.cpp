
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

DEFINE_uint64(num_threads, 32, "the number of threads");
DEFINE_uint64(num_of_ops, 1000000, "the number of operations");
DEFINE_uint64(time_interval, 10, "the time interval of insert operations");
DEFINE_string(report_prefix, "[report] ", "prefix of report data");

class Client
{
private:
    /* data */
public:

    uint64_t num_of_ops_;
    uint64_t num_threads_;
    uint64_t time_interval_;

    std::unique_ptr<ExtendibleHash> hashTable;
    std::string load_benchmark_prefix = "load";

    Client(int argc, char **argv);
    ~Client();
    void client_ops_cnt(uint32_t ops_num);
    void load_and_run();
    void standard_report(const std::string &prefix, const std::string &name, const std::string &value);

    void benchmark_report(const std::string benchmark_prefix, const std::string &name, const std::string &value)
    {
        standard_report(benchmark_prefix, name, value);
    }
    void init_myhash()
    {
        hashTable =  std::make_unique<ExtendibleHash>(5,4);
    }
};

Client::Client(int argc, char **argv)
{
    google::ParseCommandLineFlags(&argc, &argv, false);

    this->num_threads_ = FLAGS_num_threads;
    this->num_of_ops_ = FLAGS_num_of_ops;
    this->time_interval_ = FLAGS_time_interval;

    init_myhash();

}

Client::~Client()
{

}

void Client::load_and_run()
{
    // Create and start client threads
    std::vector<std::thread> threads;
    uint32_t num_of_ops_per_thread = num_of_ops_ / num_threads_;
    for (int i = 0; i < num_threads_; i++)
    {
        threads.emplace_back([this, num_of_ops_per_thread]() {
            this->client_ops_cnt(num_of_ops_per_thread);
        });
    }

    // Wait for all client threads to finish
    auto start_time = std::chrono::high_resolution_clock::now();
    for (auto &thread : threads)
    {
        thread.join();
    }
    double duration_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now() - start_time).count();
    double duration_s = duration_ns / 1000000000;
    double throughput = num_of_ops_ / duration_s;
    double average_latency_ns = (double)duration_ns / num_of_ops_;

    // benchmark_report(load_benchmark_prefix, "overall_duration_ns", std::to_string(duration_ns));
    benchmark_report(load_benchmark_prefix, "overall_duration_s", std::to_string(duration_s));
    benchmark_report(load_benchmark_prefix, "overall_throughput", std::to_string(throughput));
    benchmark_report(load_benchmark_prefix, "overall_duration_ns", std::to_string(duration_ns));
}

void Client::client_ops_cnt(uint32_t ops_num) {
    for (int i = 0; i < ops_num; i ++) {
        hashTable->insert(i, i*10);
    }
    return;
}

void Client::standard_report(const std::string &prefix, const std::string &name, const std::string &value)
{
    std::cout << FLAGS_report_prefix << prefix + "_" << name << " : " << value << std::endl;
}

int main(int argc, char *argv[])
{
    google::ParseCommandLineFlags(&argc, &argv, false);
    Client kv_bench(argc, argv);
    kv_bench.load_and_run();
    return 0;
}



