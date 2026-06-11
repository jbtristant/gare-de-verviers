#pragma once

#include <Arduino.h>
#include <semphr.h>

#include "turnout.h"

class CommandStationClient;
class U8G2_SSD1306_128X64_NONAME_1_HW_I2C;

class MainMenu
{
  public:
    MainMenu();

    void initialize(CommandStationClient *client,
                    U8G2_SSD1306_128X64_NONAME_1_HW_I2C *screenMenu,
                    Stream *logStream, SemaphoreHandle_t xLogStreamSemaphore);

    void buttonPress();
    void menuUp();
    void menuDown();

    void selectLocomotive();

    void onTrackChanged();
    void onLocomotiveChanged();
    void onTurnoutStateChanged(uint16_t id, TurnoutState state);

  private:
    enum AppContext : uint8_t {
        MAIN_MENU,
        TRACK_MENU,
        LOCO_MENU,
        DRIVING_MODE,
        TURNOUT_MENU,
        TURNOUT_MODE,
        STATUS_MENU,
        CONFIGURATION_MENU
    };

    void handlePress();
    void writeConsoleLogLine();
    void writeConsoleLogReturn(uint8_t size);

    static void TaskDrawUIStatic(void *pvParameters);
    void drawUI();
    void taskDrawUI();

    void drawMainMenu();
    void drawTrackMenu();
    void drawLocomotiveMenu();
    void drawTurnoutMenu();
    void drawDrivingMode();
    void drawStatusMenu();

    const uint8_t m_mainMenuSize = 5;
    const uint8_t m_screenMenuMaxVisibleLines = 4;
    unsigned long m_lastMenuDisplay = 0;

    const int8_t m_locomotiveSpeedStep = 4;

    CommandStationClient *m_commandStationClient;
    U8G2_SSD1306_128X64_NONAME_1_HW_I2C *m_screenMenu;
    Stream *m_logStream;
    SemaphoreHandle_t m_xLogStreamSemaphore;

    SemaphoreHandle_t m_xDisplaySemaphore;
    TaskHandle_t m_xDisplayTaskHandle;

    AppContext m_currentContext;
    uint8_t m_menuIndex;
};