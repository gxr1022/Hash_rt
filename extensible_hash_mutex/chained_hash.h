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

#define HASH_INIT_BUCKET_NUM           (1000000)
#define HASH_ASSOC_NUM                 (4)


using namespace std;

class Bucket {
public:
    unordered_map<string, string> kvStore;
    int capacity;
    mutex mtx; 

    Bucket(int cap) : capacity(cap) {}

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


class ExtendibleHash {
private:
    std::atomic<int> dirCapacity;
    int bucketCapacity;
    vector<shared_ptr<Bucket>> directory;

    int hashFunction(int key) {
        return key & dirCapacity;
    }

public:
    void insert(string key, string value) {
        int hashValue = hashFunction(std::stoi(key));
        {
            {
                if (!directory[hashValue] ) {
                    std::cerr << "Error: directory at hashValue " << hashValue << " is null." << std::endl;
                    return;
                }
                // shared_lock<shared_mutex> readLock(directory[hashValue]->directoryMutex);
                if(!directory[hashValue]){
                    std::cerr << "Error: Bucket at hashValue " << hashValue << " is null." << std::endl;
                    return;
                }
                lock_guard<mutex> dirLock(directory[hashValue]->mtx);
                bool ret = directory[hashValue]->insert(key, value);
                if(ret)
                    return;    
            }
        }
        
        return;
    }

    ExtendibleHash(int dirCap, int bucketCap) : dirCapacity(dirCap), bucketCapacity(bucketCap) {
        directory.resize(dirCapacity);
        for (int i = 0; i < dirCapacity; ++i) {
            directory[i] = make_shared<Bucket>(bucketCap);
        }
    }

    // void printStatus() {
    //     // lock_guard<mutex> lock(globalMutex); 
    //     cout << "Global Depth: " << globalDepth << endl;
    //     for (int i = 0; i < directory.size(); ++i) {
    //         cout << "Bucket " << i << " (Local Depth: " << directory[i]->localDepth << "): ";
    //         for (auto& kv : directory[i]->bucket->kvStore) {
    //             cout << "{" << kv.first << ": " << kv.second << "} ";
    //         }
    //         cout << endl;
    //     }
    // }
    
};




