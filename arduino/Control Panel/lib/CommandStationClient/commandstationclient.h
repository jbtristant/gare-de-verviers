#pragma once

#include <Arduino.h>
//#include <SPI.h>
#include <Ethernet.h>

#include "commandqueue.h"

enum class OnOff : uint8_t {  Off = 0, On = 1 };
enum class Track { Both, Main, Prog, Join };
enum class TrackMode { Prog, Main, Main_inv, Main_a, Dc, Dcx, None };
enum class Direction : uint8_t { Forward = 0, Reverse = 1 };
enum class Speedstep : uint8_t { Speed_28, Speed_128 };
enum class Momentum : uint8_t { Linear, Power };
enum class TurnoutState : uint8_t { Close = 0 , Throw = 1, eXamine = 2 };

// Helper pour convertir l'enum en chaîne de caractères (Strictement compatible C++11)
constexpr const char* trackModeToCString(TrackMode mode) {
    return (mode == TrackMode::Prog)     ? "PROG" :
           (mode == TrackMode::Main)     ? "MAIN" :
           (mode == TrackMode::Main_inv) ? "MAIN_INV" :
           (mode == TrackMode::Main_a)   ? "MAIN A" : // Avec l'espace requis par DCC-EX
           (mode == TrackMode::Dc)       ? "DC" :
           (mode == TrackMode::Dcx)      ? "DCX" : "NONE";
}

constexpr const char* SpeedstepToCString(Speedstep speedstep) {
    return (speedstep == Speedstep::Speed_28)     ? "SPEED28" :
           (speedstep == Speedstep::Speed_128)    ? "SPEED128" : "SPEED128";
}

class CommandStationClient {
    public:
        explicit CommandStationClient(Stream &stream, Stream &logStream);

        void process();

        // Commandes
        void askStatus();
        void reboot();
        void askCurrentValues();
        void askMaxCurrentValues();
        void powerTrack(OnOff onOff, Track track);
        void powerTrack(OnOff onOff, char trackLetter);
        void configureTrackManager(char trackLetter, TrackMode mode, uint16_t cabAddress = 0);
        void configureSpeedsteps(Speedstep speedstep);
        void setMomentum(Momentum momentum);

        
        /// Loco
        void askLocoInfo(uint16_t address);
        void askRosters();
        void askRosterInfo(const char* name);
        void setLocoSpeed(uint16_t address, uint8_t speed, Direction direction);
        void setLocoFunction(uint16_t address, uint8_t function, OnOff onOff);
        void setLocoMomentum(uint8_t address, uint8_t acceleration);
        void setLocoMomentum(uint8_t address, uint8_t acceleration, uint8_t deceleration);

        void setLocoSpeedChangedCallback(void (*locoSpeedChanged)(uint16_t address, int8_t speed, Direction direction));
 
        // Turnout/Point
        void askTurnouts();
        void ThrowCloseTurnout(uint16_t id, TurnoutState state);
        

    private:
        void processMessage();
        void processCmd(char data[], byte size);

        void handlePowerTrackState(char data[], byte size);
        void handleCurrentValues(char data[], byte size);
        void handleLocoUpdate(char data[], byte size);
        void handleTurnoutState(char data[], byte size);

        void _setLocoSpeed(uint16_t address, uint8_t speed, Direction direction);
        void (*_locoSpeedChanged)(uint16_t address, int8_t speed, Direction direction);
        
        void processPendingCommand();

        CommandQueue m_queue;

        Stream &m_stream;
        Stream &m_logStream;

        char inData[256];
        char inChar = -1;
        byte arraySize = 0;
};