#ifndef DEBUG_LSC_STATUSHANDLER
    #undef DEBUG_INFOS
#endif

#include <SysStatusLed.h>
#include <Msgs.h>
#include <WiFiController.h>
#include <DevelopmentHelper.h>

CSysStatusLed::CSysStatusLed(int nRedPin, int nGreenPin, int nBluePin, bool bActiveLow) :
    CRGBLed(nRedPin,nGreenPin,nBluePin,bActiveLow) 
    {

    }

int CSysStatusLed::receiveEvent(const void * pSender, int nMsgId, const void * pMessage, int nType) {
    switch(nMsgId) {
        case MSG_REBOOT_REQUEST: isRebootPending = true;   DEBUG_INFO("STATUSLED: Reboot request received...");   break;
        case MSG_BUTTON_ON     : isButtonPressed = true;   DEBUG_INFO("STATUSLED: Button pressed received...");   break;
        case MSG_BUTTON_OFF    : isButtonPressed = false;  DEBUG_INFO("STATUSLED: Button released received..."); break;
        case MSG_WIFI_CONNECTED: 
                isInAccessPointMode = (nType == WIFI_ACCESS_POINT_MODE);
                isInStationMode     = (nType == WIFI_STATION_MODE);
                isWiFiConnected = isInAccessPointMode || isInStationMode;
                break;
        case MSG_WIFI_DISABLED :
                isInAccessPointMode = false;
                isInStationMode = false;
                isWiFiConnected = false;
                break;
    };
    return(EVENT_MSG_RESULT_OK);
}

void CSysStatusLed::updateLED() {
    // Reboot and Button Pressed == Prio 1
    if(isRebootPending) {
        setColor(RGB_COLOR::RED);
    } else if(isButtonPressed) {
        blink(RGB_COLOR::GREEN,100,100);
    }  else {
        // Normal operation...
        int nBlinkOn = 50;
        int nBlinkOff = 10000;
        int nColor = RGB_COLOR::YELLOW;
        if(isInAccessPointMode) {
            nColor = RGB_COLOR::BLUE;
        } else if(isWiFiConnected) {
            nColor = RGB_COLOR::GREEN;
            // Strength of signal to low or not longer available...
            if(!WiFi.isConnected())     { nColor = RGB_COLOR::RED; }
            else if (WiFi.RSSI() < -82) { nColor = RGB_COLOR::YELLOW;}
        }
        blink(nColor,nBlinkOn,nBlinkOff); 
    }
} 
