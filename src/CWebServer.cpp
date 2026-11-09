#ifndef DEBUG_LSC_WEBSERVER
    #undef DEBUGINFOS
#endif
#include <WebServer.h>
#include <Security.h>
#include <AccessToken.h>
#include <FileSystem.h>
#include <DevelopmentHelper.h>
#ifdef ESP32
    #include <Update.h>
    #define OTA_ERROR_STRING()  String(Update.errorString())
    #define OTA_MAX_SIZE        UPDATE_SIZE_UNKNOWN
    #define OTA_ABORT()         Update.abort()
#else
    #define OTA_ERROR_STRING()  Update.getErrorString()
    #define OTA_MAX_SIZE        ((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000)
    #define OTA_ABORT()         Update.end(false)
#endif

/*
 * Threading note:
 * Request and upload handlers run in the async TCP context (AsyncTCP task on
 * ESP32, SYS context on ESP8266). Never block there (no delay(), no reboot).
 * Events are posted (queued) and delivered in the loop. Work that must run in
 * the loop (reboot after OTA) is flagged and executed in MSG_APPL_LOOP.
 */

#define WEBSERVER_FILES_GET_PREFIX      "/files/get/"
#define WEBSERVER_OTA_PROGRESS_STEP     32768       // send MSG_OTA_PROGRESS every n bytes

const char * WEBSERVER_AUTOREDIRECT_MODE       = "autoRedirectMode";

#pragma region Constructor
/**
 * @brief Creates the async web server on the selected port.
 *
 * The instance still has to be registered with the application/module system by
 * the caller (needed for MSG_APPL_LOOP and the captive portal event).
 *
 * @param nPortNumber TCP port to listen on.
 */
CWebServer::CWebServer(int nPortNumber)
	: AsyncWebServer(nPortNumber) {}

/**
 * @brief Starts the server and marks it as started in the status.
 *
 * Multiple calls are ignored. Called automatically on MSG_WIFI_CONNECTED,
 * so the server is always started through this function, even if the
 * application does not call it.
 */
void CWebServer::begin() {
    if(Status.Started) return;
    AsyncWebServer::begin();
    Status.Started = true;
    ApplLogInfo(F("WEB: server started"));
}

/**
 * @brief Stops the server and resets the started flag.
 */
void CWebServer::end() {
    if(!Status.Started) return;
    AsyncWebServer::end();
    Status.Started = false;
}

/**
 * @brief Redirect only if configured and the captive portal is active.
 */
void CWebServer::updateRedirectMode() {
    Status.AutoRedirectMode = Config.AutoRedirectMode && Status.CaptivePortalActive;
}

#pragma endregion

#pragma region Module (Config / Status / MessageBus) Interface implementation

/**
 * @brief Reads web server configuration from JSON.
 * @param oNode Source configuration node.
 */
void CWebServer::readConfigFrom(JsonNode &oNode) {
    oNode.storeValueIf(WEBSERVER_AUTOREDIRECT_MODE,&Config.AutoRedirectMode);
    updateRedirectMode();
}

/**
 * @brief Writes web server configuration to JSON.
 * @param oCfgNode Target configuration node.
 * @param bHideCritical Unused here; no web server value is critical.
 */
void CWebServer::writeConfigTo(JsonNode &oCfgNode,bool bHideCritical) {
    (void) bHideCritical;
    oCfgNode[WEBSERVER_AUTOREDIRECT_MODE] = Config.AutoRedirectMode;
}

/**
 * @brief Writes web server runtime status to JSON.
 * @param oStatusNode Target status node.
 * @param nLevel Status verbosity, currently unused.
 */
void CWebServer::writeStatusTo(JsonNode &oStatusNode, int nLevel) {
    (void) nLevel;
    oStatusNode["started"] = Status.Started;
    oStatusNode["captivePortal"] = Status.CaptivePortalActive;
    oStatusNode[WEBSERVER_AUTOREDIRECT_MODE] = Status.AutoRedirectMode;
}

