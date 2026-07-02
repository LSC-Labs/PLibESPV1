#pragma once

#include <Arduino.h>
#include <Appl.h>
#include <Network.h>
#include <WebSocket.h>

/// @brief Runtime configuration of the web server module.
struct WebServerConfig {
    /// @brief true to redirect unknown/default traffic automatically.
    bool   AutoRedirectMode = false;
    /// @brief HTTP basic auth user name; password is read from Appl.
    String UserName = "admin";
    // String Passwd   = "admin";

    /// @brief Authenticate a request against configured user and application password.
    /// @param pRequest Request to authenticate.
    /// @param pszReason Optional reason used for failed-auth logging.
    /// @param bLogFailed true to write failed attempts to the application log.
    /// @return true if the request has valid credentials.
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

/// @brief Status of the web server module.
struct WebServerStatus {
    /// @brief true after the server has been started.
    bool   Started = false;
    /// @brief Last active auto-redirect mode.
    bool   AutoRedirectMode = false;
};

/**
 * @brief Async web server module that registers default routes and file access.
 */
class CWebServer : public AsyncWebServer, public IModule {
    public:
        /// @brief Module version exposed for diagnostics.
        const int Version = 1;
        /// @brief Configuration of the webserver.
        WebServerConfig Config;
        /// @brief Status information about the webserver.
        WebServerStatus Status;
    protected:
     

    public:
        /// @brief Create a web server on the given TCP port.
        CWebServer(int nPortNumber = 80);

        /// @brief Write web server status into a JSON node.
        void writeStatusTo(JsonNode &oStatusNode, int nLevel = STATUS_LEVEL_INFO) override;
        /// @brief Write web server configuration into a JSON node.
        void writeConfigTo(JsonNode &oNode, bool bHideCritical)  override; 
        /// @brief Read web server configuration from a JSON node.
        void readConfigFrom(JsonNode &oNode) override;
        /// @brief React to application lifecycle/network events.
        int receiveEvent(const void * pSender, int nMsg, const void * pMessage, int nClass) override;
    
        /// @brief Register routes used to serve static files from the file system.
        void registerFileAccess();
        /// @brief Deliver a requested file or a fallback response.
        void deliverFile(AsyncWebServerRequest *pRequest);
        /// @brief Register built-in routes such as status/config endpoints.
        void registerDefaults();
        /// @brief Add/update authentication headers on a response.
        virtual void setNewAuthHeader(AsyncWebServerRequest *pRequest, AsyncWebServerResponse *pResponse);
};
