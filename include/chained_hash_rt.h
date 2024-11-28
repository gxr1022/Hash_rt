#pragma once
#include <cpp/when.h>
#include <unordered_map>
#include <vector>
#include <iostream>
#include <memory>
#include <atomic>
#include "debug/harness.h"
#include "test/opt.h"
#include "test/xoroshiro.h"
#include "../util/util.hpp"
#include "verona.h"
#include "test/opt.h"
#include <utility>
#include <string_view>
#include <cstring>

#define HASH_INIT_BUCKET_NUM (5000000)
#define HASH_ASSOC_NUM (2)
#define MAX_KEY_LENGTH 8
#define MAX_VALUE_LENGTH 100 

using namespace verona::rt;
using namespace verona::cpp;
using namespace std;


struct KeyValue {
    char key[MAX_KEY_LENGTH];
    char value[MAX_VALUE_LENGTH];
    bool occupied;  

    KeyValue() : occupied(false) {
        key[0] = '\0';
        value[0] = '\0';
    }
};

class Bucket
{
public:
    KeyValue slots[HASH_ASSOC_NUM];
    uint64_t idx;
    Bucket() : idx(0) {}
    bool isFull() const
    {
        return idx >= HASH_ASSOC_NUM;
    }

    bool insert(const char* key, const char* value)
    {
        if (isFull())
        {
            // cout << "bucket is full" << endl;
            return false;
        }

        if (strlen(key) >= MAX_KEY_LENGTH || strlen(value) >= MAX_VALUE_LENGTH) {
            return false;
        }

        strncpy(slots[idx].key, key, MAX_KEY_LENGTH - 1);
        strncpy(slots[idx].value, value, MAX_VALUE_LENGTH - 1);
        
        slots[idx].key[MAX_KEY_LENGTH - 1] = '\0';
        slots[idx].value[MAX_VALUE_LENGTH - 1] = '\0';
        
        slots[idx].occupied = true;
        // cout << "inserted successfully:" << slots[idx].key << " value:" << slots[idx].value << endl;
        idx++;
        return true;
    }

    ~Bucket()
    {
    }
};

static inline int hashFunction(const char* key, int dirCap)
{
    size_t key_len = strlen(key);
    uint64_t hashValue = string_key_hash_computation(key,key_len , 0, 0);
    return hashValue % dirCap;
}

class ExtendibleHash
{
private:
    int dirCapacity;
    int bucketCapacity;
    vector<cown_ptr<Bucket>> directory;
    static inline std::atomic<uint64_t> completed_inserts{0};
    static inline std::atomic<uint64_t> when_schedule_time{0};
    static inline std::atomic<uint64_t> when_schedule_count{0};

public:
    ExtendibleHash(int dirCap, int bucketCap) : dirCapacity(dirCap), bucketCapacity(bucketCap)
    {
        directory.resize(dirCapacity);
        auto start_time = std::chrono::steady_clock::now();
        for (int i = 0; i < dirCapacity; ++i)
        {
            directory[i] = make_cown<Bucket>();
        }
        auto current_time = std::chrono::steady_clock::now();
        auto duration_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(current_time - start_time).count();
        std::cout << "Init time:" << duration_ns << std::endl;
    }

    void insert(const char* key, const char* value)
    {
        int hashValue = hashFunction(key, dirCapacity);
        bool inserted = false;

        if (hashValue < directory.size())
        {
            // auto start_time = std::chrono::steady_clock::now();
            
            when(directory[hashValue]) << [=](acquired_cown<Bucket> bucketAcq) mutable
            {
                // auto end_time = std::chrono::steady_clock::now();
                // auto schedule_time = std::chrono::duration_cast<std::chrono::nanoseconds>
                //                    (end_time - start_time).count();
                // when_schedule_time.fetch_add(schedule_time);
                // when_schedule_count.fetch_add(1);
                
                inserted = bucketAcq->insert(key, value);
                if (inserted) {
                    completed_inserts.fetch_add(1);
                }
            };
        }
    }

    std::pair<uint64_t, uint64_t> get_when_stats() const {
        return {when_schedule_time.load(), when_schedule_count.load()};
    }

    // string find(string key)
    // {
    //     int hashValue = hashFunction(key, dirCapacity);
    //     when(directory[hashValue]) << [=](acquired_cown<Bucket> bucketAcq) mutable
    //     {
    //         string result = bucketAcq->get(key);
    //         if (result != string("not found"))
    //         {
    //             return result;
    //         }
    //         else
    //         {
    //             return string("not found");
    //         }
    //     };
    //     return string("not found");
    // }

    // void erase(string key)
    // {
    //     int hashValue = hashFunction(key, dirCapacity);
    //     when(directory[hashValue]) << [=](acquired_cown<Bucket> bucketAcq) mutable
    //     {
    //         bool removed = bucketAcq->remove(key);
    //         if (!removed)
    //         {
    //             std::cerr << "Key not found in bucket" << std::endl;
    //         }
    //     };
    // }

    void printStatus()
    {
        // std::cout<< "dirCapacity:" << dirCapacity << endl;
        for (size_t i = 0; i < directory.size(); ++i)
        {
            when(directory[i]) << [i](acquired_cown<Bucket> bucketAcq) mutable
            {
                std::stringstream output;
                for (const auto &kv : bucketAcq->slots)
                {
                    output << "{" << kv.key << ": " << kv.value << "} ";
                }
                std::cout << "Bucket " << i << ": " << output.str() << std::endl;
            };
        }
    }

    size_t get_completed_inserts() const { return completed_inserts.load(); }
};