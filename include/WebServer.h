#pragma once

#include <Arduino.h>
#include <Network.h>
#include <Appl.h>
#include <ModuleInterface.h>

struct WebServerConfig {
    bool   AutoRedirectMode = false;
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

struct WebServerStatus {
    bool   Started = false;
    bool   AutoRedirectMode = false;
};

/// @brief Class to handle the WebServer and to register the routes
class CWebServer : public AsyncWebServer, public IModule {
    public:
        int nVersion = 1;
        WebServerConfig Config; // Configuration of the webserver
        WebServerStatus Status; // Status information about the webserver
    public:
        CWebServer(int nPortNumber);
        void writeStatusTo(JsonObject &oStatusNode) override;
        void writeConfigTo(JsonObject &oNode, bool bHideCritical)  override; 
        void readConfigFrom(JsonObject &oNode) override;
        int receiveEvent(const void * pSender, int nMsg, const void * pMessage, int nClass) override;
    
        void registerFileAccess();
        void registerDefaults();
};