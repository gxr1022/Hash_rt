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

class Bucket
{
public:
    unordered_map<int, int> kvStore;
    int capacity;
    int prefix;

    Bucket(int cap, int pfix) : capacity(cap), prefix(pfix) {}

    bool isFull()
    {
        return kvStore.size() >= capacity;
    }

    bool insert(int key, int value)
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

    bool remove(int key)
    {
        auto it = kvStore.find(key);
        if (it != kvStore.end())
        {
            kvStore.erase(it);
            return true;
        }
        return false;
    }

    int get(int key)
    {
        auto it = kvStore.find(key);
        if (it != kvStore.end())
        {
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

static inline int hashFunction(int key, int globalDepth)
{
    return key & ((1 << globalDepth) - 1);
}

// class ExtendibleHash : public VCown<ExtendibleHash>
class ExtendibleHash 
{
private:
    atomic<int> globalDepth;
    // cown_ptr<int> globalDepth;
    int bucketCapacity;
    vector<cown_ptr<Directory>> directory;
    // cown_ptr<ExtendibleHash> self_cown;

public:
    ExtendibleHash(int initialDepth, int bucketCap)
        : globalDepth(initialDepth), bucketCapacity(bucketCap)
    {
        directory.resize(1 << initialDepth);
        for (int i = 0; i < (1 << initialDepth); ++i)
        {
            directory[i] = make_cown<Directory>();
            when(directory[i]) << [=](acquired_cown<Directory> dirAcq) mutable
            {
                dirAcq->bucket = make_cown<Bucket>(bucketCap, i);
                dirAcq->localDepth = initialDepth;
            };
        }
    }

    void insert(int key, int value,  cown_ptr<ExtendibleHash> ht_acq)
    {
        int hashValue = hashFunction(key, globalDepth);
        bool inserted;
        if(hashValue < directory.size()){
        when(directory[hashValue]) << [=](acquired_cown<Directory> dirAcq) mutable
        {
            auto &bucket = dirAcq->bucket;
            int localDepth = dirAcq->localDepth;
            when(bucket) << [=](acquired_cown<Bucket> bucketAcq) mutable
            {
                inserted = bucketAcq->insert(key, value);
            };
            if (!inserted)
            {
                // cown_ptr self = make_cown<ExtendibleHash>(); // It's weird
                // cown_ptr<ExtendibleHash> self_cown = make_cown<ExtendibleHash>(std::move(*ht_acq));
                splitBucket(dirAcq->bucket, dirAcq->localDepth, hashValue, ht_acq);
                // std::cerr << "Bucket is full, consider splitting" << std::endl;
            }
        };
        }
    }

    int find(int key)
    {
        int hashValue = hashFunction(key, globalDepth);
        when(directory[hashValue]) << [=](acquired_cown<Directory> dirAcq) mutable
        {
            auto &bucket = dirAcq->bucket;

            when(bucket) << [=](acquired_cown<Bucket> bucketAcq) mutable
            {
                int result = bucketAcq->get(key);
                if (result != -1)
                {
                    return result;
                }
                else
                {
                    return -1;
                }
            };
        };
        return -1;
    }

    void erase(int key)
    {
        int hashValue = hashFunction(key, globalDepth);
        when(directory[hashValue]) << [=](acquired_cown<Directory> dirAcq) mutable
        {
            auto &bucket = dirAcq->bucket;

            when(bucket) << [=](acquired_cown<Bucket> bucketAcq) mutable
            {
                bool removed = bucketAcq->remove(key);
                if (!removed)
                {
                    std::cerr << "Key not found in bucket" << std::endl;
                }
            };
        };
    }

    void printStatus()
    {
        std::cout << "Global Depth: " << globalDepth.load() << std::endl;

        for (size_t i = 0; i < directory.size(); ++i)
        {
            // if(directory[i]!=nullptr)
            {
            when(directory[i]) << [i](acquired_cown<Directory> dirAcq) mutable
            {
                std::cout << "Bucket " << i << " (Local Depth: " << dirAcq->localDepth << "): ";
                when(dirAcq->bucket) << [i](acquired_cown<Bucket> bucketAcq) mutable
                {
                    std::stringstream output;
                    for (const auto &kv : bucketAcq->kvStore)
                    {
                        output << "{" << kv.first << ": " << kv.second << "} ";
                    }
                    std::cout << "Bucket " << i << ": " << output.str() << std::endl;
                };
            };
            }
        }
    }

    void splitBucket(cown_ptr<Bucket> oldBucket, int localDepth, int dir_indx, cown_ptr<ExtendibleHash> hashtable)
    {

        // cown_ptr<ExtendibleHash> ht_cown = make_cown<ExtendibleHash>(*hashtable);
        // cown_array<Directory> dir_array(directory.data(), directory.size());
        when(hashtable) << [=](acquired_cown<ExtendibleHash> hs_acq) mutable
        {
            // int localDepth = dirs_acq.array[dir_indx]->localDepth;
            int cap = hs_acq->directory.size();
            bool resized = 0;
            if (localDepth == globalDepth)
            {
                hs_acq->directory.resize(cap * 2);
                for (int i = 0; i < cap; ++i)
                {
                    hs_acq->directory[i + cap] = make_cown<Directory>();
                    if (i == dir_indx)
                    {
                        // if (directory[i + cap] != nullptr)
                        {   
                            when(hs_acq->directory[i + cap]) << [=](acquired_cown<Directory> dirAcq_cap)
                            {
                                dirAcq_cap->bucket = oldBucket;
                                dirAcq_cap->localDepth = localDepth;
                            };
                        }
                        
                    }
                    else
                    {
                        // if (directory[i + cap] != nullptr && directory[i] != nullptr)
                        {
                            when(hs_acq->directory[i + cap], directory[i]) << [=](acquired_cown<Directory> dirAcq_cap, acquired_cown<Directory> dirAcq_i)
                            {
                                dirAcq_cap->bucket = dirAcq_i->bucket;
                                dirAcq_cap->localDepth = dirAcq_i->localDepth;
                            };
                        }

                    }
                }
                globalDepth++;
                cap = 2 * cap;
                resized = 1;
            }
        }; 

        when(hashtable, oldBucket) << [=](acquired_cown<ExtendibleHash> hs_acq, acquired_cown<Bucket> oldBucketAcq) mutable
        {
            // cown_ptr<Bucket> oldBucket;
            // oldBucket = dirs_acq.array[dir_indx]->bucket;
            // move kv pairs in oldBucket to new oldBucket.
            // if (oldBucket != nullptr){
            // when(oldBucket) << [=](acquired_cown<Bucket> oldBucketAcq) mutable
            {
                int cur_bucket_prefix = oldBucketAcq->prefix;
                int new_bucket_prefix = cur_bucket_prefix + (1 << localDepth);

                auto newBucket = make_cown<Bucket>(bucketCapacity, new_bucket_prefix);
                auto newBucket_o = make_cown<Bucket>(bucketCapacity, cur_bucket_prefix);

                for (auto &pair : oldBucketAcq->kvStore)
                {
                    int newHash = hashFunction(pair.first, globalDepth);
                    if (newHash == oldBucketAcq->prefix)
                    {
                        when(newBucket_o) << [=](acquired_cown<Bucket> newBucketAcq) mutable
                        {
                            newBucketAcq->insert(pair.first, pair.second);
                        };
                    }
                    else
                    {
                        when(newBucket) << [=](acquired_cown<Bucket> newBucketAcq) mutable
                        {
                            newBucketAcq->insert(pair.first, pair.second);
                        };
                    }
                }
                oldBucketAcq->kvStore.clear();

                for (int i = 0; i < hs_acq->directory.size(); ++i)
                {
                    // if(directory[i] != nullptr){
                    when(hs_acq->directory[i]) << [=](acquired_cown<Directory> subDirAcq) mutable
                    {
                        if (subDirAcq->bucket == oldBucket)
                        {
                            if (i & (1 << localDepth))
                            {
                                subDirAcq->bucket = newBucket_o;
                            }
                            else
                            {
                                subDirAcq->bucket = newBucket;
                            }
                            subDirAcq->localDepth++;
                        }
                    };
                    
                }
            }
        }; 
        
    }
};
