#pragma once
#include <cpp/when.h>
#include <unordered_map>
#include <vector>
#include <iostream>
#include <memory>
#include <atomic>

using namespace verona::rt;
using namespace verona::cpp;
using namespace std;

// 定义每个桶
class Bucket {
public:
    unordered_map<int, int> kvStore;
    int capacity;
    int prefix;

    Bucket(int cap, int pfix) : capacity(cap), prefix(pfix) {}

    bool isFull() {
        return kvStore.size() >= capacity;
    }

    bool insert(int key, int value) {
        if (kvStore.find(key) != kvStore.end()) {
            kvStore[key] = value; 
            return true;
        }
        if (!isFull()) {
            kvStore[key] = value;  
            return true;
        }
        return false;  
    }

    bool remove(int key) {
        auto it = kvStore.find(key);
        if (it != kvStore.end()) {
            kvStore.erase(it);
            return true;
        }
        return false;
    }

    int get(int key) {
        auto it = kvStore.find(key);
        if (it != kvStore.end()) {
            return it->second;
        }
        return -1;  
    }
};

// 定义目录
struct Directory
{
    cown_ptr<Bucket> bucket;   // 目录持有桶的所有权
    std::atomic<int> localDepth;
};

// 定义可扩展哈希表
class ExtendibleHash {
private:
    std::atomic<int> globalDepth;
    int bucketCapacity;
    vector<cown_ptr<Directory>> directory;  // 使用 cown_ptr 管理目录

    int hashFunction(int key) {
        return key & ((1 << globalDepth) - 1);
    }

public:
    ExtendibleHash(int initialDepth, int bucketCap) 
    : globalDepth(initialDepth), bucketCapacity(bucketCap) {
        directory.resize(1 << initialDepth);
        for (int i = 0; i < (1 << initialDepth); ++i) {
            directory[i] = make_cown<Directory>();
            when(directory[i]) << [=](acquired_cown<Directory> dirAcq) mutable {
                dirAcq->bucket = make_cown<Bucket>(bucketCap, i); 
                dirAcq->localDepth.store(initialDepth); 
            };
        }
    }

    // 插入键值对
    void insert(int key, int value) {
        int hashValue = hashFunction(key);

        when(directory[hashValue]) << [=](acquired_cown<Directory> dirAcq) mutable {
            auto& bucket = dirAcq->bucket;
            
            when(bucket) << [=](acquired_cown<Bucket> bucketAcq) mutable {
                bool inserted = bucketAcq->insert(key, value);
                if (!inserted) {
                    // 桶已满，处理桶分裂逻辑（如果需要）
                    // TODO: 实现桶分裂和目录扩展
                    std::cerr << "Bucket is full, consider splitting" << std::endl;
                }
            };
        };
    }

    // 查找键值对
    int find(int key) {
        int hashValue = hashFunction(key);
        when(directory[hashValue]) << [=](acquired_cown<Directory> dirAcq) mutable {
            auto& bucket = dirAcq->bucket;

            when(bucket) << [=](acquired_cown<Bucket> bucketAcq) mutable {
                int result = bucketAcq->get(key);
                if (result != -1) {
                    return result; // 找到键，返回结果
                } else {
                    return -1;  // 未找到键
                }
            };
        };
    }

    // 删除键值对
    void erase(int key) {
        int hashValue = hashFunction(key);
        when(directory[hashValue]) << [=](acquired_cown<Directory> dirAcq) mutable {
            auto& bucket = dirAcq->bucket;

            when(bucket) << [=](acquired_cown<Bucket> bucketAcq) mutable {
                bool removed = bucketAcq->remove(key);
                if (!removed) {
                    std::cerr << "Key not found in bucket" << std::endl;
                }
            };
        };
    }

    void printStatus() {
        std::cout << "Global Depth: " << globalDepth << std::endl;

        for (size_t i = 0; i < directory.size(); ++i) {
            when(directory[i]) << [i](acquired_cown<Directory> dirAcq) mutable {
                std::cout << "Bucket " << i << " (Local Depth: " << dirAcq->localDepth << "): ";
                when(dirAcq->bucket) << [i](acquired_cown<Bucket> bucketAcq) mutable {
                    std::stringstream output;
                    for (const auto& kv : bucketAcq->kvStore) {
                        output << "{" << kv.first << ": " << kv.second << "} ";
                    }
                    std::cout << "Bucket " << i << ": " << output.str() << std::endl;
                };
            };
        }
    }


};