/**
 * @brief Receives application events that affect web server state.
 *
 * - MSG_WIFI_CONNECTED starts the server (if not started yet).
 * - MSG_CAPTIVE_PORTAL_STARTED/STOPPED switch the redirect of unknown pages.
 * - MSG_APPL_LOOP executes the reboot after a successful firmware update in the
 *   loop context (the upload handler runs in the async context and must not block).
 *
 * @return EVENT_MSG_RESULT_OK after processing.
 */
int CWebServer::receiveEvent(const void * pSender, int nMsg, const void * pMessage, int nClass) {
    (void) pSender; (void) pMessage; (void) nClass;
    int nResult = EVENT_MSG_RESULT_OK;
    switch(nMsg) {
        case MSG_WIFI_CONNECTED: {
            begin();
            break;
        }
        case MSG_CAPTIVE_PORTAL_STARTED: {
            Status.CaptivePortalActive = true;
            updateRedirectMode();
            DEBUG_INFOS("WEB: captive portal started - redirect %s",Status.AutoRedirectMode ? "enabled" : "disabled by config");
            break;
        }
        case MSG_CAPTIVE_PORTAL_STOPPED: {
            Status.CaptivePortalActive = false;
            updateRedirectMode();
            DEBUG_INFO("WEB: captive portal stopped - redirect disabled");
            break;
        }
        case MSG_APPL_LOOP: {
            if(m_bRebootAfterOta) {
                m_bRebootAfterOta = false;
                Appl.MsgBus.sendEvent(nullptr,MSG_REBOOT_REQUEST,"Firmware update",0);
            }
            break;
        }
    }
    return(nResult);
}
#pragma endregion

#pragma region Securiy
/**
 * @brief Adds a freshly generated authentication token to a response.
 *
 * The token is bound to the remote client IP address and is returned in the
 * AUTHTOKEN header after successful authentication.
 */
bool WebServerConfig::authenticate(AsyncWebServerRequest *pRequest, const char* pszReason, bool bLogFailed) {
    if(!pRequest) return(false);
    bool bAuthenticated = pRequest->authenticate(UserName.c_str(),Appl.getDevicePwd().c_str());
    // Access token of the web frontend (from /login) - bound to the client IP address.
    // Used by fetch() requests (i.e. firmware upload), which have no basic auth credentials.
    if(!bAuthenticated && pRequest->client() && pRequest->hasHeader("AUTHTOKEN")) {
        const AsyncWebHeader *pHeader = pRequest->getHeader("AUTHTOKEN");
        if(pHeader && pHeader->value().length() > 10) {
            String strIPAddress = pRequest->client()->remoteIP().toString();
            CAccessToken oToken(pHeader->value().c_str());
            bAuthenticated = oToken.isAuthValid(strIPAddress.c_str(),APPL_SECURITY_TOKEN_KEY);
        }
    }
    if(!bAuthenticated && bLogFailed && pRequest->client()) {
        IPAddress oRemoteIP = pRequest->client()->remoteIP();
        ApplLogWarnWithParms(F("unauthorized web access (%s): %s"),
                                oRemoteIP.toString().c_str(),
                                pszReason ? pszReason : "-");
    }
    return(bAuthenticated);
}

void CWebServer::setNewAuthHeader(AsyncWebServerRequest *pRequest, AsyncWebServerResponse *pResponse){
    String strIPAddress;
    if(pRequest && pRequest->client()) strIPAddress = pRequest->client()->remoteIP().toString();
    if(pResponse) {
        CAccessToken oToken(strIPAddress.c_str(),APPL_SECURITY_TOKEN_KEY);
        pResponse->addHeader("AUTHTOKEN",oToken.getTokenAsBase64());
    }
}

/**
 * @brief Checks a plain file name for use in the root of the file system.
 *
 * Only a simple name is allowed - no path separators, no "..", no control
 * characters - so requests cannot leave the root directory.
 */
bool CWebServer::isValidFileName(const String &strName) {
    if(strName.length() == 0 || strName.length() > 31) return(false);   // LittleFS name limit
    if(strName.indexOf("..") > -1) return(false);
    for(size_t nIdx = 0; nIdx < strName.length(); nIdx++) {
        char c = strName[nIdx];
        if(c == '/' || c == '\\' || (unsigned char) c < 0x20) return(false);
    }
    return(true);
}

