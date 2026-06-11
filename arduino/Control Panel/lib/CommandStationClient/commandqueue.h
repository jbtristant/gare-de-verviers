#pragma once

#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include <queue.h>
#include <semphr.h>

#define MAX_QUEUE_SIZE 32
#define MAX_CMD_LEN 32
class CommandQueue
{
  public:
    CommandQueue();

    void initialize(Stream *stream, SemaphoreHandle_t xStreamSemaphore, Stream *logStream, SemaphoreHandle_t xLogStreamSemaphore);

    bool push(const char *cmd);
    bool push(const __FlashStringHelper *cmd);

    template <typename... Args> bool push(const char *fmt, Args... args)
    {
        //Serial.println(F("CommandQueue::push(const char *fmt, Args... args)"));
        //Serial.print(F("PUSH sur Queue @ ")); Serial.println((uint32_t)m_queue);

        if (m_queue == NULL)
            return false;

        char buffer[MAX_CMD_LEN];
        snprintf(buffer, MAX_CMD_LEN, fmt, args...);

        if (m_logStream != nullptr && m_xLogStreamSemaphore != NULL) {
            if (xSemaphoreTake(m_xLogStreamSemaphore, pdMS_TO_TICKS(50)) == pdTRUE) {
                m_logStream->print(F("Push: "));
                m_logStream->println(buffer);
                xSemaphoreGive(m_xLogStreamSemaphore);
            }
        }

        return (xQueueSend(m_queue, buffer, 0) == pdPASS);
    }

    template <typename... Args> bool push(const __FlashStringHelper *fmt, Args... args)
    {
        //Serial.println(F("CommandQueue::push(const __FlashStringHelper *fmt, Args... args)"));
        //Serial.print("PUSH sur Queue @ "); Serial.println((uint32_t)m_queue);
        if (m_queue == NULL)
            return false;

        // snprintf_P lit le format en mémoire Flash.
        // Le cast est nécessaire pour lui passer le pointeur.
        char buffer[MAX_CMD_LEN];
        snprintf_P(buffer, MAX_CMD_LEN, reinterpret_cast<const char *>(fmt), args...);

        return (xQueueSend(m_queue, buffer, 0) == pdPASS);
    }

    bool pop(char *dest, TickType_t waitTicks = portMAX_DELAY);
    bool pop(TickType_t waitTicks = portMAX_DELAY);

    uint8_t count();
    bool isEmpty();

  private:
    QueueHandle_t m_queue;
    Stream *m_stream;
    SemaphoreHandle_t m_xStreamSemaphore;
    Stream *m_logStream;
    SemaphoreHandle_t m_xLogStreamSemaphore;
};