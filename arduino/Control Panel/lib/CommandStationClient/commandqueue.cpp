#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include <queue.h>

#include "commandqueue.h"

CommandQueue::CommandQueue() : m_stream(nullptr), m_xStreamSemaphore(NULL), m_logStream(nullptr), m_xLogStreamSemaphore(NULL)
{
    m_queue = xQueueCreate(MAX_QUEUE_SIZE, MAX_CMD_LEN);
}

void CommandQueue::initialize(Stream *stream, SemaphoreHandle_t xStreamSemaphore, Stream *logStream, SemaphoreHandle_t xLogStreamSemaphore)
{
    m_stream = stream;
    m_xStreamSemaphore = xStreamSemaphore;
    m_logStream = logStream;
    m_xLogStreamSemaphore = xLogStreamSemaphore;
}

bool CommandQueue::push(const char *cmd)
{
    //Serial.println(F("CommandQueue::push(const char *cmd)"));
    if (m_queue == NULL)
        return false;

    // pdPASS signifie que l'insertion a réussi.
    // 0 correspond au "ticks to wait" : si la queue est pleine, on n'attend pas, on échoue.
    return (xQueueSend(m_queue, cmd, 0) == pdPASS);
}

bool CommandQueue::push(const __FlashStringHelper *cmd)
{
    //Serial.println(F("CommandQueue::push(const __FlashStringHelper *cmd)"));
    if (m_queue == NULL)
        return false;

    char buffer[MAX_CMD_LEN];
    strncpy_P(buffer, reinterpret_cast<const char *>(cmd), MAX_CMD_LEN - 1);

    if (m_logStream != nullptr && m_xLogStreamSemaphore != NULL) {
        if (xSemaphoreTake(m_xLogStreamSemaphore, pdMS_TO_TICKS(50)) == pdTRUE) {
            m_logStream->print(F("Push: "));
            m_logStream->println(buffer);
            xSemaphoreGive(m_xLogStreamSemaphore);
        }
    }

    return (xQueueSend(m_queue, buffer, 0) == pdPASS);
}

bool CommandQueue::pop(char *dest, TickType_t waitTicks)
{
    if (m_queue == NULL)
        return false;

    return (xQueueReceive(m_queue, dest, waitTicks) == pdPASS);
}

bool CommandQueue::pop(TickType_t waitTicks)
{
    if (m_queue == 0)
        return false;

    char buffer[MAX_CMD_LEN];

    //Serial.print(F("POP sur Queue @ ")); Serial.println((uint32_t)m_queue);

    // Serial.print(F("Queue size: "));
    // Serial.println((uint16_t)count()); // Affiche l'adresse RAM de la queue

    // if (m_logStream == nullptr) {
    //     Serial.println(F("ERREUR: m_logStream est NULL !"));
    //     return false;
    // }
    // if (m_xLogStreamSemaphore == NULL) {
    //     Serial.println(F("ERREUR: m_xLogStreamSemaphore est NULL !"));
    //     return false;
    // }
    // if (m_stream == nullptr) {
    //     Serial.println(F("ERREUR: m_stream est NULL !"));
    //     return false;
    // }
    // if (m_xStreamSemaphore == NULL) {
    //     Serial.println(F("ERREUR: m_xStreamSemaphore est NULL !"));
    //     return false;
    // }

    if (xQueueReceive(m_queue, buffer, waitTicks) == pdPASS) {
        if (m_logStream != nullptr && m_xLogStreamSemaphore != NULL) {
            if (xSemaphoreTake(m_xLogStreamSemaphore, pdMS_TO_TICKS(50)) == pdTRUE) {
                m_logStream->print(F("Pop: "));
                m_logStream->println(buffer);
                xSemaphoreGive(m_xLogStreamSemaphore);
            }
        }
        if (m_stream != nullptr && m_xStreamSemaphore != NULL) {
            if (xSemaphoreTake(m_xStreamSemaphore, portMAX_DELAY) == pdTRUE) {
                m_stream->print(buffer);
                xSemaphoreGive(m_xStreamSemaphore);
                return true;
            }
        }
    }
    return false;
}

uint8_t CommandQueue::count()
{
    if (m_queue == NULL)
        return 0;
    return uxQueueMessagesWaiting(m_queue);
}

bool CommandQueue::isEmpty() { return count() == 0; }