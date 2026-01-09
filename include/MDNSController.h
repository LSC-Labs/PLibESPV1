#pragma once
/**
 * @file MDNSController.h
 * 
 * @brief MDNS Controller class definition
 * ESP MDNS controller to handle mDNS responder start/stop on WiFi events
 * 
 * 
 */

#ifdef CONFIG_IDF_TARGET_ESP32
    #include <ESPmDNS.h>
#else
    #include <ESP8266mDNS.h>
#endif

#include <EventHandler.h>
#include <Appl.h>

class CMDNSHandler : public IMsgEventReceiver{
    public:
        CMDNSHandler();
        ~CMDNSHandler();

        bool begin(const char* pszHostname = nullptr);
        void end();

        int receiveEvent(const void * pSender, int nMsg, const void * pMessage, int nClass) override;
};