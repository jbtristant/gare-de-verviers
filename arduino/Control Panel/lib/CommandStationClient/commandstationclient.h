#pragma once

#include <Arduino.h>
#include <SPI.h>
#include <Ethernet.h>

enum class OnOff : uint8_t { on = 1, off = 0 };
enum class Track { both, main, prog, join };
enum class TrackMode { prog, main, main_inv, main_a, dc, dcx, none };
enum class Direction : uint8_t { forward = 0, reverse = 1 };
enum class Speedstep : uint8_t { speed_28, speed_128 };
enum class Momentum : uint8_t { linear, power };

// Helper pour convertir l'enum en chaîne de caractères (Strictement compatible C++11)
constexpr const char* trackModeToCString(TrackMode mode) {
    return (mode == TrackMode::prog)     ? "PROG" :
           (mode == TrackMode::main)     ? "MAIN" :
           (mode == TrackMode::main_inv) ? "MAIN_INV" :
           (mode == TrackMode::main_a)   ? "MAIN A" : // Avec l'espace requis par DCC-EX
           (mode == TrackMode::dc)       ? "DC" :
           (mode == TrackMode::dcx)      ? "DCX" : "NONE";
}

constexpr const char* SpeedstepToCString(Speedstep speedstep) {
    return (speedstep == Speedstep::speed_28)     ? "SPEED28" :
           (speedstep == Speedstep::speed_128)    ? "SPEED128" : "SPEED128";
}

class CommandStationClient {
    public:
        explicit CommandStationClient(Stream &stream, Stream &logStream);

        void process();

        // Commandes
        void askStatus();
        void askRosterList();
        void powerTrack(OnOff onOff, Track track);
        void powerTrack(OnOff onOff, char trackLetter);
        void configureTrackManager(char trackLetter, TrackMode mode, uint16_t cabId = 0);
        void configureSpeedsteps(Speedstep speedstep);
        void setMomentum(Momentum momentum);

        
        /// Loco
        void askLocoInfo(uint16_t id);
        void askRosterInfo(const char* name);
        void setLocoSteed(uint16_t id, uint8_t speed, Direction direction);
        void setLocoFunction(uint16_t id, uint8_t function, OnOff onOff);
        void setLocoMomentum(uint8_t id, uint8_t acceleration);
        void setLocoMomentum(uint8_t id, uint8_t acceleration, uint8_t deceleration);
 
        

    private:
        void processMessage();
        void processCmd(char data[], byte size);

        void handleLocoUpdate(char data[], byte size);

        Stream &m_stream;
        Stream &m_logStream;

        char inData[256];
        char inChar = -1;
        byte arraySize = 0;
};