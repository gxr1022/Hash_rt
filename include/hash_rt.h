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


struct Directory
{
    cown_ptr<Bucket> bucket; 
    int localDepth;
};


// class ExtendibleHash : public VCown<ExtendibleHash>{
class ExtendibleHash {
private:
    int globalDepth;
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
                dirAcq->localDepth = initialDepth; 
            };
        }
    }

    void insert(int key, int value) {
        int hashValue = hashFunction(key);
        when(directory[hashValue]) << [=](acquired_cown<Directory> dirAcq) mutable {
            auto& bucket = dirAcq->bucket;
            
            when(bucket) << [=](acquired_cown<Bucket> bucketAcq) mutable {
                bool inserted = bucketAcq->insert(key, value);
                if (!inserted) {
                    // to do……  split bucket
                    std::cerr << "Bucket is full, consider splitting" << std::endl;
                }
            };
        };
    }

    int find(int key) {
        int hashValue = hashFunction(key);
        when(directory[hashValue]) << [=](acquired_cown<Directory> dirAcq) mutable {
            auto& bucket = dirAcq->bucket;

            when(bucket) << [=](acquired_cown<Bucket> bucketAcq) mutable {
                int result = bucketAcq->get(key);
                if (result != -1) {
                    return result; 
                } else {
                    return -1;  
                }
            };
        };
        return -1;
    }


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

    // void splitBucket(int dir_prefix) {    
    //     if(directory[dir_prefix]->localDepth == globalDepth && !isResizing.load())
    //     {
    //         unique_lock<shared_mutex> lock(globalMutex);
    //         isResizing.store(true);
    //         int cap = directory.size();
    //         directory.resize(cap * 2);  
            
    //         for (int i = 0; i < cap; ++i) {
                
    //             directory[i + cap] = make_shared<Directory>();
    //             // unique_lock<shared_mutex> dirLock(directory[i + cap]->directoryMutex);
    //             directory[i + cap]->bucket = directory[i]->bucket;
    //             directory[i + cap]->prefix = i + cap;
    //             directory[i + cap]->localDepth.store(directory[i]->localDepth.load());
    //         }
    //         globalDepth++;
    //     }
    //     // isResizing.store(false);
    //     // resizeCondVar.notify_all();
        
    //     int localDepth;
    //     shared_ptr<Bucket> oldBucket;
    //     {
    //         shared_lock<shared_mutex> readLock(directory[dir_prefix]->directoryMutex);
    //         localDepth = directory[dir_prefix]->localDepth;
    //         oldBucket = directory[dir_prefix]->bucket;
    //     }
        
    //     if (!oldBucket) {
    //         std::cerr << "Error: oldBucket is null!" << std::endl;
    //         return;
    //     }

    //     int cur_bucket_prefix = oldBucket->prefix;
    //     int new_bucket_prefix = cur_bucket_prefix + (1 << localDepth);
    //     shared_ptr<Bucket> newBucket = make_shared<Bucket>(bucketCapacity, new_bucket_prefix);
    //     shared_ptr<Bucket> newBucket_o = make_shared<Bucket>(bucketCapacity, cur_bucket_prefix);

    //     //Re-assign kv pairs into two new buckets 
    //     {
    //         std::unique_lock<std::mutex> oldBucketLock(oldBucket->mtx); // exclusive lock
    //         for (auto& pair : oldBucket->kvStore) {
    //             int newHash = hashFunction(pair.first);
    //             if (newHash == oldBucket->prefix) {
    //                 newBucket_o->insert(pair.first, pair.second);  
    //             } else {
    //                 newBucket->insert(pair.first, pair.second);  
    //             }
    //         }
    //         oldBucket->kvStore.clear();
    //     }

    //     // modify the bucket pointer of directory entry.
    //     unique_lock<shared_mutex> lock(globalMutex);
    //     for (int i = 0; i < directory.size(); ++i) {
    //         if (!directory[i] || !directory[i]->bucket) {
    //             std::cerr << "Error: Directory or Bucket is null!" << std::endl;
    //             continue;
    //         }
    //         if (directory[i]->bucket == oldBucket) {
    //             // std::unique_lock<std::shared_mutex> dirLock(directory[i]->directoryMutex); //exlusive lock
    //             if (i & (1 << localDepth)) {
    //                 directory[i]->bucket = newBucket_o;
    //             } else {
    //                 directory[i]->bucket = newBucket;
    //             }
    //             directory[i]->localDepth++;
    //         }
            
    //     }
       
    // }



};
