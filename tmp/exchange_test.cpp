#include <iostream>
#include <atomic>
#include <chrono> // For std::chrono


struct MyStruct {
    int a;
    double b;
    char c;
};

void test_atomic_struct_exchange(int iterations) {
    std::atomic<MyStruct> atomic_struct; 
    MyStruct value1{1, 3.14, 'x'};     
    MyStruct value2{2, 6.28, 'y'};    
    atomic_struct.store(value1);


    auto exchange_start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < iterations; ++i) {
        atomic_struct.exchange(value2); 
        atomic_struct.exchange(value1); 
    }

    auto exchange_end = std::chrono::high_resolution_clock::now();
    auto exchange_duration =
        std::chrono::duration_cast<std::chrono::nanoseconds>(exchange_end - exchange_start);

    double time_per_exchange = static_cast<double>(exchange_duration.count()) / (2.0 * iterations); // 单位 ns
    std::cout << "Total elapsed time: " << exchange_duration.count() << " ns" << std::endl;
    std::cout << "Average time per exchange: " << time_per_exchange << " ns" << std::endl;
}


void test_atomic_pointer_exchange(int iterations) {
    std::atomic<int*> atomic_ptr; // 原子指针
    int value1 = 42, value2 = 84; // 测试数据
    atomic_ptr.store(&value1);

    // 使用 std::chrono 测量时间
    auto exchange_start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < iterations; ++i) {
        atomic_ptr.exchange(&value2); // 交换指针
        atomic_ptr.exchange(&value1); // 再次交换
    }

    auto exchange_end = std::chrono::high_resolution_clock::now();
    auto exchange_duration =
        std::chrono::duration_cast<std::chrono::nanoseconds>(exchange_end - exchange_start);

    // 输出结果
    double time_per_exchange = static_cast<double>(exchange_duration.count()) / (2.0 * iterations); // 单位 ns
    std::cout << "Total elapsed time: " << exchange_duration.count() << " ns" << std::endl;
    std::cout << "Average time per exchange: " << time_per_exchange << " ns" << std::endl;
}

int main() {
    int iterations = 1000000; 
    std::cout << "Testing atomic pointer exchange with " << iterations << " iterations...\n";
    test_atomic_struct_exchange(iterations);
    return 0;
}
