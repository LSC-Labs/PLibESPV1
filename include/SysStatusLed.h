#pragma once

#include <RGBLed.h>
#include <EventHandler.h>
#include <SimpleDelay.h>

#ifndef STATUSLED_WEAK_RSSI
    #define STATUSLED_WEAK_RSSI  -82        ///< below this RSSI (dBm) the station LED is yellow
#endif
#ifndef STATUSLED_OTA_TIMEOUT
    #define STATUSLED_OTA_TIMEOUT  15000    ///< ms without OTA event => firmware update no longer shown
#endif

/**
 * @brief RGB LED that shows the system state (register it on the message bus).
 *
 * Priority: reboot (red) > firmware update (fast yellow) > button pressed (fast yellow) > WiFi state:
 *  - access point: blue
 *  - station connected: green, weak signal: yellow, connection lost: red
 *  - no WiFi / connecting: yellow
 */
class CSysStatusLed : public CRGBLed, public IMsgEventReceiver {

    bool isRebootRequested = false;
    bool isButtonPressed = false;
    bool isInAccessPointMode = false;
    bool isInStationMode = false;
    bool isWiFiConnected = false;
    bool isOtaRunning = false;

    // Firmware update: no OTA event for this time => update is treated as ended (i.e. client disconnect)
    CSimpleDelay  m_oOtaTimeout   = CSimpleDelay(STATUSLED_OTA_TIMEOUT);

    // Current blink pattern - a change restarts all channels in sync
    int           m_nBlinkColor   = -1;
    unsigned long m_ulBlinkOn     = 0;
    unsigned long m_ulBlinkOff    = 0;
    // Station signal state (RSSI is read once per second, not in every loop)
    CSimpleDelay  m_oSignalCheck  = CSimpleDelay(1000);
    int           m_nStationColor = RGB_COLOR::GREEN;

    /// @brief Blink with the pattern - restarts all channels if the pattern changed.
    void showBlink(int nColor, unsigned long ulOnMillis, unsigned long ulOffMillis);

    public:
        /// @param bActiveLow true (default): common cathode (ON = HIGH), false: common anode (ON = LOW)
        ///        - name inverted to the behavior, see CRGBLed.
        CSysStatusLed(int nRedPin, int nGreenPin, int nBluePin, bool bActiveLow = true );

        virtual int receiveEvent(const void * pSender, int nMsgId, const void * pMessage, int nType);
        virtual void updateLED();
        virtual unsigned long getNormalBlinkOnTime() { return 50; };
        virtual unsigned long getNormalBlinkOffTime() { return 10000; };
};