#pragma endregion

#pragma region Registered File Access Routes

/**
 * @brief Delivers a file of the file system root (/files/get/<filename>).
 *
 * Requires authentication. The content type is derived from the file
 * extension, the file is sent as download.
 */
void CWebServer::deliverFile(AsyncWebServerRequest *pRequest) {
    DEBUG_FUNC_START_PARMS("%s",pRequest->url().c_str());
    if (!Config.authenticate(pRequest,"file",true)) {
        return pRequest->requestAuthentication();
    }
    String strName = pRequest->url().substring(strlen(WEBSERVER_FILES_GET_PREFIX));
    if(!isValidFileName(strName)) {
        pRequest->send(400, "text/plain", "Invalid file name");
        return;
    }
    String strPath = "/" + strName;
    CFS oFS;
    FS  oBaseFS = oFS.getBaseFS();
    if(!oBaseFS.exists(strPath)) {
        pRequest->send(404, "text/plain", "File not found");
        return;
    }
    AsyncWebServerResponse *pResponse = pRequest->beginResponse(oBaseFS, strPath, String(), true);
    setNewAuthHeader(pRequest,pResponse);
    pRequest->send(pResponse);
    DEBUG_FUNC_END();
}

/**
 * @brief Registers authenticated file management routes.
 *
 * Routes:
 * - /files/list lists LittleFS files.
 * - /files/get/<filename> delivers a file (one prefix route for all files,
 *   so uploaded files are available without registering new routes).
 * - /files/upload serves and handles a simple upload form.
 */
void CWebServer::registerFileAccess() {

	// Upload form
    on("/files/upload", HTTP_GET, [this](AsyncWebServerRequest *pRequest) {
        if (!Config.authenticate(pRequest,"files/upload",true)) {
            return pRequest->requestAuthentication();
		}
        pRequest->send(200, "text/html", "<form method='POST' action='/files/upload' enctype='multipart/form-data'>"
                                        "<input type='file' name='file'>"
                                        "<input type='submit' value='Upload'>"
                                        "</form>");
    });

	// Upload handler - called after all data was received
    on("/files/upload", HTTP_POST,
        [this](AsyncWebServerRequest *pRequest) {
            if (!Config.authenticate(pRequest,"files/upload",true)) {
                return pRequest->requestAuthentication();
		    }
            if(m_oUpload.pOwner != pRequest) {
                pRequest->send(m_oUpload.pOwner ? 409 : 400, "text/plain",
                               m_oUpload.pOwner ? "Another upload is running" : "No file received");
                return;
            }
            bool bOk = !m_oUpload.bError;
            m_oUpload.pOwner = nullptr;
            pRequest->send(bOk ? 200 : 500, "text/plain", bOk ? "OK" : "Upload failed");
        },
        // Upload data - called for each received chunk
        [this](AsyncWebServerRequest *pRequest, const String& strFilename, size_t index, uint8_t *pData, size_t nLen, bool bFinal) {
            if (index == 0) {
                // Check before any data is written (the request handler runs after the upload)
                if (!Config.authenticate(pRequest,"files/upload",true)) return;
                if (m_oUpload.pOwner && m_oUpload.pOwner != pRequest) return;    // another upload is running
                m_oUpload.pOwner = pRequest;
                m_oUpload.bError = !isValidFileName(strFilename);
                if (m_oUpload.bError) {
                    ApplLogErrorWithParms(F("WEB: invalid upload file name: %s"), strFilename.c_str());
                    return;
                }
                ApplLogInfoWithParms(F("Saving File: %s"), strFilename.c_str());
                CFS oFS;
                m_oUpload.oFile = oFS.getBaseFS().open("/" + strFilename, "w");
                m_oUpload.bError = !m_oUpload.oFile;
                // Client disconnects during the upload => release the slot and close the file
                pRequest->onDisconnect([this, pRequest]() {
                    if (m_oUpload.pOwner == pRequest) {
                        if (m_oUpload.oFile) m_oUpload.oFile.close();
                        m_oUpload.pOwner = nullptr;
                    }
                });
            }
            if (m_oUpload.pOwner != pRequest || m_oUpload.bError) return;
            if (nLen > 0 && m_oUpload.oFile.write(pData, nLen) != nLen) {
                ApplLogErrorWithParms(F("WEB: writing %s failed (file system full?)"), strFilename.c_str());
                m_oUpload.bError = true;
            }
            if (bFinal || m_oUpload.bError) {
                m_oUpload.oFile.close();
                if (!m_oUpload.bError) ApplLogInfoWithParms(F("Upload finished: %s"), strFilename.c_str());
            }
        }
    );

	// Show list of files
    on("/files/list", HTTP_GET, [this](AsyncWebServerRequest *pRequest) {
        if (!Config.authenticate(pRequest,"files/list",true)) {
            return pRequest->requestAuthentication();
		}
        CFS oFS;
        String strFileList = oFS.getFileList("/");
        pRequest->send(200, "application/json", strFileList);
    });

    // Download of files: one prefix route for all (existing and later uploaded) files
    on(WEBSERVER_FILES_GET_PREFIX "*", HTTP_GET, [this](AsyncWebServerRequest *pRequest) {
        deliverFile(pRequest);
    });
}

