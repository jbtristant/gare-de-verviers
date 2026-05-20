#pragma once

#include <Arduino.h>

#define MAX_QUEUE_SIZE 32
#define MAX_CMD_LEN 32

class CommandQueue {
private:
    char queue[MAX_QUEUE_SIZE][MAX_CMD_LEN];
    uint8_t head = 0; // Index pour lire
    uint8_t tail = 0; // Index pour écrire
    uint8_t count = 0; // Nombre d'éléments dans la file

public:
    bool push(const char* cmd) {
        if (count >= MAX_QUEUE_SIZE) return false;
        
        strncpy(queue[tail], cmd, MAX_CMD_LEN - 1);
        queue[tail][MAX_CMD_LEN - 1] = '\0'; // Sécurité
        
        tail = (tail + 1) % MAX_QUEUE_SIZE;
        count++;
        return true;
    }
        
    bool push(const __FlashStringHelper* cmd) {
        if (count >= MAX_QUEUE_SIZE) return false; 

        strncpy_P(queue[tail], reinterpret_cast<const char*>(cmd), MAX_CMD_LEN - 1);
        queue[tail][MAX_CMD_LEN - 1] = '\0'; // Sécurité
        
        tail = (tail + 1) % MAX_QUEUE_SIZE;
        count++;
        return true;
    }

    template<typename... Args>
    bool push(const char* fmt, Args... args)
    {
        if (count >= MAX_QUEUE_SIZE) return false;

        snprintf(queue[tail],
                MAX_CMD_LEN,
                fmt,
                args...);

        tail = (tail + 1) % MAX_QUEUE_SIZE;
        count++;

        return true;
    }

    bool pop(char* dest) {
        if (count == 0) return false;
        
        strncpy(dest, queue[head], MAX_CMD_LEN);
        dest[MAX_CMD_LEN - 1] = '\0';

        head = (head + 1) % MAX_QUEUE_SIZE;
        count--;
        return true;
    }

    bool pop(Stream& stream)
    {
        if (count == 0) return false;

        //Serial.print(F("Pop: "));
        //Serial.println(queue[head]);
        stream.print(queue[head]);

        head = (head + 1) % MAX_QUEUE_SIZE;
        count--;

        return true;
    }

    bool isEmpty() { return count == 0; }
};