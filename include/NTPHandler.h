#pragma once
#include <Arduino.h>
#include <time.h>
#include <Appl.h>
/**
 * NTP Service for ESP Devices
 * Setup when a internet connection (WiFi) becomes available...
 * @see: https://randomnerdtutorials.com/esp32-date-time-ntp-client-server-arduino/
 * @see: https://forum.arduino.cc/t/getting-time-from-ntp-service-using-nodemcu-1-0-esp-12e/702333/12
 * 
 */

 struct NTPConfig {
    bool isEnabled = true;
    String NTPServer = "pool.ntp.org";
    // String TZ = "Europe/Berlin"; // Alternat is : CET-1CEST,M3.5.0/02,M10.5.0/03
    String TZ = "CET-1CEST,M3.5.0/02,M10.5.0/03";
 };

 class CNTPHandler : public IModule {
    protected:
        NTPConfig Config;
        time_t  m_oRawTime;
        bool    m_bIsInitialized = false;
        unsigned long m_uLastUpdate = 0;
        char m_szISODateTime[40];

    public:
        CNTPHandler(const char * pszAutoRegisterName = nullptr) {
            if(pszAutoRegisterName) Appl.registerModule(pszAutoRegisterName ,this);  
        };
        void setup();
        void timeUpdatedByService();
        bool hasValidTime();
        time_t getNativeTime();
        const char * getISODateTime();
        // WiFiStatus getStatus() { return Status; }    
        void writeConfigTo( JsonObject &oCfgObj, bool bHideCritical) override;
        void readConfigFrom(JsonObject &oCfgObj) override;
        void writeStatusTo( JsonObject &oStatusObj) override;
        int receiveEvent(const void * pSender, int nMsgType, const void * pMessage, int nClass) override;
 };