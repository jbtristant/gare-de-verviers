#pragma once

#include <Arduino_FreeRTOS.h>
#include <queue.h>
#include <semphr.h>

#include "types.h"
#include "commandqueue.h"
#include "locomotive.h"
#include "turnout.h"
#include "track.h"

constexpr uint8_t maxLocomotives = 4;
constexpr uint8_t maxTurnouts = 16;
constexpr uint8_t maxTracks = 2;
constexpr unsigned int sendCommandInterval = 40;


class CommandStationClient {
public:
  explicit CommandStationClient(Stream &stream, SemaphoreHandle_t &xStreamSemaphore, 
                                Stream &logStream, SemaphoreHandle_t &xLogStreamSemaphore);

  static void TaskReadStreamStatic(void *pvParameters);
  static void TaskProcessMessageStatic(void *pvParameters);
  static void TaskProcessPendingCommandStatic(void *pvParameters);
  void taskReadStreamMessage();
  void taskProcessMessage();
  void taskProcessPendingCommand();

  // Commandes
  void askStatus();
  void reboot();
  void askCurrentValues();
  void askMaxCurrentValues();
  void powerTrack(OnOff onOff, TrackType track);
  void powerTrack(OnOff onOff, char trackLetter);
  void configureTrackManager(char trackLetter, TrackMode mode,
                             uint16_t cabAddress = 0);
  void configureSpeedsteps(Speedstep speedstep);
  void setMomentum(Momentum momentum);

  /// Loco
  void askLocoInfo(uint16_t address);
  void askRosters();
  void askRostersInfo();
  void askRosterInfo(uint16_t address);
  void setLocoSpeed(uint16_t address, int8_t speed, Direction direction);
  void setLocoFunction(uint16_t address, uint8_t function, OnOff onOff);
  void setLocoMomentum(uint8_t address, uint8_t acceleration);
  void setLocoMomentum(uint8_t address, uint8_t acceleration,
                       uint8_t deceleration);

  void setTrackChangedCallback(void (*trackChanged)());

  void setLocoSpeedChangedCallback(void (*locoSpeedChanged)(
      uint16_t address, int8_t speed, Direction direction));
  
  void setTurnoutStateChangedCallback(void (*turnoutStateChanged)(uint16_t id, TurnoutState state));

  Track *getTrack(uint8_t index);
  uint8_t getTracksCount();

  Locomotive *getLocomotive(uint8_t index);
  uint8_t getLocomotivesCount();

  Turnout *getTurnout(uint8_t index);
  uint8_t getTurnoutsCount();

  // Turnout/Point
  void askTurnouts();
  void askTurnoutsJ();
  void askTurnoutsT();
  void askTurnoutsJT();
  void askTurnoutInfo(uint16_t id);
  void ThrowCloseTurnout(uint16_t id, TurnoutState state);

private:
  struct LocomotiveCommand {
    uint16_t address = 0;
    int8_t speed = 0;
    Direction direction = Direction::Forward;
    bool isPending = false;
  };

  void processCmd(char data[], byte size);

  void handlePowerTrackState(char data[], byte size);
  void handleJValues(char data[], byte size);
  void handleLocoUpdate(char data[], byte size);
  void handleTurnoutState(char data[], byte size);

  void (*_trackChanged)();
  void _setLocoSpeed(uint16_t address, int8_t speed, Direction direction);
  void (*_locoSpeedChanged)(uint16_t address, int8_t speed,
                            Direction direction);
  void (*_turnoutStateChanged)(uint16_t id, TurnoutState state);


  CommandQueue m_queue;

  Stream &m_stream;
  Stream &m_logStream;
  SemaphoreHandle_t &m_xStreamSemaphore;
  SemaphoreHandle_t &m_xLogStreamSemaphore;

  QueueHandle_t m_xSerialCharQueue;

  char inData[256];
  //char inChar = -1;
  byte arraySize = 0;

  LocomotiveCommand m_pendingLocomotives[maxLocomotives];
  Locomotive m_locomotives[maxLocomotives];
  Turnout m_turnouts[maxTurnouts];
  Track m_tracks[maxTracks];

  unsigned long m_lastSendTime = 0;
  uint8_t m_currentLocomotiveIndex = 0;
  uint8_t m_locomotiveCount = 0;
  uint8_t m_turnoutCount = 0;
  uint8_t m_trackCount = 0;

  char m_generalWriteBuffer[32];
};