#pragma endregion

#pragma region Registered Default Routes (login, status, ota, notfound)
/**
 * @brief Registers default runtime routes.
 *
 * The defaults include root rewrite, captive-portal-aware not-found handling,
 * login/auth-token refresh, status JSON and firmware OTA update.
 */
void CWebServer::registerDefaults() {
    DEBUG_FUNC_START();
    rewrite("/", "/index.html");

    /**
     * Not found handler
     * - Captive portal active and Config.AutoRedirectMode set:
     *   every unknown URL is redirected to the captive start page. This also
     *   covers the portal detection URLs of the clients (/generate_204,
     *   /hotspot-detect.html, /connecttest.txt, ...).
     * - Otherwise: 404 error.
     */
    onNotFound([this](AsyncWebServerRequest *pRequest) {
        DEBUG_INFOS("WEB: Not found: %s (redirect=%d)",pRequest->url().c_str(),Status.AutoRedirectMode);
        if(Status.AutoRedirectMode) {
            // Absolute URL - captive portal checks of some clients ignore relative redirects
            String strLocation = "http://";
            strLocation += pRequest->client() ? pRequest->client()->localIP().toString() : String("192.168.4.1");
            strLocation += WEBSERVER_CAPTIVE_START_PAGE;
            AsyncWebServerResponse *pResponse = pRequest->beginResponse(302, "text/plain", "Redirecting to captive portal");
            pResponse->addHeader("Location", strLocation);
            pResponse->addHeader("Cache-Control", "no-cache");
            pRequest->send(pResponse);
        } else {
            ApplLogErrorWithParms(F("WEB: 404 Not found: %s"),pRequest->url().c_str());
            pRequest->send(404, "text/plain", "Not found");
        }
	});

    on("/login", HTTP_GET, [this](AsyncWebServerRequest *pRequest) {
        DEBUG_INFO("WEB:/login (GET)");
        if (!Config.authenticate(pRequest,"login",true)) {
            return pRequest->requestAuthentication();
		}
        AsyncWebServerResponse *pResponse = pRequest->beginResponse(200, "text/plain", "Success");
        setNewAuthHeader(pRequest,pResponse);
        ApplLogInfoWithParms(F("Login from address %s"),pRequest->client()->remoteIP().toString().c_str());
		pRequest->send(pResponse);
	});

    on("/status", HTTP_GET, [](AsyncWebServerRequest *pRequest) {
        DEBUG_INFO("WEB:/status (GET)");
        // Async context: never build the status here (shared buffers, module state of
        // the loop) - use the snapshot built in the loop (copy, safe from any task)
        pRequest->send(200, "application/json", Appl.getStatusSnapshot());
    });

    on("/update", HTTP_POST,
        // Called after the upload - report the real result
        [this](AsyncWebServerRequest *pRequest) {
            DEBUG_INFO("WEB:/update (POST) - result");
            if (!Config.authenticate(pRequest,"update",false)) {
                // Plain 401 (no WWW-Authenticate) - the frontend shows the error,
                // a browser login popup would not resend the firmware anyway.
                return pRequest->send(401, "text/plain", "Unauthorized - please sign in again");
            }
            bool bOk = (m_pOtaOwner == pRequest) && !m_bOtaError && !Update.hasError();
            String strResult = bOk ? String("OK") : (m_pOtaOwner == pRequest ? OTA_ERROR_STRING() : String("Another update is running"));
            if (m_pOtaOwner == pRequest) m_pOtaOwner = nullptr;
            AsyncWebServerResponse * pResponse = pRequest->beginResponse(bOk ? 200 : 500, "text/plain", strResult);
            pResponse->addHeader("Connection", "close");
            pRequest->send(pResponse);
            // Reboot is executed in the loop (MSG_APPL_LOOP), after the response was sent
            if (bOk) m_bRebootAfterOta = true;
        },
        // Called for each received chunk
        [this](AsyncWebServerRequest *pRequest, String strFilename, size_t index, uint8_t *data, size_t len, bool final) {
            if (!index) {
                DEBUG_INFO("WEB:/update (POST) - start");
                // Check before anything is written to flash
                if (!Config.authenticate(pRequest,"update",true)) return;
                if (m_pOtaOwner && m_pOtaOwner != pRequest) return;      // another update is running
                m_pOtaOwner = pRequest;
                m_bOtaError = false;
                m_nOtaProgressSent = 0;
                ApplLogInfoWithParms(F("Firmware update started : %s"),strFilename.c_str());
                Appl.MsgBus.postEventText(this,MSG_OTA_START,strFilename.c_str(),0);
                if (Update.isRunning()) OTA_ABORT();         // leftover of an interrupted update
                #ifndef ESP32
                    Update.runAsync(true);
                #endif
                if (!Update.begin(OTA_MAX_SIZE)) {
                    m_bOtaError = true;
                    ApplLogErrorWithParms(F("Firmware update: begin failed (%s)"),OTA_ERROR_STRING().c_str());
                    Appl.MsgBus.postEventText(this,MSG_OTA_ERROR,"Not enough space",0);
                }
                // Client disconnects during the update => abort and release the slot
                pRequest->onDisconnect([this, pRequest]() {
                    if (m_pOtaOwner == pRequest) {
                        if (Update.isRunning()) {
                            OTA_ABORT();
                            Appl.MsgBus.postEventText(this,MSG_OTA_ERROR,"Client disconnected",0);
                        }
                        m_pOtaOwner = nullptr;
                    }
                });
            }
            if (m_pOtaOwner != pRequest || m_bOtaError) return;

            if (Update.write(data, len) != len) {
                m_bOtaError = true;
                ApplLogErrorWithParms(F("Firmware update: writing to flash failed (%s)"), OTA_ERROR_STRING().c_str());
                Appl.MsgBus.postEventText(this,MSG_OTA_ERROR,OTA_ERROR_STRING().c_str(),0);
                return;
            }
            // Progress events are throttled (queued, delivered in the loop)
            size_t nDone = index + len;
            if (final || nDone - m_nOtaProgressSent >= WEBSERVER_OTA_PROGRESS_STEP) {
                m_nOtaProgressSent = nDone;
                Appl.MsgBus.postEvent(this,MSG_OTA_PROGRESS,(int) nDone);
            }
            if (final) {
                if (Update.end(true)) {
                    ApplLogInfoWithParms(F("Firmware update finished (%uB)"),(unsigned) nDone);
                    Appl.MsgBus.postEvent(this,MSG_OTA_END,(int) nDone);
                } else {
                    m_bOtaError = true;
                    ApplLogErrorWithParms(F("Firmware update failed: %s"),OTA_ERROR_STRING().c_str());
                    Appl.MsgBus.postEventText(this,MSG_OTA_ERROR,OTA_ERROR_STRING().c_str(),0);
                }
            }
	});
    DEBUG_FUNC_END();
}

#pragma endregion
