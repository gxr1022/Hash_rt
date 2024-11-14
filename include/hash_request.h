#pragma once
#include <cstring>

struct HashRequest {
    char key[MAX_KEY_LENGTH];
    char value[MAX_VALUE_LENGTH];
    size_t client_id;
    uint64_t work_usec;

    HashRequest(const char* k, const char* v, size_t id, uint64_t wu) 
        : client_id(id), work_usec(wu) {
        strncpy(key, k, MAX_KEY_LENGTH - 1);
        strncpy(value, v, MAX_VALUE_LENGTH - 1);
        key[MAX_KEY_LENGTH - 1] = '\0';
        value[MAX_VALUE_LENGTH - 1] = '\0';
    }
};

