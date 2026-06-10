#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include <U8g2lib.h>
#include <queue.h>
#include <semphr.h>

#include "mainmenu.h"

#include "commandstationclient.h"
#include "locomotive.h"
#include "track.h"
#include "turnout.h"

MainMenu::MainMenu()
    : m_commandStationClient(nullptr), m_screenMenu(nullptr), m_logStream(nullptr), m_xLogStreamSemaphore(NULL), m_currentContext(MAIN_MENU), m_menuIndex(0)
{
}

void MainMenu::initialize(CommandStationClient *client, U8G2_SSD1306_128X64_NONAME_F_HW_I2C *screenMenu, Stream *logStream,
                          SemaphoreHandle_t xLogStreamSemaphore)
{
    m_commandStationClient = client;
    m_screenMenu = screenMenu;
    m_logStream = logStream;
    m_xLogStreamSemaphore = xLogStreamSemaphore;
    drawUI();
}

void MainMenu::buttonPress()
{
    handlePress();
    drawUI();
}

void MainMenu::onTrackChanged()
{
    if (m_currentContext == TRACK_MENU)
        drawUI();
}

void MainMenu::onLocomotiveChanged()
{
    if (m_currentContext == DRIVING_MODE)
        drawUI();
}

void MainMenu::onTurnoutStateChanged(uint16_t id, TurnoutState state)
{
    if (m_currentContext == TURNOUT_MENU)
        drawUI();
}

void MainMenu::menuDown()
{
    switch (m_currentContext) {
    case MAIN_MENU:
        m_menuIndex++;
        if (m_menuIndex >= m_mainMenuSize)
            m_menuIndex = 0;
        break;
    case TRACK_MENU:
        m_menuIndex++;
        if (m_menuIndex > m_commandStationClient->getTracksCount())
            m_menuIndex = 0;
        break;

    case LOCO_MENU:
        m_menuIndex++;
        if (m_menuIndex > m_commandStationClient->getLocomotivesCount())
            m_menuIndex = 0;
        break;

    case DRIVING_MODE:
        Locomotive *locomotive = m_commandStationClient->getLocomotive(m_menuIndex);
        if (locomotive == nullptr)
            return;
        int8_t speed = locomotive->getSpeed();
        Direction direction = locomotive->getDirection();
        if (xSemaphoreTake(m_xLogStreamSemaphore, portMAX_DELAY) == pdTRUE) {
            m_logStream->print(F("Driving mode 'down' speed: "));
            m_logStream->print(speed);
            m_logStream->print(F(" direction "));
            (direction == Direction::Forward) ? m_logStream->println(F("Forward")) : m_logStream->println(F("Reverse"));
            xSemaphoreGive(m_xLogStreamSemaphore);
        }

        if (direction == Direction::Forward) {
            if (speed < 126 - m_locomotiveSpeedStep) {
                speed += m_locomotiveSpeedStep;
            } else if (126 - m_locomotiveSpeedStep && m_locomotiveSpeedStep < 126) {
                speed++;
            }
        } else {
            if (speed < m_locomotiveSpeedStep) {
                speed--;
            } else {
                speed -= m_locomotiveSpeedStep;
            }
            if (speed <= 0) {
                direction = Direction::Forward;
                speed = 0;
            }
        }
        if (xSemaphoreTake(m_xLogStreamSemaphore, portMAX_DELAY) == pdTRUE) {
            m_logStream->print(F("Driving mode 'down' new speed: "));
            m_logStream->print(speed);
            m_logStream->print(F(" direction "));
            (direction == Direction::Forward) ? m_logStream->println(F("Forward")) : m_logStream->println(F("Reverse"));
            xSemaphoreGive(m_xLogStreamSemaphore);
        }

        locomotive->setSpeed(speed, direction);
        break;

    case TURNOUT_MENU:
        m_menuIndex++;
        if (m_menuIndex > m_commandStationClient->getTurnoutsCount())
            m_menuIndex = 0;
        break;

    case STATUS_MENU:
        break;

    case CONFIGURATION_MENU:
        break;

    default:
        m_currentContext = MAIN_MENU;
        break;
    }
    drawUI();
}

