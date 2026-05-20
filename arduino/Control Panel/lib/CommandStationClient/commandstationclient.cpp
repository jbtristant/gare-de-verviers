#include "commandstationclient.h"

#define MAX_LOCOMOTIVES 4

/// Save a loco state command
struct LocomotiveCommand {
    uint16_t address = 0;
    uint8_t speed = 0;
    Direction direction = Direction::Forward;
    bool isPending = false;
};

LocomotiveCommand pendingLocomotives[MAX_LOCOMOTIVES];

unsigned long lastSendTime = 0;
const unsigned int COMMAND_COOLDOWN = 40;
uint8_t currentLocomotiveIndex = 0;

char generalWriteBuffer[32];

///////////////////////////////////////////////////////////////////////////////
/// CommandStation                                                          ///
///////////////////////////////////////////////////////////////////////////////

CommandStationClient::CommandStationClient(Stream &stream, Stream &logStream) :
 _locoSpeedChanged(nullptr), m_stream(stream),m_logStream(logStream)
{}

void CommandStationClient::askStatus()
{
    m_queue.push(F("<s>"));
}

void CommandStationClient::reboot()
{
    m_queue.push(F("<D RESET>"));
}

void CommandStationClient::askCurrentValues()
{
    m_queue.push(F("<JI>"));
}

void CommandStationClient::askMaxCurrentValues()
{
    m_queue.push(F("<JG>"));
}


///////////////////////////////////////////////////////////////////////////////
/// Tracks                                                                  ///
///////////////////////////////////////////////////////////////////////////////

void CommandStationClient::powerTrack(OnOff onOff, Track track)
{
    if (track == Track::Both) {
        (onOff == OnOff::On) ? m_queue.push(F("<1>")) : m_queue.push(F("<0>"));
    } else if ( track == Track::Join) {
        (onOff == OnOff::On) ? m_queue.push(F("<1 JOIN>")) : m_queue.push(F("<0>"));
    } else if ( track == Track::Main) {
        (onOff == OnOff::On) ? m_queue.push(F("<1 MAIN>")) : m_queue.push(F("<0 MAIN>"));
    } else if ( track == Track::Prog) {
        (onOff == OnOff::On) ? m_queue.push(F("<1 PROG>")) : m_queue.push(F("<0 PROG>"));
    }
}

void CommandStationClient::powerTrack(OnOff onOff, char trackLetter)
{
    m_queue.push("<%u %c>", static_cast<uint8_t>(onOff), trackLetter);
}

