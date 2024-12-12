#include "hash_server.h"
#include "hash_client.h"
#include "verona-rt/src/rt/sched/schedulerthread.h"
// #include "/users/Xuran/hash_rt/include/verona-rt/src/rt/debug/harness.h"
#include <verona-rt/src/rt/verona.h>
#include <cstring>

DEFINE_uint64(num_ops, 100, "the number of insert operations");
DEFINE_uint64(str_key_size, 8, "size of key (bytes)");
DEFINE_uint64(str_value_size, 100, "size of value (bytes)");
DEFINE_uint64(num_threads_client, 1, "the number of threads");
DEFINE_uint64(num_threads_worker, 4, "the number of threads");
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

class BenchmarkTest
{
public:
    friend HashTableServer;
    friend HashTableClient;
    friend HashRequest;
    size_t num_ops;
    size_t num_scheduler_threads;
    size_t num_client_threads;
    double worker_time;
    void runBenchmark();

    BenchmarkTest() : num_ops(FLAGS_num_ops), num_scheduler_threads(FLAGS_num_threads_worker), num_client_threads(FLAGS_num_threads_client) {
       worker_time = 0;
    }
};

void BenchmarkTest::runBenchmark()
{
    // init worker thread pool
    auto &sched = verona::rt::Scheduler::get();
    sched.set_fair(true);
    sched.init(num_scheduler_threads);

    cpu::set_affinity(0);
    // init server
    HashTableServer *server = new HashTableServer(num_ops);

    auto total_start_time = std::chrono::steady_clock::now();
    // init scheduler thread
    auto server_start = std::chrono::steady_clock::now();
    server->scheduler_loop();

    auto server_end = std::chrono::steady_clock::now();
    auto server_duration = std::chrono::duration_cast<std::chrono::nanoseconds>(server_end - server_start).count();

    // init client threads
    // auto client_start = std::chrono::steady_clock::now();
    std::vector<std::unique_ptr<HashTableClient>> clients;
    std::vector<std::thread> client_threads;
    for (size_t i = 0; i < num_client_threads; i++)
    {
        clients.push_back(std::make_unique<HashTableClient>(i, *server));
        client_threads.emplace_back([&clients, i]()
                                    { clients[i]->run(); });
    }
    // auto client_end = std::chrono::steady_clock::now();
    // auto client_duration = std::chrono::duration_cast<std::chrono::nanoseconds>(client_end - client_start).count();

    // start worker threads
    auto run_start = std::chrono::steady_clock::now();
    sched.run();
    auto run_end = std::chrono::steady_clock::now();
    auto run_duration = std::chrono::duration_cast<std::chrono::nanoseconds>(run_end - run_start).count();

    // auto join_start = std::chrono::steady_clock::now();
    // if (!server->is_running())
    // {
    //     scheduler_thread.join();
    // }
    
    // if (!server->is_running())
    // {
    //     for (auto &client : client_threads)
    //     {
    //         client.join();
    //     }
    // }
    
    // auto join_end = std::chrono::steady_clock::now();
    // auto join_duration = std::chrono::duration_cast<std::chrono::nanoseconds>(join_end - join_start).count();
    auto total_end_time = std::chrono::steady_clock::now();
    auto total_duration = std::chrono::duration_cast<std::chrono::nanoseconds>(total_end_time - total_start_time).count();
    size_t completed_inserts = server->hash_table->get_completed_inserts();
    double total_throughput = completed_inserts / (total_duration / 1e9);
    double avg_time_per_ops = total_duration / completed_inserts;

    benchmark_report("overall", "duration_ns", std::to_string(total_duration));
    benchmark_report("overall", "operation_number", std::to_string(completed_inserts));
    benchmark_report("overall", "throughput", std::to_string(total_throughput));
    benchmark_report("overall", "avg_time_per_ops", std::to_string(avg_time_per_ops));
    benchmark_report("overall", "run_duration", std::to_string(run_duration));
    benchmark_report("overall", "server_duration", std::to_string(server_duration));
    // benchmark_report("overall", "client_duration", std::to_string(client_duration));
    // benchmark_report("overall", "join_duration", std::to_string(join_duration));
    return;
}

int main(int argc, char **argv)
{
    Logging::enable_logging();
    google::ParseCommandLineFlags(&argc, &argv, false);
    BenchmarkTest benchmark;
    benchmark.runBenchmark();
    return 0;
}