void MainMenu::menuUp()
{
    switch (m_currentContext) {
    case MAIN_MENU:
        if (m_menuIndex == 0)
            m_menuIndex = m_mainMenuSize - 1;
        else
            m_menuIndex--;
        break;

    case TRACK_MENU:
        if (m_menuIndex == 0)
            m_menuIndex = m_commandStationClient->getTracksCount();
        else
            m_menuIndex--;
        break;

    case LOCO_MENU:
        if (m_menuIndex == 0)
            m_menuIndex = m_commandStationClient->getLocomotivesCount();
        else
            m_menuIndex--;
        break;

    case DRIVING_MODE:
        Locomotive *locomotive = m_commandStationClient->getLocomotive(m_menuIndex);
        if (locomotive == nullptr)
            return;
        int8_t speed = locomotive->getSpeed();
        Direction direction = locomotive->getDirection();
        if (xSemaphoreTake(m_xLogStreamSemaphore, portMAX_DELAY) == pdTRUE) {
            m_logStream->print(F("Driving mode 'up' speed: "));
            m_logStream->print(speed);
            m_logStream->print(F(" direction "));
            (direction == Direction::Forward) ? m_logStream->println(F("Forward")) : m_logStream->println(F("Reverse"));
            xSemaphoreGive(m_xLogStreamSemaphore);
        }
        if (direction == Direction::Reverse) {
            if (speed < 126 - m_locomotiveSpeedStep) {
                speed += m_locomotiveSpeedStep;
            } else if (126 - m_locomotiveSpeedStep && m_locomotiveSpeedStep < 126) {
                speed++;
            }
        } else {
            if (speed < m_locomotiveSpeedStep) {
                speed--;
            } else {
                speed -= m_locomotiveSpeedStep;
            }
            if (speed <= 0) {
                direction = Direction::Reverse;
                speed = 0;
            }
        }
        if (xSemaphoreTake(m_xLogStreamSemaphore, portMAX_DELAY) == pdTRUE) {
            m_logStream->print(F("Driving mode 'up' new speed: "));
            m_logStream->print(speed);
            m_logStream->print(F(" direction "));
            (direction == Direction::Forward) ? m_logStream->println(F("Forward")) : m_logStream->println(F("Reverse"));
            xSemaphoreGive(m_xLogStreamSemaphore);
        }
        locomotive->setSpeed(speed, direction);
        break;

    case TURNOUT_MENU:
        if (m_menuIndex == 0)
            m_menuIndex = m_commandStationClient->getTurnoutsCount();
        else
            m_menuIndex--;
        break;

    case STATUS_MENU:
        break;

    case CONFIGURATION_MENU:
        break;

    default:
        m_currentContext = MAIN_MENU;
        break;
    }
    drawUI();
}

void MainMenu::handlePress()
{
    switch (m_currentContext) {
    case MAIN_MENU:
        switch (m_menuIndex) {
        case 0:
            m_currentContext = TRACK_MENU;
            break;
        case 1:
            m_commandStationClient->getLocomotive(m_menuIndex)->askLocoInfo();
            m_currentContext = LOCO_MENU;
            break;
        case 2:
            m_commandStationClient->askTurnoutsT();
            m_currentContext = TURNOUT_MENU;
            break;
        case 3:
            m_currentContext = STATUS_MENU;
            break;
        case 4:
            m_currentContext = CONFIGURATION_MENU;
            break;
        default:
            m_currentContext = MAIN_MENU;
            break;
        }
        m_menuIndex = 0;
        break;

    case TRACK_MENU:
        if (m_menuIndex == m_commandStationClient->getTracksCount()) {
            m_currentContext = MAIN_MENU;
            m_menuIndex = 0;
        } else {
            if (m_menuIndex == 0) {
                if (m_commandStationClient->getTrack(0)->getPower() != OnOff::On) {
                    m_commandStationClient->powerTrack(OnOff::On, TrackType::Join);
                } else {
                    m_commandStationClient->powerTrack(OnOff::Off, TrackType::Both);
                }
            }
            if (m_menuIndex == 1) {
                switch (m_commandStationClient->getTrack(1)->getType()) {
                case TrackType::Prog:
                    m_commandStationClient->powerTrack(OnOff::On, TrackType::Join);
                    break;
                case TrackType::Main:
                case TrackType::Join:
                    m_commandStationClient->powerTrack(OnOff::On, TrackType::Prog);
                    break;
                default:
                    m_commandStationClient->powerTrack(OnOff::On, TrackType::Join);
                    break;
                }
            }
        }
        break;

    case LOCO_MENU:
        if (m_menuIndex == m_commandStationClient->getLocomotivesCount()) {
            m_currentContext = MAIN_MENU;
            m_menuIndex = 1;
        } else {
            m_currentContext = DRIVING_MODE;
            // pas de changement d'index, on le garde pour la loco et le retour
            // au menu
        }
        break;

    case DRIVING_MODE:
        m_currentContext = LOCO_MENU;
        break;

    case TURNOUT_MENU:
        if (m_menuIndex == m_commandStationClient->getTurnoutsCount()) {
            m_currentContext = MAIN_MENU;
            m_menuIndex = 0;
        } else {
            // m_currentContext = TURNOUT_MODE;
            // pas de changement d'index, on le garde pour la sélection de
            // l'aiguillage
            Turnout *turnout = m_commandStationClient->getTurnout(m_menuIndex);
            if (turnout == nullptr)
                break;
            if (turnout->getState() == TurnoutState::Close) {
                turnout->setState(TurnoutState::Throw);
            } else {
                turnout->setState(TurnoutState::Close);
            }
        }
        break;

    case STATUS_MENU:
        m_currentContext = MAIN_MENU;
        break;

    case CONFIGURATION_MENU:
        m_currentContext = MAIN_MENU;
        break;

    default:
        m_currentContext = MAIN_MENU;
        break;
    }
}

