#ifndef DEBUG_LSC_STATUSHANDLER
    #undef DEBUG_INFOS
#endif

#include <SysStatusLed.h>
#include <Msgs.h>
#include <WiFiController.h>
#include <DevelopmentHelper.h>

/**
 * @brief Creates a system status RGB LED.
 * @param nRedPin Red channel GPIO pin.
 * @param nGreenPin Green channel GPIO pin.
 * @param nBluePin Blue channel GPIO pin.
 * @param bActiveLow true when the LED channels are active-low.
 */
CSysStatusLed::CSysStatusLed(int nRedPin, int nGreenPin, int nBluePin, bool bActiveLow) :
    CRGBLed(nRedPin,nGreenPin,nBluePin,bActiveLow) 
    {

    }

/**
 * @brief Receives application events and updates the internal status flags.
 *
 * The actual LED pattern is refreshed on MSG_APPL_LOOP by updateLED(); event
 * handlers here only remember the newest application, button and WiFi states.
 *
 * @return EVENT_MSG_RESULT_OK after every handled or ignored message.
 */
int CSysStatusLed::receiveEvent(const void * pSender, int nMsgId, const void * pMessage, int nType) {
    switch(nMsgId) {
        case MSG_APPL_LOOP     :  updateLED();                                 
                                  break;
        case MSG_APPL_SHUTDOWN  :
        case MSG_REBOOT_REQUEST : setColor(RGB_COLOR::RED);
                                  isRebootRequested     = true;   
                                  DEBUG_INFO("STATUSLED: Reboot requested...");   
                                  break;
        case MSG_BUTTON_ON      : isButtonPressed        = true;   DEBUG_INFO("STATUSLED: Button pressed received...");   break;
        case MSG_BUTTON_OFF     : isButtonPressed        = false;  DEBUG_INFO("STATUSLED: Button released received..."); break;
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

/**
 * @brief Applies the current status flags to the RGB LED.
 *
 * Priority order is: reboot request, button pressed, then normal WiFi status.
 * In station mode the signal strength can downgrade green to yellow; a lost
 * connection is shown as red.
 */
void CSysStatusLed::updateLED() {
    // Reboot and Button Pressed == Prio 1
    if(isRebootRequested) {
        setColor(RGB_COLOR::RED);
    } else if(isButtonPressed) {
        blink(RGB_COLOR::YELLOW,100,100);
    }  else {
        // Normal operation...
        int nBlinkOn = getNormalBlinkOnTime();
        int nBlinkOff = getNormalBlinkOffTime();
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
