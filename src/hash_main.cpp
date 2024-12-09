#include "hash_server.h"
#include "hash_client.h"
// #include "/users/Xuran/hash_rt/include/verona-rt/src/rt/debug/harness.h"
#include <verona-rt/src/rt/verona.h>
#include <cstring>

DEFINE_uint64(num_ops, 1000000, "the number of insert operations");
DEFINE_uint64(str_key_size, 8, "size of key (bytes)");
DEFINE_uint64(str_value_size, 100, "size of value (bytes)");
DEFINE_uint64(num_threads_client, 8, "the number of threads");
DEFINE_uint64(num_threads_worker, 32, "the number of threads");
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

    // init server
    HashTableServer *server = new HashTableServer(num_ops);

    // init scheduler thread
    std::thread scheduler_thread([&server]()
                                 { server->scheduler_loop(); 
                                 cpu::set_affinity(0); });

    // init client threads
    std::vector<std::unique_ptr<HashTableClient>> clients;
    std::vector<std::thread> client_threads;
    for (size_t i = 0; i < num_client_threads; i++)
    {
        clients.push_back(std::make_unique<HashTableClient>(i, *server));
        client_threads.emplace_back([&clients, i]()
                                    { clients[i]->run(); });
    }

    // start worker threads
    // if (server->is_first_batch.load())
    {
        // auto run_start = std::chrono::steady_clock::now();
        sched.run();
        // auto run_end = std::chrono::steady_clock::now();
        // worker_time = std::chrono::duration_cast<std::chrono::nanoseconds>(run_end - run_start).count();
    }

    if (!server->is_running())
    {
        scheduler_thread.join();
    }

    if (!server->is_running())
    {
        for (auto &client : client_threads)
        {
            client.join();
        }
    }
    // auto [when_total_time, when_count] = server.hash_table->get_when_stats();
    // double avg_when_time = when_count > 0 ? static_cast<double>(when_total_time) / when_count : 0;
    // benchmark_report("scheduler", "total_when_schedules", std::to_string(when_count));
    // benchmark_report("scheduler", "total_when_time_ns", std::to_string(when_total_time));

    // std::cout << "Teardown: all threads stopped" << std::endl;

    // size_t completed_inserts = server->hash_table->get_completed_inserts();
    // double throughput = completed_inserts / (worker_time / 1e9);
    // double avg_insert_time = worker_time / completed_inserts;
    // benchmark_report("insert", "overall_duration_ns", std::to_string(duration_ns));
    // benchmark_report("insert", "worker_time_ns", std::to_string(worker_time));
    // benchmark_report("insert", "overall_operation_number", std::to_string(completed_inserts));
    // benchmark_report("insert", "overall_throughput", std::to_string(throughput));

    // benchmark_report("scheduler", "avg_when_time_ns", std::to_string(avg_when_time));
    // benchmark_report("insert", "avg_insert_time_ns", std::to_string(avg_insert_time));
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