void MainMenu::writeConsoleLogLine()
{
    if (xSemaphoreTake(m_xLogStreamSemaphore, portMAX_DELAY) == pdTRUE) {
        m_logStream->println(F("================================="));
        xSemaphoreGive(m_xLogStreamSemaphore);
    }
}

void MainMenu::writeConsoleLogReturn(uint8_t size)
{
    if (xSemaphoreTake(m_xLogStreamSemaphore, portMAX_DELAY) == pdTRUE) {
        m_logStream->println((m_menuIndex == size) ? F("> [Retour]") : F("  [Retour]"));
        xSemaphoreGive(m_xLogStreamSemaphore);
    }
}

void MainMenu::drawUI()
{
    writeConsoleLogLine();
    switch (m_currentContext) {
    case MAIN_MENU:
        if (xSemaphoreTake(m_xLogStreamSemaphore, portMAX_DELAY) == pdTRUE) {
            m_logStream->println(F("[ MENU PRINCIPAL ]"));
            m_logStream->println((m_menuIndex == 0) ? F("> Voies") : F("  Voies"));
            m_logStream->println((m_menuIndex == 1) ? F("> Locomotives") : F("  Locomotives"));
            m_logStream->println((m_menuIndex == 2) ? F("> Aiguillages") : F("  Aiguillages"));
            m_logStream->println((m_menuIndex == 3) ? F("> Statut réseau") : F("  Statut réseau"));
            m_logStream->println((m_menuIndex == 4) ? F("> Configuration") : F("  Configuration"));
            xSemaphoreGive(m_xLogStreamSemaphore);
        }
        drawMainMenu();
        break;

    case TRACK_MENU:
        if (xSemaphoreTake(m_xLogStreamSemaphore, portMAX_DELAY) == pdTRUE) {
            m_logStream->println(F("[ VOIES ]"));
            xSemaphoreGive(m_xLogStreamSemaphore);
        }
        for (uint8_t i = 0; i < maxTracks; ++i) {
            Track *track = m_commandStationClient->getTrack(i);
            if (xSemaphoreTake(m_xLogStreamSemaphore, portMAX_DELAY) == pdTRUE) {
                if (m_menuIndex == i) {
                    m_logStream->print(F("> "));
                } else {
                    m_logStream->print(F("  "));
                }
                m_logStream->print(track->getName());
                m_logStream->print(F("  "));
                m_logStream->print(onOffToCString(track->getPower()));
                m_logStream->print(F("  "));
                m_logStream->println(trackModeToCString(track->getMode()));
                xSemaphoreGive(m_xLogStreamSemaphore);
            }
        }
        writeConsoleLogReturn(m_commandStationClient->getLocomotivesCount());
        drawTrackMenu();
        break;

        // case TRACK_MODE:
        //      if (xSemaphoreTake(m_xLogStreamSemaphore, portMAX_DELAY) == pdTRUE) {
        //          m_logStream->print(F("[ TRACK: "));
        //          m_logStream->print(m_commandStationClient->getTrack(m_menuIndex)->getName());
        //          m_logStream->println(F(" ]"));
        //          xSemaphoreGive(m_xLogStreamSemaphore);
        //      }
        //     drawTrackMode();
        //     break;

    case LOCO_MENU:
        if (xSemaphoreTake(m_xLogStreamSemaphore, portMAX_DELAY) == pdTRUE) {
            m_logStream->println(F("[ SELECTION LOCO ]"));
            xSemaphoreGive(m_xLogStreamSemaphore);
        }
        for (uint8_t i = 0; i < m_commandStationClient->getLocomotivesCount(); i++) {
            if (xSemaphoreTake(m_xLogStreamSemaphore, portMAX_DELAY) == pdTRUE) {
                m_logStream->println((m_menuIndex == i) ? String("> ") + m_commandStationClient->getLocomotive(i)->getName()
                                                        : String("  ") + m_commandStationClient->getLocomotive(i)->getName());
                xSemaphoreGive(m_xLogStreamSemaphore);
            }
        }
        writeConsoleLogReturn(m_commandStationClient->getLocomotivesCount());
        drawLocomotiveMenu();
        break;

    case DRIVING_MODE:
        if (xSemaphoreTake(m_xLogStreamSemaphore, portMAX_DELAY) == pdTRUE) {
            m_logStream->print(F("[ PILOTAGE : "));
            m_logStream->print(m_commandStationClient->getLocomotive(m_menuIndex)->getName());
            m_logStream->println(F(" ]"));
            m_logStream->print(F(" Vitesse actuelle : "));
            m_logStream->println(m_commandStationClient->getLocomotive(m_menuIndex)->getSpeed());
            m_logStream->println(F("\n > CLIC pour STOP & RETOUR"));
            xSemaphoreGive(m_xLogStreamSemaphore);
        }
        drawDrivingMode();
        break;

    case TURNOUT_MENU:
        if (xSemaphoreTake(m_xLogStreamSemaphore, portMAX_DELAY) == pdTRUE) {
            m_logStream->println(F("[ Aiguillages ]"));
            xSemaphoreGive(m_xLogStreamSemaphore);
        }
        for (uint8_t i = 0; i < m_commandStationClient->getTurnoutsCount(); i++) {
            Turnout *turnout = m_commandStationClient->getTurnout(i);
            if (xSemaphoreTake(m_xLogStreamSemaphore, portMAX_DELAY) == pdTRUE) {
                if (m_menuIndex == i) {
                    m_logStream->print(F("> "));
                } else {
                    m_logStream->print(F("  "));
                }
                m_logStream->print(turnout->getName());
                if (turnout->getState() == TurnoutState::Close) {
                    m_logStream->println(F("  C"));
                } else if (turnout->getState() == TurnoutState::Throw) {
                    m_logStream->println(F("  T"));
                } else if (turnout->getState() == TurnoutState::eXamine) {
                    m_logStream->println(F("  X"));
                } else if (turnout->getState() == TurnoutState::Undefined) {
                    m_logStream->println(F("  U"));
                } else {
                    m_logStream->println(F("  I"));
                }
                xSemaphoreGive(m_xLogStreamSemaphore);
            }
        }
        writeConsoleLogReturn(m_commandStationClient->getTurnoutsCount());
        drawTurnoutMenu();
        break;

    case STATUS_MENU:
        if (xSemaphoreTake(m_xLogStreamSemaphore, portMAX_DELAY) == pdTRUE) {
            m_logStream->println(F("[ STATUT SYSTEME ]"));
            // Ici tu liras les vraies variables de ta centrale DCC-EX plus tard
            m_logStream->println(F(" SIGNAL DCC : ACTIF"));
            m_logStream->println(F(" COURANT    : 420 mA"));
            m_logStream->println(F(" TENSION    : 14.8 V"));
            m_logStream->print(F(" LOCOS ACT. : "));
            m_logStream->println(m_commandStationClient->getLocomotivesCount());
            m_logStream->print(F(" AIGUI ACT. : "));
            m_logStream->println(m_commandStationClient->getTurnoutsCount());
            m_logStream->println(F("\n > CLIC POUR RETOUR"));
            xSemaphoreGive(m_xLogStreamSemaphore);
        }
        drawStatusMenu();
        break;

    case CONFIGURATION_MENU:
        break;

    default:
        m_currentContext = MAIN_MENU;
        break;
    }
    writeConsoleLogLine();
}

