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

TaskHandle_t TaskDccReaderHandle = nullptr;

void vTaskDccReader(void *pvParameters);

QueueHandle_t xSerialCharQueue = nullptr;

void vTaskRxInterruptEmulator(void *pvParameters);
void vTaskDccProcessor(void *pvParameters);

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

  // xSerialCharQueue = xQueueCreate(255, sizeof(char));

  // if (xSerialCharQueue != NULL) {
  //   // Tâche 1: Lecture ultra-rapide (Producteur)
  //   xTaskCreate(vTaskRxInterruptEmulator, "RxRead", 100, NULL, 2, NULL);

  //   // Tâche 2: Traitement et renvoi (Consommateur)
  //   xTaskCreate(vTaskDccProcessor, "DccProc", 200, NULL, 3, NULL);

  //   Serial.println(F("Initialisation FreeRTOS OK !"));
  // } else {
  //   // Ce message t'aurait sauvé la mise !
  //   Serial.println(
  //       F("ERREUR FATALE : Impossible d'allouer la RAM pour la Queue."));
  // }

  // xTaskCreate(
  //     vTaskDccReader, "DccRead",
  //     128, // Petite stack suffisante pour du pur bypass
  //     NULL,
  //     3, // Priorité haute pour ne pas se faire couper par l'IHM ou l'OLED
  //     &TaskDccReaderHandle);
}

void loop() {}

// Tâche de lecture / réécriture directe
void vTaskDccReader(void *pvParameters) {
  (void)pvParameters;

  for (;;) {
    // TANT QU'IL y a des octets dans le buffer matériel du Mega
    while (Serial2.available() > 0) {
      char c = Serial2.read();
      Serial.write(c); // On le recrache immédiatement vers le PC
    }

    // Une fois le buffer matériel vide, on relâche le CPU
    // juste 1 tick (1ms) pour laisser les autres tâches s'exécuter.
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}

// 1. Tâche de capture : Ne fait aucun calcul, elle stocke juste.
void vTaskRxInterruptEmulator(void *pvParameters) {
  (void)pvParameters;

  Serial.println(F("-> Tâche RxRead (Producteur) est EN LIGNE."));

  for (;;) {
    // S'il y a des données qui attendent dans le gros buffer matériel
    if (Serial2.available() > 0) {

      // On vide tout d'un coup dans la file FreeRTOS
      while (Serial2.available() > 0) {
        char c = Serial2.read();
        xQueueSendToBack(xSerialCharQueue, &c, 0);
      }

      // On a fini de vider, on force FreeRTOS à passer à la tâche
      // de traitement (Consommateur) SANS ATTENDRE le prochain tick.
      taskYIELD();

    } else {
      // S'il n'y a rien à lire sur la ligne, on s'endort 1 tick (15ms)
      // pour laisser le CPU aux écrans et aux boutons.
      vTaskDelay(1);
    }
  }
}

// 2. Tâche de traitement : Elle consomme la file et envoie vers Serial
void vTaskDccProcessor(void *pvParameters) {
  (void)pvParameters;
  char receivedChar;

  Serial.println(F("-> Tâche DccProc (Consommateur) est EN LIGNE."));

  // On attend 1 seconde que la centrale DCC soit bien réveillée
  vTaskDelay(pdMS_TO_TICKS(1000));

  Serial.println(F("Envoi de la commande <s> à la centrale..."));
  Serial2.println(F("<s>"));

  for (;;) {
    if (xQueueReceive(xSerialCharQueue, &receivedChar, portMAX_DELAY) ==
        pdPASS) {
      Serial.print(receivedChar);
    }
  }
}