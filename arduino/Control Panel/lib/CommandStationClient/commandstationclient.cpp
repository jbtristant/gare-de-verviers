#include "commandstationclient.h"



///////////////////////////////////////////////////////////////////////////////
/// CommandStation                                                          ///
///////////////////////////////////////////////////////////////////////////////

CommandStationClient::CommandStationClient(Stream &stream, Stream &logStream) :
    _trackChanged(nullptr),
    _locoSpeedChanged(nullptr), 
    _turnoutStateChanged(nullptr),
    m_stream(stream),
    m_logStream(logStream),
    m_lastSendTime(0),
    m_currentLocomotiveIndex(0),
    m_locomotiveCount(0),
    m_trackCount(0)
{
    m_tracks[0].initialize(this, 'A', TrackType::Main);
    m_tracks[1].initialize(this, 'B', TrackType::Prog);
}

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

void CommandStationClient::powerTrack(OnOff onOff, TrackType track)
{
    if (track == TrackType::Both) {
        (onOff == OnOff::On) ? m_queue.push(F("<1>")) : m_queue.push(F("<0>"));
    } else if ( track == TrackType::Join) {
        (onOff == OnOff::On) ? m_queue.push(F("<1 JOIN>")) : m_queue.push(F("<0>"));
    } else if ( track == TrackType::Main) {
        (onOff == OnOff::On) ? m_queue.push(F("<1 MAIN>")) : m_queue.push(F("<0 MAIN>"));
    } else if ( track == TrackType::Prog) {
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

void CommandStationClient::setTrackChangedCallback(void (*trackChanged)())
{
    _trackChanged = trackChanged;
}

///////////////////////////////////////////////////////////////////////////////
/// Cabs, Locomotive, Roster                                                ///
///////////////////////////////////////////////////////////////////////////////

void CommandStationClient::askRosters()
{
    m_queue.push(F("<JR>"));
}

void CommandStationClient::askRostersInfo()
{
    for (uint8_t i = 0; i < m_locomotiveCount; ++i) 
        askRosterInfo(m_locomotives[i].getAddress());
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

void CommandStationClient::askRosterInfo(uint16_t address) 
{
    m_queue.push("<J R %u>", address);
}

void CommandStationClient::setLocoSpeed(uint16_t address, int8_t speed, Direction direction)
{
    int8_t speedValue = 0;
    if (speed < 0) {
        speedValue = -1; // Emergency stop
    } else if (0 <= speed && speed <= 127) {
        speedValue = speed;
    } else {
        speedValue = 0;
    }
    m_logStream.print(F("CommandStationClient::setLocoSpeed: "));
    m_logStream.println(speedValue);

    for (uint8_t i = 0; i < maxLocomotives; ++i) {
        if (m_pendingLocomotives[i].address == address) {
            m_pendingLocomotives[i].speed = speedValue;
            m_pendingLocomotives[i].direction = direction;
            m_pendingLocomotives[i].isPending = true;
            return;
        }
    }

    for (uint8_t i = 0; i < maxLocomotives; ++i) {
        if (m_pendingLocomotives[i].address == 0) {
            m_pendingLocomotives[i].address = address;
            m_pendingLocomotives[i].speed = speedValue;
            m_pendingLocomotives[i].direction = direction;
            m_pendingLocomotives[i].isPending = true;
            return;
        }
    };
}

void CommandStationClient::_setLocoSpeed(uint16_t address, int8_t speed, Direction direction)
{
    snprintf(m_generalWriteBuffer, sizeof(m_generalWriteBuffer), "<t %u %d %u>", address, speed, static_cast<uint8_t>(direction));
    m_logStream.print(F("CommandStationClient._setLocoSpeeed: "));
    m_logStream.println(m_generalWriteBuffer);
    m_stream.print(m_generalWriteBuffer);
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

void CommandStationClient::setTurnoutStateChangedCallback(void (*turnoutStateChanged)(uint16_t id, TurnoutState state))
{
    _turnoutStateChanged = turnoutStateChanged;
}

Track* CommandStationClient::getTrack(uint8_t index)
{
    if (index >= maxTracks) return nullptr;

    return &m_tracks[index];
}

uint8_t CommandStationClient::getTracksCount()
{
    return maxTracks;
}


Locomotive* CommandStationClient::getLocomotive(uint8_t index)
{
    if (index >= m_locomotiveCount) return nullptr;

    return &m_locomotives[index];
}

uint8_t CommandStationClient::getLocomotivesCount()
{
    return m_locomotiveCount;
}

Turnout* CommandStationClient::getTurnout(uint8_t index)
{
    if (index >= m_turnoutCount) return nullptr;

    return &m_turnouts[index];
}

uint8_t CommandStationClient::getTurnoutsCount()
{
    return m_turnoutCount;
}



///////////////////////////////////////////////////////////////////////////////
/// Turnouts/Points                                                         ///
///////////////////////////////////////////////////////////////////////////////

void CommandStationClient::askTurnouts()
{
    askTurnoutsJ();
}

void CommandStationClient::askTurnoutsJ()
{
    m_queue.push(F("<J>"));
}

void CommandStationClient::askTurnoutsT()
{
    m_queue.push(F("<T>"));
}

void CommandStationClient::askTurnoutsJT()
{
    m_queue.push(F("<JT>"));
}

void CommandStationClient::askTurnoutInfo(uint16_t id)
{
    m_queue.push("<JT %u>", id);
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
        m_lastSendTime = millis() + sendCommandInterval; // block sending message.
        if (arraySize < 255) { // One less than the size of the array
            inChar = m_stream.read(); // Read a character
            if (inChar == '<') {
                arraySize = 0;
                inData[0] = '\0';
                //return;
            }else if (inChar == '>') {
                inData[arraySize] = '\0';
                if (arraySize > 0) {
                    processCmd(inData, arraySize);
                }
                arraySize = 0;
                //return;
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
    m_logStream.print(F("Input data: "));
    m_logStream.println(inData);

    switch(data[0]) {
        case 'p':
            handlePowerTrackState(data, size);
            break;

        case 'j':
            handleJValues(data, size);
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
    //m_lastSendTime = millis() - sendCommandInterval + 10;
}

void CommandStationClient::handlePowerTrackState(char data[], byte size)
{
    uint16_t state;
    char track;

    int matched = sscanf(data + 1, "%u %c", &state, &track);

    if (matched == 1) {
        if (state == 1) {
            m_logStream.println(F("All tracks are on"));
            for (uint8_t i = 0; i < maxTracks; ++i)
                m_tracks[i].setPowerPrivate(OnOff::On);
         } else {
            m_logStream.println(F("All tracks are off"));
            for (uint8_t i = 0; i < maxTracks; ++i)
                m_tracks[i].setPowerPrivate(OnOff::Off);
        } 
    }    
    
    if (matched == 2) {
        m_logStream.print(F("Track "));
        m_logStream.print(track);
        state == 1 ? m_logStream.println(F(" is on")) : m_logStream.println(F(" is off"));
        if (track == 'A') {
            m_tracks[0].setPowerPrivate(state == 1 ? OnOff::On : OnOff::Off);
        } else if (track == 'B') {
            m_tracks[0].setPowerPrivate(state == 1 ? OnOff::On : OnOff::Off);
        } else if (track == 'J') {
            m_tracks[0].setPowerPrivate(state == 1 ? OnOff::On : OnOff::Off);
            m_tracks[1].setPowerPrivate(state == 1 ? OnOff::On : OnOff::Off);
            m_tracks[1].setTypePrivate(TrackType::Join);
       }
                
    }

    if (_trackChanged)
        _trackChanged();
}
        
void CommandStationClient::handleJValues(char data[], byte size)
{
    if (size < 2 || data[0] != 'j') return;

    char type = data[1]; // 'R', 'I', 'G' ou 'T'

    // On va stocker les pointeurs vers chaque argument trouvé
    char* args[5]; 
    byte argCount = 0;

    bool inQuotes = false;
    bool hasQuotes = false;
    char* start = &data[3]; // On commence après "jR "

    // Étape 1 : Découpage intelligent qui respecte les guillemets
    for (char* p = start; *p != '\0'; p++) {
        if (*p == '"') {
            inQuotes = !inQuotes; // On entre ou on sort des guillemets
            hasQuotes = true;
            if (inQuotes) {
                start = p + 1; // Le texte commence APRÈS le guillemet ouvrant
            } else {
                *p = '\0'; // On remplace le guillemet fermant par une fin de chaîne
                if (argCount < 5) args[argCount++] = start;
            }
        } 
        else if (*p == ' ' && !inQuotes) {
            // Un espace HORS des guillemets sépare deux arguments
            *p = '\0';
            if (p != start && argCount < 5) {
                args[argCount++] = start;
            }
            start = p + 1;
        }
    }
    // Ne pas oublier le tout dernier argument s'il ne se finissait pas par un espace/guillemet
    if (*start != '\0' && argCount < 5 && !inQuotes) {
        args[argCount++] = start;
    }

    // Étape 2 : Traitement selon le type
    if (type == 'R') {
        if (hasQuotes) {
            if (argCount >= 3) {
                // Cas de la loco unique : jR 64 "SNCB 64045" "Lights/F5/F6"
                uint16_t address = atoi(args[0]);
                bool found = false;
                uint8_t locoIndex = 0;
                for (uint8_t i = 0; i < m_locomotiveCount && m_locomotiveCount; ++i) {
                    if (m_locomotives[i].getAddress() == address) {
                        locoIndex = i;
                        found = true;
                        break;
                    }
                }
                if (found) {
                    m_locomotives[locoIndex].setName(args[1]);
                    m_locomotives[locoIndex].setFunctions(args[2]);
                    // Optionnel : Tu peux appeler une fonction pour mettre à jour ton interface ici
                }
            }
        } 
        else {
            // Cas de la liste globale de locos : jR 64 32 12 ...
            m_locomotiveCount = 0;
            for (uint8_t i = 0; i < argCount && m_locomotiveCount < maxLocomotives; ++i) {
                m_locomotives[m_locomotiveCount].initialize(this, atoi(args[i]));
                //askLocoInfo(locomotives[locomotiveCount].address);
                askRosterInfo(m_locomotives[m_locomotiveCount].getAddress());
                ++m_locomotiveCount;
            }
        }
    }
    else if (type == 'T') {
        if (hasQuotes) {
            if (argCount >= 3) {
                uint16_t id = atoi(args[0]);
                bool found = false;
                uint8_t turnoutIndex = 0;
                for (uint8_t i = 0; i < m_turnoutCount && m_turnoutCount; ++i) {
                    if (m_turnouts[i].getId() == id) {
                        turnoutIndex = i;
                        found = true;
                        break;
                    }
                }
                if (found) {
                    m_turnouts[turnoutIndex].setStatePrivate(TurnoutState(atoi(args[2])));
                    m_turnouts[turnoutIndex].setName(args[2]);
                    // Optionnel : Tu peux appeler une fonction pour mettre à jour ton interface ici
                }
            }
        } 
        else {
            // Cas de la liste globale de aiguilles : jT 64 32 12 ...

            for (uint8_t i = 0; i < argCount && m_turnoutCount < maxTurnouts; ++i) {
                uint16_t id = atoi(args[i]);
                bool found = false;

                for (uint8_t e = 0; e < m_turnoutCount && m_turnoutCount < maxTurnouts; ++e) {
                    if (m_turnouts[e].getId() == id) {
                        found = true;
                    }
                }
                if (!found && m_turnoutCount < maxTurnouts) {
                    m_turnouts[m_turnoutCount].initialize(this, id, TurnoutState::Undefined);
                    m_turnoutCount++;
                    askTurnoutInfo(id);
                }
            }
        }
    }
    else if (type == 'I') {
        m_trackCount = 0;
        
        for (uint8_t i = 0; i < argCount && m_trackCount < maxTracks; ++i) {
            m_tracks[i].setCurrent(atoi(args[i]));
        }

        if (_trackChanged)
            _trackChanged();
    }
    else if (type == 'G') {
        m_trackCount = 0;
        
        for (uint8_t i = 0; i < argCount && m_trackCount < maxTracks; ++i) {
            m_tracks[i].setMaxCurrent(atoi(args[i]));
        }

        if (_trackChanged)
            _trackChanged();
    }
}

void CommandStationClient::handleLocoUpdate(char data[], byte size)
{
    int cabid, reg; 
    uint8_t speedbyte;
    byte functionMap;
    Direction direction;
        
    int8_t realThrottle = 0;

    // sscanf cherche le format : l [espace] entier [espace] entier...
    // On commence à data+1 pour ignorer le 'l'
    int matched = sscanf(data + 1, "%d %d %hhu %hhu", &cabid, &reg, &speedbyte, &functionMap);

    if (matched == 4) {
        Serial.print(F("4 speedbyte: "));
        Serial.println(speedbyte);

        // Conversion du SpeedByte en ton système (-127 à 127)
        if (speedbyte == 0) {
            // Stop
            realThrottle = 0;
            direction = Direction::Reverse;
        } else if (speedbyte == 1) {
            // Emergency Stop
            realThrottle = 0;
            direction = Direction::Reverse;
        } else if (2 <= speedbyte && speedbyte <= 127) {
            realThrottle = speedbyte - 1;
            direction = Direction::Reverse;
        } else if (speedbyte == 128) {
            // Stop
            realThrottle = 0;
            direction = Direction::Forward;
        } else if (speedbyte == 129) {
            // Emergency Stop
            realThrottle = 0;
            direction = Direction::Forward;
        }  else if (130 <= speedbyte && speedbyte <= 255) {
            realThrottle = speedbyte - 129; // Marche avant : 129 à 255 -> 1 à 126
            direction = Direction::Forward;
        } else {
            realThrottle = 0;
            direction = Direction::Forward;
        }

        for(uint8_t i = 0; i < m_locomotiveCount; ++i) {
            if (m_locomotives[i].getAddress() == static_cast<uint16_t>(cabid)) {
                m_locomotives[i].setSpeedPrivate(realThrottle, direction);
            }
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

        bool found = false;

        for (uint8_t i = 0; i < m_turnoutCount && m_turnoutCount < maxTurnouts; ++i) {
            if (m_turnouts[i].getId() == id) {
                m_turnouts[i].setStatePrivate(TurnoutState(state));
                found = true;
            }
        }
        if (!found && m_turnoutCount < maxTurnouts) {
            m_turnouts[m_turnoutCount].initialize(this, id, TurnoutState(state));
            m_turnoutCount++;
            askTurnoutInfo(id);
        }

        if (_turnoutStateChanged)
            _turnoutStateChanged(id, TurnoutState(state));

    }
}

void CommandStationClient::processPendingCommand()
{
    if (millis() - m_lastSendTime < sendCommandInterval) return;

    m_currentLocomotiveIndex = (m_currentLocomotiveIndex + 1) % maxLocomotives;

    if (m_pendingLocomotives[m_currentLocomotiveIndex].isPending) {
        _setLocoSpeed(m_pendingLocomotives[m_currentLocomotiveIndex].address, 
            m_pendingLocomotives[m_currentLocomotiveIndex].speed, 
            m_pendingLocomotives[m_currentLocomotiveIndex].direction);

        m_pendingLocomotives[m_currentLocomotiveIndex].isPending = false;

        m_lastSendTime = millis();
        return;
    }

    if (!m_queue.isEmpty()) {
        if (m_queue.pop(m_stream)) {
            m_lastSendTime = millis();
            return;
        }
    }

}