void MainMenu::drawMainMenu()
{
    m_screenMenu->clearBuffer(); // 1. Efface l'écran

    // 2. Choix de la police pour le titre (ex: un peu plus grande ou en
    // gras)
    m_screenMenu->setFont(u8g2_font_6x12_tf);
    m_screenMenu->drawStr(10, 10, "MENU PRINCIPAL"); // x=0, y=10
    m_screenMenu->drawHLine(0, 13, 128);

    // 3. Choix de la police pour les éléments du menu
    m_screenMenu->setFont(u8g2_font_6x10_tf);

    // Tableau contenant les intitulés du menu dans l'ordre
    const char *menuItems[] = {"Voies", "Locomotives", "Aiguillages", "Statut reseau", "Configuration"};

    // Calcul de la fenêtre glissante (topIndex)
    uint8_t topIndex = 0;
    if (m_menuIndex >= m_screenMenuMaxVisibleLines) {
        topIndex = m_menuIndex - m_screenMenuMaxVisibleLines + 1;
    }

    // 4. Boucle d'affichage des lignes visibles
    uint8_t ligneGraphique = 0;
    for (uint8_t i = topIndex; i < m_mainMenuSize && ligneGraphique < m_screenMenuMaxVisibleLines; i++) {
        // Calcul de la position Y : ligne 1 à y=25, ligne 2 à y=37, etc.
        // (pas de 12 pixels)
        int yPos = 25 + (ligneGraphique * 12);

        char buffer[22];
        // Ajout de la flèche de sélection ou de l'espace d'alignement
        snprintf(buffer, sizeof(buffer), "%s%s", (m_menuIndex == i) ? "> " : "  ", menuItems[i]);

        // Dessin de la ligne de texte
        m_screenMenu->drawStr(0, yPos, buffer);
        ligneGraphique++;
    }

    // 5. Astuce : Indicateurs visuels de défilement (Flèches)
    // S'il y a des éléments cachés au-dessus (on a scrollé vers le bas)
    if (topIndex > 0) {
        m_screenMenu->drawStr(120, 22, "^");
    }
    // S'il y a des éléments cachés en-dessous
    if (topIndex + m_screenMenuMaxVisibleLines < m_mainMenuSize) {
        m_screenMenu->drawStr(120, 62, "v");
    }

    m_screenMenu->sendBuffer(); // Envoie le dessin à l'écran
}

