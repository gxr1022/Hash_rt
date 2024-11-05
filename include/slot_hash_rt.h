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
#include <utility>
#include <string_view>
#include <cstring>

#define HASH_INIT_BUCKET_NUM (1000000)
#define HASH_ASSOC_NUM (2)
#define MAX_KEY_LENGTH 64
#define MAX_VALUE_LENGTH 128 

using namespace verona::rt;
using namespace verona::cpp;
using namespace std;

struct Item {
    char key[MAX_KEY_LENGTH];
    char value[MAX_VALUE_LENGTH];
    bool occupied;  

    Item() : occupied(false) {
        key[0] = '\0';
        value[0] = '\0';
    }

    bool insert(const char *key, const char *value) {
        if (occupied) {
            return false;
        }

        if (strlen(key) >= MAX_KEY_LENGTH || strlen(value) >= MAX_VALUE_LENGTH) {
            return false;
        }

        strncpy(this->key, key, MAX_KEY_LENGTH - 1);
        strncpy(this->value, value, MAX_VALUE_LENGTH - 1);
        
        this->key[MAX_KEY_LENGTH - 1] = '\0';
        this->value[MAX_VALUE_LENGTH - 1] = '\0';
        
        occupied = true;
        return true;
    }
};

static inline int hashFunction(char* key, int dirCap)
{
    size_t key_len = strlen(key);
    uint64_t hashValue = string_key_hash_computation(key, key_len, 0, 0);
    return hashValue % dirCap;
}

class ExtendibleHash
{
private:
    int dirCapacity;
    cown_ptr<Item>** directory;  

public:
    ExtendibleHash(int dirCap, int bucketCap) : dirCapacity(dirCap)
    {
        directory = new cown_ptr<Item>*[dirCapacity];
        auto start_time = std::chrono::steady_clock::now();
        
        for (int i = 0; i < dirCapacity; ++i)
        {
            directory[i] = new cown_ptr<Item>[HASH_ASSOC_NUM];
            for (int j = 0; j < HASH_ASSOC_NUM; ++j) {
                directory[i][j] = make_cown<Item>();
            }
        }
        
        auto current_time = std::chrono::steady_clock::now();
        auto duration_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(current_time - start_time).count();
        std::cout << "Init time:" << duration_ns << std::endl;
    }

    ~ExtendibleHash() {
        for (int i = 0; i < dirCapacity; ++i) {
            delete[] directory[i];
        }
        delete[] directory;
    }

    void insert(char* key, char* value)
    {
        int hashValue = hashFunction(key, dirCapacity);
        
        if (hashValue >= dirCapacity) {
            return;
        }

        for (int i = 0; i < HASH_ASSOC_NUM; ++i) {
            when(directory[hashValue][i]) << [=](acquired_cown<Item> slotAcq) mutable
            {
                if (!slotAcq->occupied) {
                    // cout << "inserting key:" << key << " value:" << value << endl;
                    slotAcq->insert(key, value);
                    return;
                }
            };
        }
        return;
    }

    void printStatus()
    {
        for (size_t i = 0; i < dirCapacity; ++i)
        {
            std::cout << "Bucket " << i << ": ";
            for (int j = 0; j < HASH_ASSOC_NUM; ++j) {
                when(directory[i][j]) << [](acquired_cown<Item> slotAcq) mutable
                {
                    if (slotAcq->occupied) {
                        std::cout << "{" << slotAcq->key << ": " << slotAcq->value << "} ";
                    } else {
                        std::cout << "[-] ";
                    }
                };
            }
            std::cout << std::endl;
        }
    }

    string find(char* key)
    {
        int hashValue = hashFunction(key, dirCapacity);
        string result = "not found";

        if (hashValue >= dirCapacity) {
            return result;
        }

        for (int i = 0; i < HASH_ASSOC_NUM; ++i) {
            when(directory[hashValue][i]) << [&result, key](acquired_cown<Item> slotAcq) mutable
            {
                if (slotAcq->occupied && strcmp(slotAcq->key, key) == 0) {
                    result = string(slotAcq->value);
                }
            };
            
            if (result != "not found") {
                break;
            }
        }
        
        return result;
    }

    void erase(char* key)
    {
        int hashValue = hashFunction(key, dirCapacity);
        
        if (hashValue >= dirCapacity) {
            return;
        }

        for (int i = 0; i < HASH_ASSOC_NUM; ++i) {
            when(directory[hashValue][i]) << [key](acquired_cown<Item> slotAcq) mutable
            {
                if (slotAcq->occupied && strcmp(slotAcq->key, key) == 0) {
                    slotAcq->occupied = false;
                    slotAcq->key[0] = '\0';
                    slotAcq->value[0] = '\0';
                }
            };
        }
    }
};