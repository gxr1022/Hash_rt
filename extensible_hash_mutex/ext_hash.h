#pragma once
#include <iostream>
#include <vector>
#include <unordered_map>
#include <cmath>
#include <thread>
#include <mutex>
#include <memory> 

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
    int localDepth;
    mutex directoryMutex;
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
        
        int localDepth;
        int cap;

        // lock_guard<mutex> globalLock(); //Lock only avoid the same resize operation
        globalMutex.lock(); // lock
        // directory[dir_prefix]->directoryMutex.lock(); // lock

        localDepth = directory[dir_prefix]->localDepth;
        if (localDepth == globalDepth) {
            cap = directory.size();
            directory.resize(cap * 2);
            globalDepth++;
            for (int i = 0; i < cap; ++i) {
                directory[i + cap] = std::make_shared<Directory>();
                directory[i + cap]->directoryMutex.lock(); // lock
                directory[i + cap]->bucket = directory[i]->bucket;
                directory[i + cap]->prefix = i + cap;
                directory[i + cap]->localDepth = localDepth;
                directory[i + cap]->directoryMutex.unlock(); //lock
            }
        }
        // directory[dir_prefix]->directoryMutex.unlock(); // lock
        

        // relocate kv pairs frome old bucket to new bucket.
        shared_ptr<Bucket> oldBucket = directory[dir_prefix]->bucket;
        int cur_bucket_prefix = directory[dir_prefix]->bucket->prefix;
        int new_bucket_prefix = cur_bucket_prefix + (1 << localDepth);
        
        shared_ptr<Bucket> newBucket;
        shared_ptr<Bucket> newBucket_o;


        oldBucket->mtx.lock();
        newBucket = make_shared<Bucket>(bucketCapacity, new_bucket_prefix);
        newBucket_o = make_shared<Bucket>(bucketCapacity, cur_bucket_prefix);

        for (auto& pair : oldBucket->kvStore) {
            int newHash = hashFunction(pair.first);
            if(newHash == oldBucket->prefix)
                newBucket_o->insert(pair.first, pair.second); // insert into old bucket.
            else
                newBucket->insert(pair.first, pair.second);
        }
        oldBucket->kvStore.clear();
        oldBucket->mtx.unlock();


        // update the bucket point of directory
        for (int i = 0; i < directory.size(); ++i) 
        {
            if (directory[i]->bucket == oldBucket) 
            {
                // newBucket->mtx.lock();
                // newBucket_o->mtx.lock();
                directory[i]->directoryMutex.lock(); //lock
                if (i & (1 << localDepth)) {
                    directory[i]->bucket = newBucket_o;  
                } else {
                    directory[i]->bucket = newBucket;  
                }
                directory[i]->localDepth = localDepth + 1;
                directory[i]->directoryMutex.unlock(); //unlock
                
                // newBucket->mtx.unlock();
                // newBucket_o->mtx.unlock();
            }
        }
        globalMutex.unlock(); //unlock
        
    }

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
        while (true) {
            // globalMutex.lock(); 
            int hashValue = hashFunction(key);
            if (!directory[hashValue] || !directory[hashValue]->bucket) {
                std::cerr << "Error: Bucket at hashValue " << hashValue << " is null." << std::endl;
                return;
            }

            // lock_guard<mutex> bucketLock(directory[hashValue]->directoryMutex);
            // directory[hashValue]->directoryMutex.lock();
            bool ret = directory[hashValue]->bucket->insert(key, value);
            // directory[hashValue]->directoryMutex.unlock();
            // globalMutex.unlock(); 
            if (ret) {
                break;
            } else {
                splitBucket(hashValue);
            }
        }
    }

    bool remove(int key) {
        int hashValue = hashFunction(key);
        lock_guard<mutex> bucketLock(directory[hashValue]->directoryMutex);
        return directory[hashValue]->bucket->remove(key);
    }

    int get(int key) {
        int hashValue = hashFunction(key);
        lock_guard<mutex> bucketLock(directory[hashValue]->directoryMutex);
        return directory[hashValue]->bucket->get(key);
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

void threadInsert(ExtendibleHash& hashTable, int start, int end) {
    for (int i = start; i <= end; ++i) {
        hashTable.insert(i, i * 10);
    }
}

void threadGet(ExtendibleHash& hashTable, int start, int end) {
    for (int i = start; i <= end; ++i) {
        cout << "Get key " << i << ": " << hashTable.get(i) << endl;
    }
}


