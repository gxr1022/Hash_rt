#pragma once
#include <iostream>
#include <vector>
#include <unordered_map>
#include <cmath>
#include <thread>
#include <mutex>
#include <memory> 
#include <shared_mutex>
#include <condition_variable>
#include <atomic>

#define HASH_INIT_LOCAL_DEPTH          (10)
#define HASH_INIT_BUCKET_NUM           (1 << HASH_INIT_LOCAL_DEPTH)
#define HASH_ASSOC_NUM                 (8)


using namespace std;

class Bucket {
public:
    unordered_map<string, string> kvStore;
    int capacity;
    int prefix;
    mutex mtx; 

    Bucket(int cap, int pfix) : capacity(cap), prefix(pfix) {}

    bool isFull() {
        // lock_guard<mutex> lock(mtx); 
        return kvStore.size() >= capacity;
    }

    bool insert(string key, string value) {
        // lock_guard<mutex> lock(mtx);  
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

    bool remove(string key) {
        lock_guard<mutex> lock(mtx);
        if (kvStore.find(key) != kvStore.end()) {
            kvStore.erase(key);
            return true;
        }
        return false; 
    }

    string get(string key) {
        lock_guard<mutex> lock(mtx);  
        if (kvStore.find(key) != kvStore.end()) {
            return kvStore[key];
        }
        return "not found"; 
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
    shared_mutex globalMutex;   
    condition_variable resizeCondVar;
    atomic<bool> isResizing;
    
    // thread resizeThread;

    int hashFunction(int key) {
        return key & ((1 << globalDepth) - 1);
    }

    void splitBucket(int dir_prefix) {
        
        if(directory[dir_prefix]->localDepth == globalDepth && !isResizing.load())
        {
            unique_lock<shared_mutex> lock(globalMutex);
            isResizing.store(true);
            int cap = directory.size();
            directory.resize(cap * 2);  
            
            for (int i = 0; i < cap; ++i) {
                
                directory[i + cap] = make_shared<Directory>();
                // unique_lock<shared_mutex> dirLock(directory[i + cap]->directoryMutex);
                directory[i + cap]->bucket = directory[i]->bucket;
                directory[i + cap]->prefix = i + cap;
                directory[i + cap]->localDepth.store(directory[i]->localDepth.load());
            }
            globalDepth++;
        }
        // isResizing.store(false);
        // resizeCondVar.notify_all();
        
        int localDepth;
        shared_ptr<Bucket> oldBucket;
        {
            shared_lock<shared_mutex> readLock(directory[dir_prefix]->directoryMutex);
            localDepth = directory[dir_prefix]->localDepth;
            oldBucket = directory[dir_prefix]->bucket;
        }
        
        if (!oldBucket) {
            std::cerr << "Error: oldBucket is null!" << std::endl;
            return;
        }

        int cur_bucket_prefix = oldBucket->prefix;
        int new_bucket_prefix = cur_bucket_prefix + (1 << localDepth);
        shared_ptr<Bucket> newBucket = make_shared<Bucket>(bucketCapacity, new_bucket_prefix);
        shared_ptr<Bucket> newBucket_o = make_shared<Bucket>(bucketCapacity, cur_bucket_prefix);

        //Re-assign kv pairs into two new buckets 
        {
            std::unique_lock<std::mutex> oldBucketLock(oldBucket->mtx); // exclusive lock
            for (auto& pair : oldBucket->kvStore) {
                int newHash = hashFunction(std::stoi(pair.first));
                if (newHash == oldBucket->prefix) {
                    newBucket_o->insert(pair.first, pair.second);  
                } else {
                    newBucket->insert(pair.first, pair.second);  
                }
            }
            oldBucket->kvStore.clear();
        }

        // modify the bucket pointer of directory entry.
        unique_lock<shared_mutex> lock(globalMutex);
        for (int i = 0; i < directory.size(); ++i) {
            if (!directory[i] || !directory[i]->bucket) {
                std::cerr << "Error: Directory or Bucket is null!" << std::endl;
                continue;
            }
            if (directory[i]->bucket == oldBucket) {
                // std::unique_lock<std::shared_mutex> dirLock(directory[i]->directoryMutex); //exlusive lock
                if (i & (1 << localDepth)) {
                    directory[i]->bucket = newBucket_o;
                } else {
                    directory[i]->bucket = newBucket;
                }
                directory[i]->localDepth++;
            }
            
        }
       
    }

public:
    void insert(string key, string value) {
        int hashValue = hashFunction(std::stoi(key));
        {
            shared_lock<shared_mutex> readlock(globalMutex);
            {
                if (!directory[hashValue] ) {
                    std::cerr << "Error: directory at hashValue " << hashValue << " is null." << std::endl;
                    return;
                }
                // shared_lock<shared_mutex> readLock(directory[hashValue]->directoryMutex);
                if(!directory[hashValue]->bucket){
                    std::cerr << "Error: Bucket at hashValue " << hashValue << " is null." << std::endl;
                    return;
                }
                lock_guard<mutex> dirLock(directory[hashValue]->bucket->mtx);
                bool ret = directory[hashValue]->bucket->insert(key, value);
                if(ret)
                    return;    
            }
        }
        
        splitBucket(hashValue);
    }

    ExtendibleHash(int initialDepth, int bucketCap) : globalDepth(initialDepth), bucketCapacity(bucketCap) {
        directory.resize(1 << initialDepth);
        for (int i = 0; i < (1 << initialDepth); ++i) {

            directory[i] = make_shared<Directory>();
            directory[i]->bucket = make_shared<Bucket>(bucketCap,i);
            
            directory[i]->prefix = i;
            directory[i]->localDepth = initialDepth;
        }
        isResizing.store(false);
        // resizeThread = thread(&ExtendibleHash::resizeDirectory, this);
    }

    //  ~ExtendibleHash() {
    //     if (resizeThread.joinable()) {
    //         resizeThread.join(); 
    //     }
    // }

    void printStatus() {
        // lock_guard<mutex> lock(globalMutex); 
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




