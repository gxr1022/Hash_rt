#include <iostream>
#include <atomic>
#include <chrono>

struct MyStruct {
    int a;
    double b;
    char c;
};

void compare_exchange_performance(int iterations) {
    // 测试指针交换
    std::atomic<int*> atomic_ptr;
    int value1 = 42, value2 = 84;
    atomic_ptr.store(&value1);
    
    auto ptr_start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        atomic_ptr.exchange(&value2);
        
    }
    auto ptr_end = std::chrono::high_resolution_clock::now();
    
    std::atomic<MyStruct*> atomic_struct;
    MyStruct struct1{1, 3.14, 'x'};
    MyStruct struct2{2, 6.28, 'y'};
    atomic_struct.store(&struct1);
    
    auto struct_start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        atomic_struct.exchange(&struct2);
        atomic_struct.exchange(&struct1);
    }
    auto struct_end = std::chrono::high_resolution_clock::now();
    
    // 计算并输出结果
    auto ptr_duration = std::chrono::duration_cast<std::chrono::nanoseconds>(ptr_end - ptr_start);
    auto struct_duration = std::chrono::duration_cast<std::chrono::nanoseconds>(struct_end - struct_start);
    std::cout << "Pointer exchange average time: " << ptr_duration.count() / (2.0 * iterations) << " ns\n";
    std::cout << "Struct exchange average time: " << struct_duration.count() / (2.0 * iterations) << " ns\n";
    // std::cout << "指针交换平均时间: " << ptr_duration.count() / (2.0 * iterations) << " ns\n";
    // std::cout << "结构体交换平均时间: " << struct_duration.count() / (2.0 * iterations) << " ns\n";
}

int main() {
    int iterations = 1000000;
    compare_exchange_performance(iterations);
    return 0;
}