#pragma once
#include <iostream>
#include <vector>
#include <unordered_map>
#include <cmath>
#include <thread>
#include <mutex>
#include <memory> 
#include <shared_mutex>


using namespace std;

class Bucket {
public:
    unordered_map<int, int> kvStore;
    int capacity;
    int prefix;
    mutex mtx; 

    Bucket(int cap, int pfix) : capacity(cap), prefix(pfix) {}

    bool isFull() {
        // lock_guard<mutex> lock(mtx); 
        return kvStore.size() >= capacity;
    }

    bool insert(int key, int value) {
        lock_guard<mutex> lock(mtx);  
        if (!kvStore.empty() && kvStore.find(key) != kvStore.end()) {
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
        lock_guard<mutex> lock(mtx);
        if (kvStore.find(key) != kvStore.end()) {
            kvStore.erase(key);
            return true;
        }
        return false; 
    }

    int get(int key) {
        lock_guard<mutex> lock(mtx);  
        if (kvStore.find(key) != kvStore.end()) {
            return kvStore[key];
        }
        return -1; 
    }
};

struct Directory
{
    shared_ptr<Bucket> bucket;
    int prefix;
    std::atomic<int> localDepth;
    shared_mutex directoryMutex;
};

class ExtendibleHash {
private:
    std::atomic<int> globalDepth;
    int bucketCapacity;
    vector<shared_ptr<Directory>> directory;
    mutex globalMutex;   

    int hashFunction(int key) {
        return key & ((1 << globalDepth) - 1);
    }

    void splitBucket(int dir_prefix) {
    // 加全局锁保护目录和全局深度的修改
    std::unique_lock<std::mutex> globalLock(globalMutex);

    int localDepth = directory[dir_prefix]->localDepth;
    if (localDepth == globalDepth) {
        int cap = directory.size();
        directory.resize(cap * 2);  
        globalDepth++;  

        for (int i = 0; i < cap; ++i) {
            directory[i + cap] = std::make_shared<Directory>();
            std::unique_lock<std::shared_mutex> writeLock(directory[i + cap]->directoryMutex);
            directory[i + cap]->bucket = directory[i]->bucket;
            directory[i + cap]->prefix = i + cap;
            directory[i + cap]->localDepth = localDepth;
        }
    }
    globalLock.unlock();

  
    shared_ptr<Bucket> oldBucket = directory[dir_prefix]->bucket;
    if (!oldBucket) {
        std::cerr << "Error: oldBucket is null!" << std::endl;
        return;
    }

    int cur_bucket_prefix = oldBucket->prefix;
    int new_bucket_prefix = cur_bucket_prefix + (1 << localDepth);
    shared_ptr<Bucket> newBucket = make_shared<Bucket>(bucketCapacity, new_bucket_prefix);
    shared_ptr<Bucket> newBucket_o = make_shared<Bucket>(bucketCapacity, cur_bucket_prefix);

    
    {
        std::unique_lock<std::mutex> oldBucketLock(oldBucket->mtx);
        for (auto& pair : oldBucket->kvStore) {
            int newHash = hashFunction(pair.first);
            if (newHash == oldBucket->prefix) {
                newBucket_o->insert(pair.first, pair.second);  // 插入旧桶
            } else {
                newBucket->insert(pair.first, pair.second);  // 插入新桶
            }
        }
        oldBucket->kvStore.clear();
    }


    for (int i = 0; i < directory.size(); ++i) {
        if (!directory[i] || !directory[i]->bucket) {
            std::cerr << "Error: Directory or Bucket is null!" << std::endl;
            continue;
        }
        if (directory[i]->bucket == oldBucket) {
            std::unique_lock<std::mutex> bucketLock(directory[i]->bucket->mtx);
            if (i & (1 << localDepth)) {
                directory[i]->bucket = newBucket_o;
            } else {
                directory[i]->bucket = newBucket;
            }
        }
        directory[i]->localDepth++;
    }
}
    // void splitBucket(int dir_prefix) {
        
    //     int localDepth;
    //     int cap;

      
    //     std::unique_lock<std::mutex> globalLock(globalMutex);

