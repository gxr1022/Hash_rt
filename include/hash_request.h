#pragma once
#include <cstring>
#include <string>
#include <utility>

class HashRequest 
{
public:
    char key[MAX_KEY_LENGTH];
    char value[MAX_VALUE_LENGTH];
    size_t client_id;

    HashRequest() = default;

    HashRequest(const char* k, const char* v, size_t cid)
    {
        strncpy(key, k, MAX_KEY_LENGTH - 1);
        strncpy(value, v, MAX_VALUE_LENGTH - 1);
        key[MAX_KEY_LENGTH - 1] = '\0';  
        value[MAX_VALUE_LENGTH - 1] = '\0';
        client_id = cid;
    }

    // Move constructor
    HashRequest(HashRequest&& other) noexcept
    {
        memcpy(key, other.key, MAX_KEY_LENGTH);
        memcpy(value, other.value, MAX_VALUE_LENGTH);
        client_id = other.client_id;
    }

    // Move assignment
    HashRequest& operator=(HashRequest&& other) noexcept
    {
        if (this != &other) {
            memcpy(key, other.key, MAX_KEY_LENGTH);
            memcpy(value, other.value, MAX_VALUE_LENGTH);
            client_id = other.client_id;
        }
        return *this;
    }

    // Delete copy operations
    HashRequest(const HashRequest&) = delete;
    HashRequest& operator=(const HashRequest&) = delete;
};

