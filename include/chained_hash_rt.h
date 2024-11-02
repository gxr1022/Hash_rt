#pragma once
#include <cpp/when.h>
#include <unordered_map>
#include <vector>
#include <iostream>
#include <memory>
#include <atomic>
#include "../util/util.hpp"
#include "debug/harness.h"
#include "test/opt.h"
#include "test/xoroshiro.h"
#include "verona.h"
#include "test/opt.h"
#include <utility>

#define HASH_INIT_BUCKET_NUM (1000000)
#define HASH_ASSOC_NUM (1)

using namespace verona::rt;
using namespace verona::cpp;
using namespace std;

inline size_t WORK_USEC = 0;

class Bucket
{
public:
    std::pair<char *, char *> slots[HASH_ASSOC_NUM];
    uint64_t idx;

    Bucket() : idx(0) {}

    bool isFull() const
    {
        return idx >= HASH_ASSOC_NUM;
    }

    bool insert(const char *key, const char *value)
    {
        if (isFull())
        {
            return false;
        }

        char *key_copy = new char[strlen(key) + 1];
        std::strcpy(key_copy, key);

        char *value_copy = new char[strlen(value) + 1];
        std::strcpy(value_copy, value);

        slots[idx] = std::make_pair(key_copy, value_copy);
        idx++;
        return true;
    }

    ~Bucket()
    {

        for (uint64_t i = 0; i < idx; ++i)
        {
            delete[] slots[i].first;  
            delete[] slots[i].second; 
        }
    }
};

// class Bucket
// {
// public:
//     unordered_map<string, string> kvStore;
//     int capacity;
//     Bucket(int cap) : capacity(cap) {}

//     bool isFull()
//     {
//         return kvStore.size() >= capacity;
//     }

//     bool insert(string key, string value)
//     {
//         if (kvStore.find(key) != kvStore.end())
//         {
//             kvStore[key] = value;
//             return true;
//         }
//         if (!isFull())
//         {
//             kvStore[key] = value;
//             return true;
//         }
//         return false;
//     }

//     bool remove(string key)
//     {
//         auto it = kvStore.find(key);
//         if (it != kvStore.end())
//         {
//             kvStore.erase(it);
//             return true;
//         }
//         return false;
//     }

//     string get(string key)
//     {
//         auto it = kvStore.find(key);
//         if (it != kvStore.end())
//         {
//             return it->second;
//         }
//         return "not found";
//     }
// };

static inline int hashFunction(char* key, int dirCap)
{
    // return stoi(key) % dirCap;
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

    void insert(char* key, char* value)
    {
        int hashValue = hashFunction(key, dirCapacity);
        // cout<< "bucketID:" << hashValue<<endl;
        bool inserted = false;

        if (hashValue < directory.size())
        {
            when(directory[hashValue]) << [=](acquired_cown<Bucket> bucketAcq) mutable
            {
                inserted = bucketAcq->insert(key, value);
                // busy_loop(WORK_USEC);
            };
        }
        return;
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
        for (size_t i = 0; i < directory.size(); ++i)
        {
            when(directory[i]) << [i](acquired_cown<Bucket> bucketAcq) mutable
            {
                std::stringstream output;
                for (const auto &kv : bucketAcq->slots)
                {
                    output << "{" << kv.first << ": " << kv.second << "} ";
                }
                std::cout << "Bucket " << i << ": " << output.str() << std::endl;
            };
        }
    }
};