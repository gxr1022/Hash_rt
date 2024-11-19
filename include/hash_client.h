#pragma once
#include "hash_server.h"
#include <random>
#include <thread>
#include "spsc.h"

class HashTableClient 
{
private:
    SPSCQueue<HashRequest, 1024> request_queue;  // Client owns the queue
    HashTableServer& server;
    size_t client_id;
    std::mt19937 rng;
    
    void generate_random_string(char* buffer, size_t length) 
    {
        static const char charset[] = "abcdefghijklmnopqrstuvwxyz";
        for (size_t i = 0; i < length - 1; ++i) {
            buffer[i] = charset[rng() % (sizeof(charset) - 1)];
        }
        buffer[length - 1] = '\0';
    }

public:
    HashTableClient(size_t id, HashTableServer& srv) 
        : client_id(id)
        , server(srv)
        , rng(std::random_device{}()) 
    {
        // Register client queue with server
        server.register_client_queue(&request_queue, client_id);
    }
    
    ~HashTableClient() {
        // Unregister from server
        server.unregister_client_queue(client_id);
    }
    
    void submit_request(const char* key, const char* value) {
        HashRequest req(key, value, client_id);
        while (!request_queue.try_push(std::move(req))) {
            std::this_thread::yield();
        }
    }
    
    void run() 
    {
        while (server.is_running()) 
        {
            char key[MAX_KEY_LENGTH];
            char value[MAX_VALUE_LENGTH];
            
            generate_random_string(key, MAX_KEY_LENGTH);
            generate_random_string(value, MAX_VALUE_LENGTH);

            submit_request(key, value); 
        }
    }
};