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

using namespace verona::rt;
using namespace verona::cpp;
using namespace std;

class Bucket
{
public:
    unordered_map<string, string> kvStore;
    int capacity;

    Bucket(int cap) : capacity(cap) {}

    bool isFull()
    {
        return kvStore.size() >= capacity;
    }

    bool insert(string key, string value)
    {
        if (kvStore.find(key) != kvStore.end())
        {
            kvStore[key] = value;
            return true;
        }
        if (!isFull())
        {
            kvStore[key] = value;
            return true;
        }
        return false;
    }

    bool remove(string key)
    {
        auto it = kvStore.find(key);
        if (it != kvStore.end())
        {
            kvStore.erase(it);
            return true;
        }
        return false;
    }

    string get(string key)
    {
        auto it = kvStore.find(key);
        if (it != kvStore.end())
        {
            return it->second;
        }
        return "not found";
    }
};

static inline int hashFunction(string key, int dirCap)
{
    // uint64_t hashValue = VariableLengthHash(key.data(), key.size(), 0);
    // return hashValue % dirCap;
    return stoi(key) % dirCap;
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
            directory[i] = make_cown<Bucket>(bucketCap);
        }
        auto current_time = std::chrono::steady_clock::now();
        auto duration_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(current_time - start_time).count();
        std::cout << "Init time:" << duration_ns << std::endl;
    }

    void insert(string key, string value)
    {
        int hashValue = hashFunction(key, dirCapacity);
        // cout<< hashValue<<endl;
        bool inserted = false;
        if (hashValue < directory.size())
        {
            when(directory[hashValue]) << [=](acquired_cown<Bucket> bucketAcq) mutable
            {
                inserted = bucketAcq->insert(key, value);
                // if (inserted)
                // {
                //     std::cout << "successful!" << std::endl;
                //     std::cout << "Inside Lambda: key=" << key << ", value=" << value << std::endl;
                //     when(directory[hashValue]) << [hashValue](acquired_cown<Bucket> bucketAcq) mutable
                //     {
                //         std::stringstream output;
                //         for (const auto &kv : bucketAcq->kvStore)
                //         {
                //             output << "{" << kv.first << ": " << kv.second << "} ";
                //         }
                //         std::cout << "Bucket " << hashValue << ": " << output.str() << std::endl;
                //     };
                // }
                // else
                // {
                //     // std::cout<<"failed!"<<std::endl;
                //     // std::cout << "Inside Lambda: key=" << key << ", value=" << value << std::endl;
                // }
            };
        }
        return;
    }

    string find(string key)
    {
        int hashValue = hashFunction(key, dirCapacity);
        when(directory[hashValue]) << [=](acquired_cown<Bucket> bucketAcq) mutable
        {
            string result = bucketAcq->get(key);
            if (result != string("not found"))
            {
                return result;
            }
            else
            {
                return string("not found");
            }
        };
        return string("not found");
    }

    void erase(string key)
    {
        int hashValue = hashFunction(key, dirCapacity);
        when(directory[hashValue]) << [=](acquired_cown<Bucket> bucketAcq) mutable
        {
            bool removed = bucketAcq->remove(key);
            if (!removed)
            {
                std::cerr << "Key not found in bucket" << std::endl;
            }
        };
    }

    void printStatus()
    {
        for (size_t i = 0; i < directory.size(); ++i)
        {
            // if(directory[i]!=nullptr)
            {
                when(directory[i]) << [i](acquired_cown<Bucket> bucketAcq) mutable
                {
                    std::stringstream output;
                    for (const auto &kv : bucketAcq->kvStore)
                    {
                        output << "{" << kv.first << ": " << kv.second << "} ";
                    }
                    std::cout << "Bucket " << i << ": " << output.str() << std::endl;
                };
            }
        }
    }
};