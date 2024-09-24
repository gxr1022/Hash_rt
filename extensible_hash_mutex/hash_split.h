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
#include <queue>
#include <atomic>

using namespace std;

class Bucket {
public:
    unordered_map<int, int> kvStore;
    int capacity;
    int prefix;
    mutex mtx;

    Bucket(int cap, int pfix) : capacity(cap), prefix(pfix) {}

    bool isFull() {
        return kvStore.size() >= capacity;
    }

    bool insert(int key, int value) {
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

struct Directory {
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
    mutex taskQueueMutex;
    condition_variable taskCondVar;
    atomic<bool> isResizing;
    thread splitWorker;
    queue<int> taskQueue; // Task queue for bucket splits
    bool stopWorker;

    int hashFunction(int key) {
        return key & ((1 << globalDepth) - 1);
    }

    void workerThread() {
        while (!stopWorker) {
            int dir_prefix;
            {
                unique_lock<mutex> lock(taskQueueMutex);
                taskCondVar.wait(lock, [this]() { return !taskQueue.empty() || stopWorker; });

                if (stopWorker) break;

                dir_prefix = taskQueue.front();
                taskQueue.pop();
            }
            splitBucket(dir_prefix); // Process split task
        }
    }

    void splitBucket(int dir_prefix) {
        if (directory[dir_prefix]->localDepth == globalDepth && !isResizing.load()) {
            unique_lock<mutex> lock(globalMutex);
            isResizing.store(true);
            int cap = directory.size();
            directory.resize(cap * 2);

            for (int i = 0; i < cap; ++i) {
                directory[i + cap] = make_shared<Directory>();
                directory[i + cap]->bucket = directory[i]->bucket;
                directory[i + cap]->prefix = i + cap;
                directory[i + cap]->localDepth.store(directory[i]->localDepth.load());
            }
            globalDepth++;
            isResizing.store(false);
        }

        int localDepth;
        shared_ptr<Bucket> oldBucket;
        {
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

        // Re-assign kv pairs into two new buckets
        {
            for (auto& pair : oldBucket->kvStore) {
                int newHash = hashFunction(pair.first);
                if (newHash == oldBucket->prefix) {
                    newBucket_o->insert(pair.first, pair.second);
                } else {
                    newBucket->insert(pair.first, pair.second);
                }
            }
            oldBucket->kvStore.clear();
        }

        // modify the bucket pointer of directory entry
        for (int i = 0; i < directory.size(); ++i) {
            if (!directory[i] || !directory[i]->bucket) {
                std::cerr << "Error: Directory or Bucket is null!" << std::endl;
                continue;
            }
            if (directory[i]->bucket == oldBucket) {
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
    ExtendibleHash(int initialDepth, int bucketCap) : globalDepth(initialDepth), bucketCapacity(bucketCap), stopWorker(false) {
        directory.resize(1 << initialDepth);
        for (int i = 0; i < (1 << initialDepth); ++i) {
            directory[i] = make_shared<Directory>();
            directory[i]->bucket = make_shared<Bucket>(bucketCap, i);
            directory[i]->prefix = i;
            directory[i]->localDepth = initialDepth;
        }
        isResizing.store(false);
        splitWorker = thread(&ExtendibleHash::workerThread, this); // Start background worker thread
    }

    ~ExtendibleHash() {
        {
            unique_lock<mutex> lock(taskQueueMutex);
            stopWorker = true;
        }
        taskCondVar.notify_all();
        if (splitWorker.joinable()) {
            splitWorker.join();
        }
    }

    void insert(int key, int value) {
        int hashValue = hashFunction(key);
        {
            // if (!directory[hashValue]) {
            //     std::cerr << "Error: directory at hashValue " << hashValue << " is null." << std::endl;
            //     return;
            // }
            if(directory[hashValue])
            {
                shared_lock<shared_mutex> readLock(directory[hashValue]->directoryMutex);
                if(directory[hashValue]->bucket)
                {
                    lock_guard<mutex> dirLock(directory[hashValue]->bucket->mtx);
                    bool ret = directory[hashValue]->bucket->insert(key, value);
                    if (ret) return;
                }
            }
            
            // if (!directory[hashValue]->bucket) {
            //     std::cerr << "Error: Bucket at hashValue " << hashValue << " is null." << std::endl;
            //     return;
            // }
            
            
        }

        // If insert failed, enqueue a bucket split task
        {
            lock_guard<mutex> lock(taskQueueMutex);
            taskQueue.push(hashValue);
        }
        taskCondVar.notify_one(); // Notify worker thread
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
