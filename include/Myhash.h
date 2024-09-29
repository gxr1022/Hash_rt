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
#include "verona.h"
#include "test/opt.h"

using namespace verona::rt;
using namespace verona::cpp;
using namespace std;

// Key 类作为 Cown 对象
class Key : public VCown<Key>
{
public:
    int key;

    Key(int v) : key(v) {}

    void print() {
        std::cout << "Key: " << key << std::endl;
    }
};

class Bucket {
public:
    unordered_map<Cown*, int> kvStore;  // 使用 Cown* 作为键类型
    int capacity;
    int prefix;

    Bucket(int cap, int pfix) : capacity(cap), prefix(pfix) {}

    bool isFull() {
        return kvStore.size() >= capacity;
    }

    bool insert(Cown* key, int value) {
        if (kvStore.find(key) != kvStore.end()) {
            kvStore[key] = value;
            return true;
        }
        if (!isFull()) {
            Cown::acquire(key);  // 手动增加引用计数
            kvStore[key] = value;
            return true;
        }
        return false;
    }

    bool remove(Cown* key) {
        auto it = kvStore.find(key);
        if (it != kvStore.end()) {
            kvStore.erase(it);
            Cown::release(ThreadAlloc::get(), key);  // 手动减少引用计数
            return true;
        }
        return false;
    }

    int get(Cown* key) {
        auto it = kvStore.find(key);
        if (it != kvStore.end()) {
            return it->second;
        }
        return -1;
    }
};

struct Directory
{
    cown_ptr<Bucket> bucket;
    int localDepth;
};

class ExtendibleHash : public VCown<ExtendibleHash> {
private:
    int globalDepth;
    int bucketCapacity;
    vector<cown_ptr<Directory>> directory;

    int hashFunction(Cown* key) {
        schedule_lambda(key, [=](acquired_cown<Key> keyAcq) mutable {
            int keyVal = keyAcq.;
            int hashValue = keyVal & ((1 << globalDepth) - 1);
            callback(hashValue);  // 通过回调函数返回哈希值
        });
    }

public:
    ExtendibleHash(int initialDepth, int bucketCap)
        : globalDepth(initialDepth), bucketCapacity(bucketCap) {
        directory.resize(1 << initialDepth);
        for (int i = 0; i < (1 << initialDepth); ++i) {
            directory[i] = make_cown<Directory>();
            when(directory[i]) << [=](acquired_cown<Directory> dirAcq) mutable {
                dirAcq->bucket = make_cown<Bucket>(bucketCap, i);
                dirAcq->localDepth = initialDepth;
            };
        }
    }

    void insert(Cown* key, int value) {
        int hashValue = hashFunction(key);
        when(directory[hashValue]) << [=](acquired_cown<Directory> dirAcq) mutable {
            auto& bucket = dirAcq->bucket;
            when(bucket) << [=](acquired_cown<Bucket> bucketAcq) mutable {
                bool inserted = bucketAcq->insert(key, value);
                if (!inserted) {
                    std::cerr << "Bucket is full, consider splitting" << std::endl;
                }
            };
        };
    }

    int find(Cown* key) {
        int hashValue = hashFunction(key);
        when(directory[hashValue]) << [=](acquired_cown<Directory> dirAcq) mutable {
            auto& bucket = dirAcq->bucket;
            when(bucket) << [=](acquired_cown<Bucket> bucketAcq) mutable {
                int result = bucketAcq->get(key);
                if (result != -1) {
                    std::cout << "Found key with value: " << result << std::endl;
                } else {
                    std::cout << "Key not found." << std::endl;
                }
            };
        };
        return -1;
    }

    void erase(Cown* key) {
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
                        Key* key = dynamic_cast<Key*>(kv.first);
                        if (key) {
                            output << "{Key: " << key->key << ", Value: " << kv.second << "} ";
                        }
                    }
                    std::cout << "Bucket " << i << ": " << output.str() << std::endl;
                };
            };
        }
    }
};


// 将 Directory 和 Bucket 都改为 Cown 类型
struct Directory : public VCown<Directory>
{
    cown_ptr<Bucket> bucket;  // 改为 Cown
    std::atomic<int> localDepth;
};

struct Bucket : public VCown<Bucket>
{
    std::unordered_map<int, int> kvStore;
    int capacity;
    int prefix;
    std::mutex mtx;

    Bucket(int cap, int pfix) : capacity(cap), prefix(pfix) {}

    bool insert(int key, int value) {
        if (kvStore.size() >= capacity) {
            return false;
        }
        kvStore[key] = value;
        return true;
    }
};


