#include <iostream>
#include <chrono>
#include <atomic>
#include <vector>
#include <random>
#include <algorithm>

template <typename T>
std::vector<T *> create_scattered_array(size_t count)
{
    std::vector<T *> ptrs(count);

    char *pool = new char[sizeof(T) * count];

    std::vector<size_t> indices(count);
    for (size_t i = 0; i < count; i++)
        indices[i] = i;
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::shuffle(indices.begin(), indices.end(), gen);

    for (size_t i = 0; i < count; i++)
    {
        ptrs[i] = new (pool + indices[i] * sizeof(T)) T();
    }
    return ptrs;
}

struct SmallStruct
{
    std::atomic<uint64_t> first;
    char padding[16]; // Total size: 24 bytes
};

struct LargeStruct
{
    std::atomic<uint64_t> first;
    char padding[545]; // Total size: 553 bytes
};

template <typename T>
void measure_access_latency(std::vector<T> &structs, const std::string &struct_name)
{
    auto start = std::chrono::high_resolution_clock::now();

    for (auto &s : structs)
    {
        // Access the first atomic 8 bytes
        s.first.fetch_add(1, std::memory_order_relaxed);
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::nano> duration = end - start;
    std::cout << "Access latency for " << struct_name << ": "
              << duration.count() / structs.size() << " ns per access" << std::endl;
}

int main()
{
    const size_t num_structs = 1000000;

    // Initialize vectors of structs
    std::vector<SmallStruct> small_structs(num_structs);
    std::vector<LargeStruct> large_structs(num_structs);

    // Measure access latency
    std::cout << "\nTesting scattered allocation..." << std::endl;
    auto scattered_small = create_scattered_array<SmallStruct>(num_structs);
    auto scattered_large = create_scattered_array<LargeStruct>(num_structs);
    measure_access_latency(small_structs, "SmallStruct");
    measure_access_latency(large_structs, "LargeStruct");

    return 0;
}