void MainMenu::dessinerMenuOptionnel()
{
    m_screenMenu->clearBuffer();

    // Titre
    m_screenMenu->setFont(u8g2_font_6x12_tf);
    m_screenMenu->drawStr(10, 10, "GARE DE VERVIERS");
    m_screenMenu->drawHLine(0, 13,
                            128); // Petite ligne de séparation sous le titre

    m_screenMenu->setFont(u8g2_font_6x10_tf);

    // Tableau des textes pour simplifier le code
    const char *menuItems[] = {"Locomotives", "Aiguillages", "Statut reseau", "Configuration"};

    for (int i = 0; i < 4; i++) {
        int yPos = 26 + (i * 12); // Calcule la position Y pour chaque ligne
                                  // (espacement de 12 pixels)

        if (m_menuIndex == i) {
            // Si c'est l'élément sélectionné : on dessine un rectangle
            // plein en fond
            m_screenMenu->setDrawColor(1);
            m_screenMenu->drawBox(0, yPos - 9, 128,
                                  11); // x, y, largeur, hauteur

            // On écrit le texte en mode "inverse" (texte noir sur fond
            // blanc)
            m_screenMenu->setDrawColor(0);
            m_screenMenu->drawStr(4, yPos, menuItems[i]);

            // On repasse en mode normal pour la suite
            m_screenMenu->setDrawColor(1);
        } else {
            // Élément non sélectionné : texte normal
            m_screenMenu->drawStr(4, yPos, menuItems[i]);
        }
    }

    m_screenMenu->sendBuffer();
}

