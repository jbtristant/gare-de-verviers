/*
        Created by Jean-Benoit Tristant, May 11, 2026
*/

#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include <queue.h>
#include <semphr.h>

#include "commandstationclient.h"

SemaphoreHandle_t xSerialSemaphore = nullptr;
SemaphoreHandle_t xSerial2Semaphore = nullptr;
CommandStationClient* commandStationClient = nullptr;


// ========================
// Initialisation programme
// ========================
void setup() {
  Serial.begin(115200);
  Serial2.begin(115200);

  xSerialSemaphore = xSemaphoreCreateMutex();
  xSerial2Semaphore = xSemaphoreCreateMutex();

  if (xSerialSemaphore == nullptr || xSerial2Semaphore == nullptr) {
    Serial.println(F("ERREUR FATALE : Impossible d'allouer les sémaphore."));
    return;
  }

  commandStationClient = new CommandStationClient(Serial2, xSerial2Semaphore, Serial, xSerialSemaphore);
}

void loop() {}