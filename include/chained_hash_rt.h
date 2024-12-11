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
#include <functional>

#define HASH_INIT_BUCKET_NUM (5000000)
// #define HASH_INIT_BUCKET_NUM (100)
#define HASH_ASSOC_NUM (2)
#define MAX_KEY_LENGTH 8
#define MAX_VALUE_LENGTH 100

using namespace verona::rt;
using namespace verona::cpp;
using namespace std;

struct DataNode
{
    char key[MAX_KEY_LENGTH];
    char value[MAX_VALUE_LENGTH];
    DataNode *next;
    DataNode() : next(nullptr)
    {
        key[0] = '\0';
        value[0] = '\0';
    }

    DataNode(const char *k, const char *v) : next(nullptr)
    {
        strncpy(key, k, MAX_KEY_LENGTH - 1);
        strncpy(value, v, MAX_VALUE_LENGTH - 1);
        key[MAX_KEY_LENGTH - 1] = '\0';
        value[MAX_VALUE_LENGTH - 1] = '\0';
    }
};

class Bucket
{
public:
    DataNode *head;
    Bucket() : head(nullptr) {}

    bool insert(const char *key, const char *value)
    {
        if (strlen(key) >= MAX_KEY_LENGTH || strlen(value) >= MAX_VALUE_LENGTH)
        {
            return false;
        }
        DataNode *newNode = new DataNode(key, value);
        if (newNode == nullptr)
            return false;

        newNode->next = head;
        head = newNode;
        return true;
    }

    const char *find(const char *key)
    {
        DataNode *current = head;
        while (current != nullptr)
        {
            if (strcmp(current->key, key) == 0)
            {
                return current->value;
            }
            current = current->next;
        }
        return nullptr;
    }

    ~Bucket()
    {
        DataNode *current = head;
        while (current != nullptr)
        {
            DataNode *next = current->next;
            delete current;
            current = next;
        }
    }
};

static inline int hashFunction(const char *key, int dirCap)
{
    size_t key_len = strlen(key);
    uint64_t hashValue = string_key_hash_computation(key, key_len, 0, 0);
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
    // struct UpdateBehaviour
    // {
    //     char *key;
    //     char *value;
    //     void operator()()
    //     {
    //         int hashValue = hashFunction(key, dirCapacity);
    //         when(directory[hashValue]) << [=](acquired_cown<Bucket> bucketAcq) mutable
    //         {
    //             bucketAcq->insert(key, value);
    //             completed_inserts.fetch_add(1);
    //         }
    //     }
    // };

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


    void insert(const char *key, const char *value)
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
                completed_inserts.fetch_add(1);
            };
        }
    }

    std::pair<uint64_t, uint64_t> get_when_stats() const
    {
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
                DataNode *current = bucketAcq->head;
                while (current != nullptr)
                {
                    output << "{" << current->key << ": " << current->value << "} ";
                    current = current->next;
                }
                std::cout << "Bucket " << i << ": " << output.str() << std::endl;
            };
        }
    }

    size_t get_completed_inserts() const { return completed_inserts.load(); }
};