void MainMenu::drawTrackMenu()
{
    m_screenMenu->clearBuffer();

    // 1. DESSINER LE TITRE
    m_screenMenu->setFont(u8g2_font_6x12_tf);
    m_screenMenu->drawStr(10, 10, "SELECTION VOIE");
    m_screenMenu->drawHLine(0, 13, 128); // Ligne de séparation

    m_screenMenu->setFont(u8g2_font_6x10_tf);

    // 2. CONFIGURATION DU DÉFILEMENT
    uint8_t totalTracks = m_commandStationClient->getTracksCount();
    uint8_t totalElements = totalTracks + 1; // +1 pour l'option [Retour]

    // Calcul de l'index de départ pour le défilement
    uint8_t topIndex = 0;
    if (m_menuIndex >= m_screenMenuMaxVisibleLines) {
        topIndex = m_menuIndex - m_screenMenuMaxVisibleLines + 1;
    }

    // 3. BOUCLE D'AFFICHAGE
    int ligneGraphique = 0;

    for (uint8_t i = topIndex; i < totalElements && ligneGraphique < m_screenMenuMaxVisibleLines; i++) {
        int yPos = 25 + (ligneGraphique * 12); // Espacement de 12 pixels en vertical

        char buffer[22] = ""; // Conteneur pour la ligne de texte

        // Flèche de sélection
        if (m_menuIndex == i) {
            strcpy(buffer, "> ");
        } else {
            strcpy(buffer, "  ");
        }

        // CAS 1 : C'est une voie
        if (i < totalTracks) {
            Track *track = m_commandStationClient->getTrack(i);

            // On ajoute le nom de l'aiguillage
            buffer[2] = track->getName();
            buffer[3] = '\0';

            strcat(buffer, "  ");
            strcat(buffer, onOffToCString(track->getPower()));
            strcat(buffer, "  ");
            strcat(buffer, trackTypeToCString(track->getType()));
        }
        // CAS 2 : C'est le bouton [Retour]
        else if (i == totalTracks) {
            snprintf(buffer, sizeof(buffer), "%s[Retour]", (m_menuIndex == i) ? "> " : "  ");
        }

        // Affichage sur l'écran
        m_screenMenu->drawStr(0, yPos, buffer);
        ligneGraphique++;
    }

    // 4. INDICATEURS DE DEFILEMENT (Optionnel mais pratique)
    if (topIndex > 0) {
        m_screenMenu->drawStr(122, 22, "^"); // Il y a des locos au-dessus
    }
    if (topIndex + m_screenMenuMaxVisibleLines < totalElements) {
        m_screenMenu->drawStr(122, 62,
                              "v"); // Il y a des locos (ou le bouton retour) en-dessous
    }

    m_screenMenu->sendBuffer();
}

// void MainMenu::drawTrackMode() {
//     m_screenMenu->clearBuffer();

//     Track* track = m_commandStationClient->getTrack(m_menuIndex);
//     if (track == nullptr) return;

//     // DESSINER LE TITRE
//     m_screenMenu->setFont(u8g2_font_6x12_tf);
//     char titreBuffer[22];
//     snprintf(titreBuffer, sizeof(titreBuffer), "[ Voie %c ]",
//     track->getName()); m_screenMenu->drawStr(0, 10, titreBuffer);
//     m_screenMenu->drawHLine(0, 13, 128); // Ligne de séparation

//     m_screenMenu->setFont(u8g2_font_6x10_tf);

//     m_screenMenu->sendBuffer();
// }

void MainMenu::drawLocomotiveMenu()
{
    m_screenMenu->clearBuffer();

    // 1. DESSINER LE TITRE
    m_screenMenu->setFont(u8g2_font_6x12_tf);
    m_screenMenu->drawStr(10, 10, "SELECTION LOCO");
    m_screenMenu->drawHLine(0, 13, 128); // Ligne de séparation

    m_screenMenu->setFont(u8g2_font_6x10_tf);

    // 2. CONFIGURATION DU DÉFILEMENT
    uint8_t totalLocos = m_commandStationClient->getLocomotivesCount();
    uint8_t totalElements = totalLocos + 1; // +1 pour l'option [Retour]

    // Calcul de l'index de départ pour le défilement
    uint8_t topIndex = 0;
    if (m_menuIndex >= m_screenMenuMaxVisibleLines) {
        topIndex = m_menuIndex - m_screenMenuMaxVisibleLines + 1;
    }

    // 3. BOUCLE D'AFFICHAGE
    int ligneGraphique = 0;

    for (uint8_t i = topIndex; i < totalElements && ligneGraphique < m_screenMenuMaxVisibleLines; i++) {
        int yPos = 25 + (ligneGraphique * 12); // Espacement de 12 pixels en vertical

        char buffer[22] = ""; // Conteneur pour la ligne de texte

        // CAS 1 : C'est une locomotive
        if (i < totalLocos) {
            const char *nomLoco = m_commandStationClient->getLocomotive(i)->getName();

            // On compose la ligne : "> Nom" ou "  Nom"
            snprintf(buffer, sizeof(buffer), "%s%s", (m_menuIndex == i) ? "> " : "  ", nomLoco);
        }
        // CAS 2 : C'est le bouton [Retour]
        else if (i == totalLocos) {
            snprintf(buffer, sizeof(buffer), "%s[Retour]", (m_menuIndex == i) ? "> " : "  ");
        }

        // Affichage sur l'écran
        m_screenMenu->drawStr(0, yPos, buffer);
        ligneGraphique++;
    }

    // 4. INDICATEURS DE DEFILEMENT (Optionnel mais pratique)
    if (topIndex > 0) {
        m_screenMenu->drawStr(122, 22, "^"); // Il y a des locos au-dessus
    }
    if (topIndex + m_screenMenuMaxVisibleLines < totalElements) {
        m_screenMenu->drawStr(122, 62,
                              "v"); // Il y a des locos (ou le bouton retour) en-dessous
    }

    m_screenMenu->sendBuffer();
}

