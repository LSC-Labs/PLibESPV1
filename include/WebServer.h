#pragma once

#include <Arduino.h>
#include <Network.h>
#include <Appl.h>

struct WebServerConfig {
    String UserName = "admin";
    // String Passwd   = "admin";

    /// @brief Function to authenticate against the UserName and Passwd of this config
    /// @param pRequest 
    /// @param pszReason 
    /// @param bLogFailed 
    /// @return 
    bool authenticate(AsyncWebServerRequest *pRequest, const char* pszReason, bool bLogFailed) {
        bool bAuthenticated = pRequest->authenticate(UserName.c_str(),Appl.getDevicePwd().c_str());
        if(!bAuthenticated && bLogFailed) {
            IPAddress oRemoteIP = pRequest->client()->remoteIP();
            ApplLogWarnWithParms(F("unauthorized web access (%s): %s"),
                                    oRemoteIP.toString().c_str(),
                                    pszReason ? pszReason : "-");
        }
        return(bAuthenticated);
    }
};

/// @brief Class to handle the WebServer and to register the routes
class CWebServer : public AsyncWebServer, public IConfigHandler {
    public:
        int nVersion = 1;
        WebServerConfig Config;
    public:
        CWebServer(int nPortNumber);
        void writeConfigTo(JsonObject &oNode, bool bHideCritical)  override; 
        void readConfigFrom(JsonObject &oNode) override;
    
        void registerFileAccess();
        void registerDefaults();
};