void CommandStationClient::configureTrackManager(char trackLetter, TrackMode mode, uint16_t cabId)
{
    if (cabId == 0) {
        m_queue.push("<= %c %s>", trackLetter, trackModeToCString(mode));
    } else {
        m_queue.push("<= %c %s %u>", trackLetter, trackModeToCString(mode), cabId);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// Cabs, Locomotive, Roster                                                ///
///////////////////////////////////////////////////////////////////////////////

void CommandStationClient::askRosters()
{
    m_queue.push(F("<JR>"));
}

void CommandStationClient::configureSpeedsteps(Speedstep speedstep)
{
    m_queue.push("<D %s>", SpeedstepToCString(speedstep));
}

void CommandStationClient::setMomentum(Momentum momentum)
{
    if (momentum == Momentum::Linear) m_queue.push(F("<m LINEAR>"));
    if (momentum == Momentum::Power) m_queue.push(F("<m POWER>"));
}

void CommandStationClient::askLocoInfo(uint16_t id)
{
    m_queue.push("<t %u>", id);
}

void CommandStationClient::askRosterInfo(const char* name) 
{
    m_queue.push("<J R %s>", name);
}

void CommandStationClient::setLocoSpeed(uint16_t address, uint8_t speed, Direction direction)
{
    for (uint8_t i; i < MAX_LOCOMOTIVES; ++i) {
        if (pendingLocomotives[i].address == address) {
            pendingLocomotives[i].speed = speed;
            pendingLocomotives[i].direction = direction;
            pendingLocomotives[i].isPending = true;
            return;
        }
    }

    for (uint8_t i; i < MAX_LOCOMOTIVES; ++i) {
        if (pendingLocomotives[i].address == 0) {
            pendingLocomotives[i].address = address;
            pendingLocomotives[i].speed = speed;
            pendingLocomotives[i].direction = direction;
            pendingLocomotives[i].isPending = true;
            return;
        }
    };
}

void CommandStationClient::_setLocoSpeed(uint16_t address, uint8_t speed, Direction direction)
{
    snprintf(generalWriteBuffer, sizeof(generalWriteBuffer), "<t %u %u %u>", address, speed, static_cast<uint8_t>(direction));
    m_stream.print(generalWriteBuffer);
}

void CommandStationClient::setLocoFunction(uint16_t id, uint8_t function, OnOff onOff)
{
    m_queue.push("<F %u %u %u>", id, function, static_cast<uint8_t>(onOff));
}

void CommandStationClient::setLocoMomentum(uint8_t id, uint8_t acceleration)
{
    m_queue.push("<m %u %u>", id, acceleration);
}

void CommandStationClient::setLocoMomentum(uint8_t id, uint8_t acceleration, uint8_t deceleration)
{
    m_queue.push("<m %u %u %u>", id, acceleration, deceleration);
}

void CommandStationClient::setLocoSpeedChangedCallback(void (*locoSpeedChanged)(uint16_t , int8_t , Direction ))
{
    _locoSpeedChanged = locoSpeedChanged;
}


///////////////////////////////////////////////////////////////////////////////
/// Turnouts/Points                                                         ///
///////////////////////////////////////////////////////////////////////////////

void CommandStationClient::askTurnouts()
{
    m_queue.push(F("<T>"));
}

void CommandStationClient::ThrowCloseTurnout(uint16_t id, TurnoutState state)
{
    switch (state) {
        case TurnoutState::Close:
            m_queue.push("<T %u C>", id);
            break;
        case TurnoutState::Throw:
            m_queue.push("<T %u T>", id);
            break;
        case TurnoutState::eXamine:
            m_queue.push("<T %u X>", id);
            break;
    }
}


void CommandStationClient::process()
{
    processMessage();
    processPendingCommand();
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
    m_logStream.println(inData);

    switch(data[0]) {
        case 'p':
            handlePowerTrackState(data, size);
            break;

        case 'j':
            handleCurrentValues(data, size);
            break;

        case 'l':
            handleLocoUpdate(data, size);
            break;

        case 'H':
            handleTurnoutState(data, size);
            break;

        case 'X':
            Serial.println(F("Commande refusée par la centrale."));
            break;

        default:
            // Print not yet implemented messages (ID, Power status, etc.)
            Serial.print(F("Other message : "));
            Serial.println(data);
            break;
    }
    lastSendTime = millis() - COMMAND_COOLDOWN;
}

void CommandStationClient::handlePowerTrackState(char data[], byte size)
{
    uint16_t state;
    char track;

    int matched = sscanf(data + 1, "%u %c", &state, &track);

    if (matched == 1) {
        state == 0 ? m_logStream.println(F("Track is on")) : m_logStream.println(F("Track is off"));
    }    
    
    if (matched == 2) {
        m_logStream.print(F("Track "));
        m_logStream.print(track);
        state == 0 ? m_logStream.println(F(" is on")) : m_logStream.println(F(" is off"));
    }
}
        
void CommandStationClient::handleCurrentValues(char data[], byte size)
{
    // 1. Déterminer le type de commande
    bool isMaxCurrent = (data[1] == 'G'); // 'I' pour courant actuel, 'G' pour max

    // 2. Trouver le début des données utiles (après le crochet ouvrant '[')
    char* startPtr = strchr(data, '[');
    if (startPtr == nullptr) {
        // Format invalide, pas de crochet ouvrant
        return; 
    }
    startPtr++; // On se place juste après le '['

    // 3. Nettoyer la fin de la chaîne (remplacer ']' ou '>' par des fins de chaîne '\0')
    char* endPtr = strchr(startPtr, ']');
    if (endPtr != nullptr) {
        *endPtr = '\0'; // On coupe la chaîne proprement au crochet fermant
    }

    // 4. Découpage et lecture des valeurs (séparées par des espaces)
    byte trackIndex = 0;
    // strtok remplace le premier espace trouvé par un '\0' et retourne le pointeur vers le jeton
    char* token = strtok(startPtr, " "); 

    while (token != nullptr) {
        // Conversion du texte en entier (milliampères)
        int milliAmps = atoi(token);

        // 5. Traitement de la valeur
        if (isMaxCurrent) {
            // C'est une valeur maximale (jG)
            //this->saveMaxCurrentForTrack(trackIndex, milliAmps);
            m_logStream.print(F("Max current from track: "));
            m_logStream.print(trackIndex);
            m_logStream.print(F(" is: "));
            m_logStream.println(milliAmps);
        } else {
            // C'est une valeur actuelle (jI)
            //this->saveCurrentForTrack(trackIndex, milliAmps);
            m_logStream.print(F("Current value from track: "));
            m_logStream.print(trackIndex);
            m_logStream.print(F(" is: "));
            m_logStream.println(milliAmps);
        }

        trackIndex++;
        // On demande le jeton suivant (en passant nullptr, strtok continue sur la même chaîne)
        token = strtok(nullptr, " ");
    }
}

void CommandStationClient::handleLocoUpdate(char data[], byte size)
{
    int cabid, reg, speedbyte;
    Direction direction;
    long functionMap;

    // sscanf cherche le format : l [espace] entier [espace] entier...
    // On commence à data+1 pour ignorer le 'l'
    int matched = sscanf(data + 1, "%d %d %d %ld", &cabid, &reg, &speedbyte, &functionMap);

    if (matched >= 3) {
        int8_t realThrottle = 0;
        Serial.print("speedbyte: ");
        Serial.println(speedbyte);

        // Conversion du SpeedByte en ton système (-127 à 127)
        if (speedbyte >= 130) {
            realThrottle = speedbyte - 129; // Marche avant
            direction = Direction::Forward;
        } else if (speedbyte >= 2 && speedbyte <= 127) {
            realThrottle = -(speedbyte); // Marche arrière
            direction = Direction::Reverse;
        } else {
            realThrottle = 0; // Arrêt (0 ou 128)
            direction = Direction::Forward;
        }

        if (_locoSpeedChanged)
            _locoSpeedChanged(static_cast<uint16_t>(cabid), realThrottle, direction);
    }
}

void CommandStationClient::handleTurnoutState(char data[], byte size)
{
    uint16_t id, state;

    int matched = sscanf(data + 1, "%u %u", &id, &state);
    if (matched == 2) {
        m_logStream.print(F("Update from turnout: "));
        m_logStream.print(id);
        m_logStream.print(F(" state is: "));
        m_logStream.println(state);
    }
}

void CommandStationClient::processPendingCommand()
{
    if (millis() - lastSendTime < COMMAND_COOLDOWN) return;

    currentLocomotiveIndex = (currentLocomotiveIndex + 1) % MAX_LOCOMOTIVES;

    if (pendingLocomotives[currentLocomotiveIndex].isPending) {
        _setLocoSpeed(pendingLocomotives[currentLocomotiveIndex].address, 
            pendingLocomotives[currentLocomotiveIndex].speed, 
            pendingLocomotives[currentLocomotiveIndex].direction);

        pendingLocomotives[currentLocomotiveIndex].isPending = false;

        lastSendTime = millis();
        return;
    }

    if (!m_queue.isEmpty()) {
        if (m_queue.pop(m_stream)) {
            lastSendTime = millis();
            return;
        }
    }

}