void MainMenu::drawTurnoutMenu()
{
    m_screenMenu->clearBuffer();

    // 1. DESSINER LE TITRE (Fixe en haut)
    m_screenMenu->setFont(u8g2_font_6x12_tf);
    m_screenMenu->drawStr(10, 10, "Aiguillages");
    m_screenMenu->drawHLine(0, 13,
                            128); // Ligne de séparation sous le titre

    m_screenMenu->setFont(u8g2_font_6x10_tf);

    // 2. CONFIGURATION DU DÉFILEMENT
    uint8_t totalAiguillages = m_commandStationClient->getTurnoutsCount();
    uint8_t totalElements = totalAiguillages + 1; // +1 pour l'option [Retour]

    // Calcul de la première ligne à afficher (la "fenêtre" qui glisse)
    uint8_t topIndex = 0;
    if (m_menuIndex >= m_screenMenuMaxVisibleLines) {
        topIndex = m_menuIndex - m_screenMenuMaxVisibleLines + 1;
    }

    // 3. BOUCLE D'AFFICHAGE DES LIGNES VISIBLES
    int ligneGraphique = 0; // Compteur pour positionner sur l'écran (0 à 3)

    for (uint8_t i = topIndex; i < totalElements && ligneGraphique < m_screenMenuMaxVisibleLines; i++) {
        int yPos = 25 + (ligneGraphique * 12); // Espacement vertical de 12 pixels

        // On prépare la chaîne de texte à afficher pour cette ligne
        char buffer[22] = ""; // Largeur de l'écran ~21 caractères en font 6x10

        // Flèche de sélection
        if (m_menuIndex == i) {
            strcpy(buffer, "> ");
        } else {
            strcpy(buffer, "  ");
        }

        // CAS 1 : C'est un aiguillage
        if (i < totalAiguillages) {
            Turnout *turnout = m_commandStationClient->getTurnout(i);

            // On ajoute le nom de l'aiguillage
            strcat(buffer, turnout->getName());

            // On ajoute l'état (C, T ou X)
            if (turnout->getState() == TurnoutState::Close) {
                strcat(buffer, " [C]");
            } else if (turnout->getState() == TurnoutState::Throw) {
                strcat(buffer, " [T]");
            } else if (turnout->getState() == TurnoutState::eXamine) {
                strcat(buffer, " [X]");
            } else if (turnout->getState() == TurnoutState::Undefined) {
                strcat(buffer, " [U]");
            } else {
                strcat(buffer, " [I]");
            }
        }
        // CAS 2 : C'est la dernière ligne, le bouton [Retour]
        else if (i == totalAiguillages) {
            strcat(buffer, "[Retour]");
        }

        // Dessin effectif de la ligne sur l'écran
        m_screenMenu->drawStr(0, yPos, buffer);
        ligneGraphique++;
    }

    // 4. PETIT BONUS : Indicateur visuel de défilement (Scrollbar
    // simplifiée) S'il y a plus d'éléments que de place, on dessine des
    // flèches ou un indicateur
    if (topIndex > 0) {
        m_screenMenu->drawStr(122, 22,
                              "^"); // Flèche vers le haut s'il y a du contenu au-dessus
    }
    if (topIndex + m_screenMenuMaxVisibleLines < totalElements) {
        m_screenMenu->drawStr(122, 62,
                              "v"); // Flèche vers le bas s'il y a du contenu en dessous
    }

    m_screenMenu->sendBuffer();
}

