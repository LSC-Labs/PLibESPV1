#ifndef DEBUG_LSC_STATUSLED
    #undef DEBUGINFOS
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
 * @param bActiveLow true (default): common cathode (ON = HIGH), false: common
 *        anode (ON = LOW) - name inverted to the behavior, see CRGBLed.
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
    (void) pSender; (void) pMessage;
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
                m_oSignalCheck.setExpired();        // check the signal immediately
                break;
        // (Re)connecting or connection failed - not connected until MSG_WIFI_CONNECTED
        case MSG_WIFI_STARTING :
        case MSG_WIFI_ERROR    :
        case MSG_WIFI_DISABLED :
                isInAccessPointMode = false;
                isInStationMode = false;
                isWiFiConnected = false;
                break;
    };
    return(EVENT_MSG_RESULT_OK);
}

/**
 * @brief Blinks with the given pattern; a changed pattern restarts all channels.
 *
 * Every color channel has its own blink timer. Without the restart, mixed
 * colors (yellow = red + green) would blink out of phase after a change.
 */
void CSysStatusLed::showBlink(int nColor, unsigned long ulOnMillis, unsigned long ulOffMillis) {
    if(nColor != m_nBlinkColor || ulOnMillis != m_ulBlinkOn || ulOffMillis != m_ulBlinkOff) {
        m_nBlinkColor = nColor;
        m_ulBlinkOn   = ulOnMillis;
        m_ulBlinkOff  = ulOffMillis;
        restartBlink();
    }
    blink(nColor, ulOnMillis, ulOffMillis);
}

/**
 * @brief Applies the current status flags to the RGB LED (called on MSG_APPL_LOOP).
 *
 * Priority order is: reboot request, button pressed, then normal WiFi status.
 * In station mode the signal strength can downgrade green to yellow; a lost
 * connection is shown as red. The signal is checked once per second.
 */
void CSysStatusLed::updateLED() {
    // Reboot and Button Pressed == Prio 1
    if(isRebootRequested) {
        m_nBlinkColor = -1;                 // next blink pattern starts fresh
        setColor(RGB_COLOR::RED);
    } else if(isButtonPressed) {
        showBlink(RGB_COLOR::YELLOW,100,100);
    }  else {
        // Normal operation...
        int nColor = RGB_COLOR::YELLOW;     // no WiFi / connecting
        if(isInAccessPointMode) {
            nColor = RGB_COLOR::BLUE;
        } else if(isWiFiConnected) {
            if(m_oSignalCheck.isDone()) {
                m_oSignalCheck.restart();
                // Strength of signal to low or not longer available...
                if(!WiFi.isConnected())                         m_nStationColor = RGB_COLOR::RED;
                else if (WiFi.RSSI() < STATUSLED_WEAK_RSSI)     m_nStationColor = RGB_COLOR::YELLOW;
                else                                            m_nStationColor = RGB_COLOR::GREEN;
            }
            nColor = m_nStationColor;
        }
        showBlink(nColor,getNormalBlinkOnTime(),getNormalBlinkOffTime()); 
    }
} 
