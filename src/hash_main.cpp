#include "hash_server.h"
#include "hash_client.h"
// #include "/users/Xuran/hash_rt/include/verona-rt/src/rt/debug/harness.h"
#include </users/Xuran/hash_rt/include/verona-rt/src/rt/verona.h>

int main(int argc, char **argv)
{
    google::ParseCommandLineFlags(&argc, &argv, false);
    // Logging::enable_logging();
    size_t total_cores = FLAGS_num_threads;
    size_t scheduler_cores = total_cores / 2;
    size_t external_cores = total_cores - scheduler_cores;

    SystematicTestHarness harness(argc, argv);

    harness.external_core_start = scheduler_cores;
    harness.external_core_count = external_cores;
    
    auto &sched = verona::rt::Scheduler::get();
    sched.set_fair(true);
    sched.init(scheduler_cores);
    HashTableServer server(harness);

    size_t client_threads = external_cores - 1;

    std::vector<std::unique_ptr<HashTableClient>> clients;

    auto start_time = std::chrono::steady_clock::now();
    
    std::cout << "Scheduler cores: " << scheduler_cores << std::endl;
    for (size_t i = 0; i < client_threads; i++)
    {
        clients.push_back(std::make_unique<HashTableClient>(i, server));
        harness.external_thread([&clients, i]()
                                { clients[i]->run(); });
    }

    // std::this_thread::sleep_for(std::chrono::microseconds(1000000));

    auto run_start = std::chrono::steady_clock::now();
    sched.run();
    auto run_end = std::chrono::steady_clock::now();

    auto run_time = std::chrono::duration_cast<std::chrono::nanoseconds>(run_end - run_start).count();

    auto [when_total_time, when_count] = server.hash_table->get_when_stats();
    double avg_when_time = when_count > 0 ? static_cast<double>(when_total_time) / when_count : 0;
    benchmark_report("insert", "run_time_ns", std::to_string(run_time));
    benchmark_report("scheduler", "total_when_schedules", std::to_string(when_count));
    benchmark_report("scheduler", "total_when_time_ns", std::to_string(when_total_time));

    std::cout << "Teardown: all threads stopped" << std::endl;
    // auto current_time = std::chrono::steady_clock::now();
    // auto duration_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(current_time - start_time).count();
    // std::cout << "Run behaviours time:" << duration_ns << std::endl;
    size_t completed_inserts = server.hash_table->get_completed_inserts();
    double throughput = completed_inserts / (run_time / 1e9);
    double avg_insert_time = run_time / completed_inserts;
    // benchmark_report("insert", "overall_duration_ns", std::to_string(duration_ns));
    benchmark_report("insert", "overall_operation_number", std::to_string(completed_inserts));
    benchmark_report("insert", "overall_throughput", std::to_string(throughput));

    benchmark_report("scheduler", "avg_when_time_ns", std::to_string(avg_when_time));
    benchmark_report("insert", "avg_insert_time_ns", std::to_string(avg_insert_time));

    return 0;
}