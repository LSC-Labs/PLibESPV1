#pragma once
/**
 * @file MDNSController.h
 * 
 * @brief MDNS Controller class definition
 * ESP MDNS controller to handle mDNS responder start/stop on WiFi events
 * 
 * 
 */

#ifdef ESP32
    #include <ESPmDNS.h>
#else
    #include <ESP8266mDNS.h>
#endif

#include <EventHandler.h>
#include <Appl.h>

/**
 * @brief Starts/stops the mDNS responder (<hostname>.local) on WiFi events.
 *
 * MSG_WIFI_CONNECTED (station or access point) (re)starts the responder,
 * MSG_WIFI_STARTING / MSG_WIFI_DISABLING stop it. The HTTP service (port 80)
 * is advertised. Invalid hostname characters are replaced by '-'.
 */
class CMDNSController : public IMsgEventReceiver {
    private:
        bool m_bRunning = false;    // responder started by this controller

    public:
        CMDNSController();
        CMDNSController(const char *pszAutoregisterName);
        ~CMDNSController();

        bool begin(const char* pszHostname = nullptr);
        void end();
        /// @brief true if the responder was started successfully.
        bool isRunning() const { return(m_bRunning); }
        /// @brief Convert a device name into a valid mDNS host label (a-z, 0-9, '-', max. 63 chars).
        static String toHostLabel(const String &strName);

        int receiveEvent(const void * pSender, int nMsg, const void * pMessage, int nClass) override;
};