void MainMenu::drawDrivingMode()
{
    m_screenMenu->clearBuffer();

    // 1. RÉCUPÉRATION DES DONNÉES
    Locomotive *loco = m_commandStationClient->getLocomotive(m_menuIndex);
    const char *nomLoco = loco->getName();
    int vitesseReelle = loco->getSpeed();

    // Séparation du sens et de la valeur absolue de la vitesse
    // bool estEnMarcheArriere = (vitesseReelle < 0);
    // int vitesseAbsolue = abs(vitesseReelle); // Transforme -40 en 40, et
    // 40 reste 40

    // 2. EN-TÊTE : NOM DE LA LOCO
    m_screenMenu->setFont(u8g2_font_6x12_tf);
    char titreBuffer[22];
    snprintf(titreBuffer, sizeof(titreBuffer), "[ %s ]", nomLoco);
    m_screenMenu->drawStr(0, 10, titreBuffer);
    m_screenMenu->drawHLine(0, 13, 128);

    // 3. AFFICHAGE DU SENS ET DE LA VITESSE (TEXTE)
    m_screenMenu->setFont(u8g2_font_6x10_tf);

    // On affiche le sens de marche à gauche
    m_screenMenu->drawStr(0, 28, (loco->getDirection() == Direction::Reverse) ? "Arr. :" : "Av.  :");

    if (vitesseReelle == -1) {
        m_screenMenu->drawStr(40, 30, "Urgence STOP");
    }

    // On affiche la valeur de la vitesse absolue à droite
    m_screenMenu->setFont(u8g2_font_6x12_tf);
    char vitBuffer[8];
    snprintf(vitBuffer, sizeof(vitBuffer), "%d", vitesseReelle);
    m_screenMenu->drawStr(100, 30, vitBuffer);

    // 4. SÉCURISATION ET DESSIN DE LA JAUGE GRAPHIQUE (Échelle 126)
    m_screenMenu->drawFrame(0, 36, 100, 6);

    // on filtre pour la jauge
    if (vitesseReelle < 0)
        vitesseReelle = 0;
    if (vitesseReelle > 126)
        vitesseReelle = 126;

    // Calcul de la jauge : on "mappe" de 0 à 126 vers les 98 pixels
    // disponibles
    int largeurJauge = map(vitesseReelle, 0, 126, 0, 98);
    m_screenMenu->drawBox(1, 37, largeurJauge, 4);

    // 5. PIED DE PAGE : INSTRUCTIONS POUR L'UTILISATEUR
    m_screenMenu->drawHLine(0, 50, 128);
    m_screenMenu->setFont(u8g2_font_6x10_tf);
    m_screenMenu->drawStr(0, 61, "> CLIC pour STOP & RETOUR");

    m_screenMenu->sendBuffer();
}

void MainMenu::drawStatusMenu()
{
    m_screenMenu->clearBuffer();

    // 1. EN-TÊTE
    m_screenMenu->setFont(u8g2_font_6x12_tf);
    m_screenMenu->drawStr(10, 10, "STATUT SYSTEME");
    m_screenMenu->drawHLine(0, 13, 128); // Ligne sous le titre

    // 2. POLICE POUR LES DONNÉES
    m_screenMenu->setFont(u8g2_font_6x10_tf);

    // Ligne 1 : Signal DCC
    m_screenMenu->drawStr(0, 24, "Signal DCC :");
    m_screenMenu->drawStr(75, 24,
                          "ACTIF"); // Aligné à droite pour plus de clarté

    // Ligne 2 : Courant
    m_screenMenu->drawStr(0, 34, "Courant    :");
    m_screenMenu->drawStr(75, 34, "420 mA");

    // Ligne 3 : Tension
    m_screenMenu->drawStr(0, 44, "Tension    :");
    m_screenMenu->drawStr(75, 44, "14.8 V");

    // Ligne 4 : Compteurs Locos et Aiguillages (combinés sur une ligne ou
    // séparés) Pour que tout entre, on peut formater une ligne avec les
    // deux infos :
    char compteursBuffer[22];
    snprintf(compteursBuffer, sizeof(compteursBuffer), "Locos: %d | Aiguil: %d", m_commandStationClient->getLocomotivesCount(),
             m_commandStationClient->getTurnoutsCount());
    m_screenMenu->drawStr(0, 54, compteursBuffer);

    // 3. PIED DE PAGE (Séparateur + Action)
    m_screenMenu->drawHLine(0, 56, 128);
    m_screenMenu->setFont(u8g2_font_6x10_tf); // Police plus petite pour le bouton retour
    m_screenMenu->drawStr(0, 64, "> CLIC POUR RETOUR");

    m_screenMenu->sendBuffer();
}