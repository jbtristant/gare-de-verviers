#include "commandstationclient.h"

CommandStationClient::CommandStationClient(Stream &stream, Stream &logStream) :
m_stream(stream), m_logStream(logStream)
{}

void CommandStationClient::askStatus()
{
    m_stream.print(F("<s>"));
}

void CommandStationClient::askRosterList()
{
    m_stream.print(F("<JR>"));
}

void CommandStationClient::powerTrack(OnOff onOff, Track track)
{
    if (track == Track::both) {
        (onOff == OnOff::on) ? m_stream.print(F("<1>")) : m_stream.print(F("<0>"));
    } else if ( track == Track::join) {
        (onOff == OnOff::on) ? m_stream.print(F("<1 JOIN>")) : m_stream.print(F("<0>"));
    } else if ( track == Track::main) {
        (onOff == OnOff::on) ? m_stream.print(F("<1 MAIN>")) : m_stream.print(F("<0 MAIN>"));
    } else if ( track == Track::prog) {
        (onOff == OnOff::on) ? m_stream.print(F("<1 PROG>")) : m_stream.print(F("<0 PROG>"));
    }
}

void CommandStationClient::powerTrack(OnOff onOff, char trackLetter)
{
    ///                           "<1 A>\0"
    constexpr size_t bufferSize = 2 + 1 + 1 + 2;
    char buffer[bufferSize];
    snprintf(buffer, sizeof(buffer), "<%u %c>", static_cast<uint8_t>(onOff), trackLetter);
    m_stream.print(buffer);
}

void CommandStationClient::configureTrackManager(char trackLetter, TrackMode mode, uint16_t cabId)
{
    char buffer[32];
    if (cabId == 0) {
        snprintf(buffer, sizeof(buffer), "<= %c %s>", trackLetter, trackModeToCString(mode));
    } else {
        snprintf(buffer, sizeof(buffer), "<= %c %s %u>", trackLetter, trackModeToCString(mode), cabId);
    }
    m_stream.print(buffer);
}

void CommandStationClient::configureSpeedsteps(Speedstep speedstep)
{
    ///                           "<D SPEED128>\0"
    constexpr size_t bufferSize = 2 + 1 + 8 + 2;
    char buffer[bufferSize];
    snprintf(buffer, sizeof(buffer), "<D %s>", SpeedstepToCString(speedstep));
    m_stream.print(buffer);
}

void CommandStationClient::setMomentum(Momentum momentum)
{
    if (momentum == Momentum::linear) m_stream.print(F("<m LINEAR>"));
    if (momentum == Momentum::power) m_stream.print(F("<m POWER>"));
}

void CommandStationClient::askLocoInfo(uint16_t id)
{
    ///                          "<t 0-10293>\0"
    constexpr size_t bufferSize = 2 + 1 + 5 + 2;
    char buffer[bufferSize];
    snprintf(buffer, sizeof(buffer), "<t %u>", id);
    m_stream.print(buffer);
}

void CommandStationClient::askRosterInfo(const char* name) {
    // "<J R " + le nom + ">\0"
    char buffer[128]; 
    snprintf(buffer, sizeof(buffer), "<J R %s>", name);   
    m_stream.print(buffer); 
}

void CommandStationClient::setLocoSteed(uint16_t id, uint8_t speed, Direction direction)
{
    ///                          "<t 0-10293 0-127 0-1>\0"
    constexpr size_t bufferSize = 2 + 1 + 5 + 1 + 3 + 1 + 1 + 2;
    char buffer[bufferSize];
    snprintf(buffer, sizeof(buffer), "<t %u %u %u>", id, speed, static_cast<uint8_t>(direction));
    m_stream.print(buffer);
}

void CommandStationClient::setLocoFunction(uint16_t id, uint8_t function, OnOff onOff)
{
    ///                          "<t 0-10293 0-68 0-1>\0"
    constexpr size_t bufferSize = 2 + 1 + 5 + 1 + 2 + 1 + 1 + 2;
    char buffer[bufferSize];
    snprintf(buffer, sizeof(buffer), "<F %u %u %u>", id, function, static_cast<uint8_t>(onOff));
    m_stream.print(buffer);
}

void CommandStationClient::setLocoMomentum(uint8_t id, uint8_t acceleration)
{
    ///                          "<m 0-10293 7-21>\0"
    constexpr size_t bufferSize = 2 + 1 + 5 + 1 + 3 + 2;
    char buffer[bufferSize];
    snprintf(buffer, sizeof(buffer), "<m %u %u>", id, acceleration);
    m_stream.print(buffer);
}

void CommandStationClient::setLocoMomentum(uint8_t id, uint8_t acceleration, uint8_t deceleration)
{
    ///                          "<m 0-10293 7-21 7-21>\0"
    constexpr size_t bufferSize = 2 + 1 + 5 + 1 + 3 + 1 + 3 + 2;
    char buffer[bufferSize];
    snprintf(buffer, sizeof(buffer), "<m %u %u %u>", id, acceleration, deceleration);
    m_stream.print(buffer);
}


void CommandStationClient::process()
{
    processMessage();
}

void CommandStationClient::processMessage()
{
    while (m_stream.available() > 0) {
        if (arraySize < 255) { // One less than the size of the array
            inChar = m_stream.read(); // Read a character
            if (inChar == '<') {
                arraySize = 0;
                inData[0] = '\0';
                return;
            }else if (inChar == '>') {
                inData[arraySize] = '\0';
                if (arraySize > 0) {
                    processCmd(inData, arraySize);
                }
                arraySize = 0;
                return;
            } else {
                inData[arraySize] = inChar; // Store it
                arraySize++; // Increment where to write next
                inData[arraySize] = '\0'; // Null terminate the string
            }
        } else {
            arraySize = 0;
        }
    }
}

void CommandStationClient::processCmd(char data[], byte size)
{
    Serial.println(inData);

    switch(data[0]) {
        case 'l':
            handleLocoUpdate(data, size);
            break;

        case 'X':
            Serial.println(F("Commande refusée par la centrale."));
            break;

        default:
            // Affiche les autres messages (ID, Power status, etc.)
            Serial.print(F("Autre message : "));
            Serial.println(data);
            break;
    }
}

void CommandStationClient::handleLocoUpdate(char data[], byte size)
{
    int cabid, slot, speedbyte;
    long functionMap;

    // sscanf cherche le format : l [espace] entier [espace] entier...
    // On commence à data+1 pour ignorer le 'l'
    int matched = sscanf(data + 1, "%d %d %d %ld", &cabid, &slot, &speedbyte, &functionMap);

    if (matched >= 3) {
        int realThrottle = 0;

        // Conversion du SpeedByte en ton système (-127 à 127)
        if (speedbyte >= 130) {
            realThrottle = speedbyte - 129; // Marche avant
        } else if (speedbyte >= 2 && speedbyte <= 127) {
            realThrottle = -(speedbyte - 1); // Marche arrière
        } else {
            realThrottle = 0; // Arrêt (0 ou 128)
        }

        // Filtrage : On ne met à jour que si c'est notre loco (64)
        if (cabid == 64) {
            //throttle = realThrottle;
            //updateThrottleDisplay(throttle);
            
            Serial.print(F("Update Reçu - Loco 64 | Vitesse: "));
            //Serial.println(throttle);
        }
    }
}