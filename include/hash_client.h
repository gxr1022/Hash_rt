#pragma once
#include "hash_server.h"
#include <random>
#include <thread>

class HashTableClient
{
private:
    size_t client_id;
    HashTableServer &server;
    std::mt19937 rng;

    void generate_random_string(char *buffer, size_t length)
    {
        static const char charset[] = "abcdefghijklmnopqrstuvwxyz";
        for (size_t i = 0; i < length - 1; ++i)
        {
            buffer[i] = charset[rng() % (sizeof(charset) - 1)];
        }
        buffer[length - 1] = '\0';
    }

public:
    HashTableClient(size_t id, HashTableServer &srv)
        : client_id(id), server(srv), rng(std::random_device{}()) {}

    void run()
    {
        char key[MAX_KEY_LENGTH];
        char value[MAX_VALUE_LENGTH];
        while (server.is_running())
        {
            generate_random_string(key, MAX_KEY_LENGTH);
            generate_random_string(value, MAX_VALUE_LENGTH);

            uint64_t work_usec = 0;
            server.handle_insert(key, value, client_id, work_usec);

            // std::this_thread::sleep_for(std::chrono::microseconds(FLAGS_time_interval));
        }
    }
};