    //     localDepth = directory[dir_prefix]->localDepth;
    //     if (localDepth == globalDepth) {
    //         cap = directory.size();
    //         directory.resize(cap * 2);
    //         globalDepth++;
    //         for (int i = 0; i < cap; ++i) {
    //             directory[i + cap] = std::make_shared<Directory>();
    //             // directory[i + cap]->directoryMutex.lock(); // lock
    //             std::unique_lock<std::shared_mutex> writeLock(directory[i + cap]->directoryMutex);
    //             directory[i + cap]->bucket = directory[i]->bucket;
    //             directory[i + cap]->prefix = i + cap;
    //             directory[i + cap]->localDepth = localDepth;
    //             // directory[i + cap]->directoryMutex.unlock(); //lock
    //         }
    //     }
    //     globalLock.unlock();

    //     // relocate kv pairs frome old bucket to new bucket.
        
    //     int cur_bucket_prefix = directory[dir_prefix]->bucket->prefix;
    //     int new_bucket_prefix = cur_bucket_prefix + (1 << localDepth);
        
    //     shared_ptr<Bucket> newBucket;
    //     shared_ptr<Bucket> newBucket_o;
    //     newBucket = make_shared<Bucket>(bucketCapacity, new_bucket_prefix);
    //     newBucket_o = make_shared<Bucket>(bucketCapacity, cur_bucket_prefix);
        
    //     shared_ptr<Bucket> oldBucket = directory[dir_prefix]->bucket;
    //     if (!oldBucket) {
    //         std::cerr << "Error: oldBucket is null!" << std::endl;
    //         return;  // 如果 oldBucket 为空，直接返回或处理错误
    //     }
    //     {
    //         std::unique_lock<std::mutex> oldBucketLock(oldBucket->mtx);
    //         for (auto& pair : oldBucket->kvStore) {
    //             int newHash = hashFunction(pair.first);
    //             if(newHash == oldBucket->prefix)
    //                 newBucket_o->insert(pair.first, pair.second); // insert into old bucket.
    //             else
    //                 newBucket->insert(pair.first, pair.second);
    //         }
    //         oldBucket->kvStore.clear();

    //     }
        

    //     // update the bucket point of directory
    //     for (int i = 0; i < directory.size(); ++i) 
    //     {
    //         if (!directory[i]) {
    //             std::cerr << "Error: Directory at index " << i << " is null!" << std::endl;
    //             continue;  // 如果目录项为空，跳过
    //         }
    //         if (!directory[i]->bucket) {
    //             std::cerr << "Error: Bucket at directory[" << i << "] is null!" << std::endl;
    //             continue;  // 如果为空，跳过这个条目
    //         }
    //         if (directory[i]->bucket == oldBucket) 
    //         {
                
    //            std::unique_lock<std::mutex> bucketLock(directory[i]->bucket->mtx); // Lock the bucket
    //             if (i & (1 << localDepth)) {
    //                 directory[i]->bucket = newBucket_o;  
    //             } else {
    //                 directory[i]->bucket = newBucket;  
    //             }
    //             bucketLock.unlock(); //unlock

    //             std::unique_lock<std::shared_mutex> dirLock(directory[i]->directoryMutex); // Lock directory for writing
    //             directory[i]->localDepth = localDepth + 1;
    //             dirLock.unlock(); // Unlock the directory
                
    //         }
    //     }
        
    // }

public:
    ExtendibleHash(int initialDepth, int bucketCap) : globalDepth(initialDepth), bucketCapacity(bucketCap) {
        directory.resize(1 << initialDepth);
        for (int i = 0; i < (1 << initialDepth); ++i) {

            directory[i] = make_shared<Directory>();
            directory[i]->bucket = make_shared<Bucket>(bucketCap,i);
            
            directory[i]->prefix = i;
            directory[i]->localDepth = initialDepth;
        }
    }

    void insert(int key, int value) {
        int hashValue = hashFunction(key);
        {
            if (!directory[hashValue] || !directory[hashValue]->bucket) {
                std::cerr << "Error: Bucket at hashValue " << hashValue << " is null." << std::endl;
                return;
            }
            shared_lock<shared_mutex> readLock(directory[hashValue]->directoryMutex);
            bool ret = directory[hashValue]->bucket->insert(key, value);
            if(ret)
                return;    
        }
        splitBucket(hashValue);
    }

    void printStatus() {
        lock_guard<mutex> lock(globalMutex); 
        cout << "Global Depth: " << globalDepth << endl;
        for (int i = 0; i < directory.size(); ++i) {
            cout << "Bucket " << i << " (Local Depth: " << directory[i]->localDepth << "): ";
            for (auto& kv : directory[i]->bucket->kvStore) {
                cout << "{" << kv.first << ": " << kv.second << "} ";
            }
            cout << endl;
        }
    }
};




