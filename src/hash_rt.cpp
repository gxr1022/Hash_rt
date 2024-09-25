#include "verona.h"
#include <iostream>
#include <vector>
#include "../include/hash_rt.h"
#include "debug/harness.h"
#include "test/opt.h"
#include "test/xoroshiro.h"
#include "verona.h"
#include "test/opt.h"
#include <unordered_map>
#include <vector>

using namespace verona::rt;
using namespace std;

enum OperationType { INSERT, UPDATE, DELETE, FIND };

void performInsertions(ExtendibleHash* ht_acq, size_t num_of_ops)
{
    for (int i = 0; i < num_of_ops; i++) {
        int value = i * 100;
        ht_acq->insert(i, value, ht_acq);
        std::cout << "Inserted key: " << i << " value: " << value << std::endl;
    }
}

void runTest(size_t cores, size_t num_of_ops)
{
    auto& sched = verona::rt::Scheduler::get();
    sched.set_fair(true);
    sched.init(cores);    

    // auto hashTable = std::make_shared<ExtendibleHash>(4, 1000);
    auto hashTable = new ExtendibleHash(4, 1000);
    // auto hashTable = ExtendibleHash::create(4, 1000); 
 
    performInsertions(hashTable,num_of_ops);

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

    auto start_time = std::chrono::high_resolution_clock::now();
    sched.run();  
    double duration_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now() - start_time).count();
    std::cout << "Execution Time: " << duration_ns << " ns" << std::endl;
}

int main(int argc, char** argv)
{
    opt::Opt opt(argc, argv);
    const auto cores = opt.is<size_t>("--cores", 4); 
    size_t num_of_ops = opt.is<size_t>("--number_of_ops", 100);
    std::cout << "Running with " << cores << " cores" << std::endl;

    runTest(cores,num_of_ops);
    return 0;
}
