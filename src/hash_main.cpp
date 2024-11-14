#include "hash_server.h"
#include "hash_client.h"
#include </users/Xuran/hash_rt/include/verona-rt/src/rt/verona.h>


DEFINE_uint64(str_key_size, 8, "size of key (bytes)");
DEFINE_uint64(str_value_size, 100, "size of value (bytes)");
DEFINE_uint64(num_threads, 5, "the number of threads");
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

int main(int argc, char** argv) {
    size_t total_cores = FLAGS_num_threads;
    
    HashTableServer server; //Create server 

    size_t scheduler_cores = total_cores / 2;
    size_t client_cores = total_cores - scheduler_cores - 1;
    size_t client_start_core = scheduler_cores + 1;
    // 1 core for schedule works
    
    server.start(scheduler_cores);
    
    std::vector<std::unique_ptr<HashTableClient>> clients;
    for (size_t i = 0; i < client_cores; i++) {
        clients.push_back(std::make_unique<HashTableClient>(i, server));
        std::thread([&clients, i, client_start_core ]() {
            cpu::set_affinity(client_start_core + i);
            clients[i]->run();
        }).detach();
    }
    
    while (server.is_running()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    return 0;
}