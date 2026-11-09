#pragma once

#include <Arduino.h>
#include <Appl.h>
#include <Network.h>
#include <FS.h>
#include <WebSocket.h>

/// @brief Page the captive portal redirects to (start page of the portal).
#ifndef WEBSERVER_CAPTIVE_START_PAGE
    #define WEBSERVER_CAPTIVE_START_PAGE "/"
#endif

/// @brief Runtime configuration of the web server module.
struct WebServerConfig {
    /// @brief true: while the captive portal is active, requests to unknown pages
    ///        are redirected to the captive start page (WEBSERVER_CAPTIVE_START_PAGE).
    ///        Without an active captive portal unknown pages always return 404.
    bool   AutoRedirectMode = true;
    /// @brief HTTP basic auth user name; password is read from Appl.
    String UserName = "admin";
    // String Passwd   = "admin";

    /// @brief Authenticate a request against configured user and application password,
    ///        or by the access token (header "AUTHTOKEN") returned by /login.
    /// @param pRequest Request to authenticate.
    /// @param pszReason Optional reason used for failed-auth logging.
    /// @param bLogFailed true to write failed attempts to the application log.
    /// @return true if the request has valid credentials.
    bool authenticate(AsyncWebServerRequest *pRequest, const char* pszReason, bool bLogFailed);
};

/// @brief Status of the web server module.
struct WebServerStatus {
    /// @brief true after the server has been started.
    bool   Started = false;
    /// @brief true while the captive portal is active (MSG_CAPTIVE_PORTAL_STARTED/STOPPED).
    bool   CaptivePortalActive = false;
    /// @brief true if unknown pages are currently redirected
    ///        (Config.AutoRedirectMode && CaptivePortalActive).
    bool   AutoRedirectMode = false;
};

/**
 * @brief Async web server module that registers default routes and file access.
 */
class CWebServer : public AsyncWebServer, public IModule {
    private:
        /// @brief State of the running file upload (one at a time).
        struct UploadState {
            AsyncWebServerRequest *pOwner = nullptr;   // request that owns the upload
            File                   oFile;
            bool                   bError = false;
        } m_oUpload;

        AsyncWebServerRequest *m_pOtaOwner      = nullptr;  // request that owns the firmware update
        bool                   m_bOtaError      = false;
        size_t                 m_nOtaProgressSent = 0;
        volatile bool          m_bRebootAfterOta = false;   // executed in MSG_APPL_LOOP

        /// @brief Recalculate Status.AutoRedirectMode from config and captive portal state.
        void updateRedirectMode();

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
        /// @brief Start the server (sets Status.Started). Multiple calls are ignored.
        /// Is called automatically on MSG_WIFI_CONNECTED, if not started before.
        void begin();
        /// @brief Stop the server (resets Status.Started).
        void end();
        /// @brief true for a plain file name (no path, no "..", max. 31 chars).
        static bool isValidFileName(const String &strName);

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
