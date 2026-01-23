#ifndef DEBUG_LSC_WEBSERVER
    #undef DEBUGINFOS
#endif
#include <WebServer.h>
#include <Security.h>
#include <FileSystem.h>
#include <JsonHelper.h>
#include <DevelopmentHelper.h>


const char * WEBSERVER_AUTOREDIRECT_MODE       = "autoRedirectMode";
/*
#define WEBSERVER_HIDDEN_PASSWORD       "******"
#define WEBSERVER_CONFIG_PASSWORD_KEY   "httpPasswd"
*/

namespace WebServer {
    WebServerConfig Config;
}

/// @brief Constructor with route register callback
/// Registers per default alos all well known routes
/// @param nPortNumber 
/// @param bRegisterRoutes true - default routes will be registered, otherwise do it by yourself.
/// 
CWebServer::CWebServer(int nPortNumber )
	: AsyncWebServer(nPortNumber) {}

void CWebServer::readConfigFrom(JsonObject &oNode) {
    WebServer::Config.AutoRedirectMode = oNode[WEBSERVER_AUTOREDIRECT_MODE] | false;
    /* Currently no config - password is moved to appl - device password.
    String strPasswd = oNode[WEBSERVER_CONFIG_PASSWORD_KEY];
    if(!strPasswd.equals(WEBSERVER_HIDDEN_PASSWORD)) {
        WebServer::Config.Passwd = strPasswd;
    }
    */
}

void CWebServer::writeConfigTo(JsonObject &oCfgNode,bool bHideCritical) {
    oCfgNode[WEBSERVER_AUTOREDIRECT_MODE] = WebServer::Config.AutoRedirectMode;
    // oCfgNode[WEBSERVER_CONFIG_PASSWORD_KEY] = bHideCritical ? WEBSERVER_HIDDEN_PASSWORD : WebServer::Config.Passwd;
}

void CWebServer::writeStatusTo(JsonObject &oStatusNode) {
    oStatusNode["started"] = Status.Started;
    oStatusNode[WEBSERVER_AUTOREDIRECT_MODE] = Status.AutoRedirectMode;
}   

int CWebServer::receiveEvent(const void * pSender, int nMsg, const void * pMessage, int nClass) {
    int nResult = EVENT_MSG_RESULT_OK;
    switch(nMsg) {
        case MSG_CAPTIVE_PORTAL_STARTED: {
            Status.AutoRedirectMode = true;
            DEBUG_INFO("WEB: Autoredirect mode enabled");
            break;
        }
    }
    return(nResult);
}


void CWebServer::registerFileAccess() {

	// Webserver - Startseite
    on("/files/upload", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (!WebServer::Config.authenticate(request,"files/upload",true)) {
            return request->requestAuthentication();
		}
        request->send(200, "text/html", "<form method='POST' action='/files/upload' enctype='multipart/form-data'>"
                                        "<input type='file' name='file'>"
                                        "<input type='submit' value='Upload'>"
                                        "</form>");
    });

	 // Upload-Handler
    on("/files/upload", HTTP_POST, 
        [](AsyncWebServerRequest *request) {
            if (!WebServer::Config.authenticate(request,"files/upload",true)) {
                return request->requestAuthentication();
		    }
            request->send(200);
        }, 
        [](AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final) {
            static File file;
            CFS oFS;
            if (index == 0) {  // Neue Datei öffnen
                Serial.printf("Saving File: %s\n", filename.c_str());
                file =  oFS.getBaseFS().open("/" + filename, "w");
            }
            if (file) {
                file.write(data, len);
            }
            if (final) {  // Datei schließen
                file.close();
                Serial.printf("Upload finished: %s\n", filename.c_str());
            }
        }
    );
	
	// Liste der Dateien anzeigen
    on("/files/list", HTTP_GET, [](AsyncWebServerRequest *request) {
        CFS oFS;        
        if (!WebServer::Config.authenticate(request,"files/list",true)) {
            return request->requestAuthentication();
		}
        String strFileList = oFS.getFileList("/");
        request->send(200, "application/json", strFileList);
    });
}

/// @brief register the defaults:
/// - rewrite "/" to "/index.html"
/// - offer the "/login"
/// - offer the OTA page "/update"
/// - offer the default "/status" REST API
void CWebServer::registerDefaults() {
    DEBUG_FUNC_START();
    rewrite("/", "/index.html");

    // Not found handler - for captive portal support
    // If not found and capture mode is active, redirect to "/", if a HTML page is requested
    onNotFound([this](AsyncWebServerRequest *request) {
        bool bIsHtmlPage = true;
        ApplLogErrorWithParms(F("WEB: 404 Not found: %s"),request->url().c_str());
        if(request->url().indexOf(".") > -1) {
            bIsHtmlPage = request->url().endsWith(".htm") || request->url().endsWith(".html");
        }
        DEBUG_INFOS("WEB: Requested URL: %s (CaptureMode=%d) (isHtmlPage=%s)",request->url().c_str(),Status.AutoRedirectMode, bIsHtmlPage ? "true" : "false");
        if(Status.AutoRedirectMode && bIsHtmlPage) {
            DEBUG_INFO("WEB: Captive portal mode - redirect to /");
            AsyncWebServerResponse *response = request->beginResponse(302, "text/plain", "Redirecting to captive portal");
            response->addHeader("Location", "/");
            request->send(response);
        } else {
            AsyncWebServerResponse *response = request->beginResponse(404, "text/plain", "Not found");
            request->send(response);
        }
	});

    on("/login", HTTP_GET, [](AsyncWebServerRequest *pRequest) {    
        DEBUG_FUNC_START();
        if (!WebServer::Config.authenticate(pRequest,"login",true)) {
            return pRequest->requestAuthentication();
		}
        AsyncWebServerResponse *response = pRequest->beginResponse(200, "text/plain", "Success");
        setNewAuthHeader(pRequest,response);
        ApplLogInfoWithParms(F("Login from address %s"),pRequest->client()->remoteIP().toString().c_str());
		pRequest->send(response);
	});

     on("/status", HTTP_GET, [](AsyncWebServerRequest *pRequest) {
        ApplLogInfo(F("WEB:/status"));
        DEBUG_INFO("/status route called...");

        // DynamicJsonDocument oStatusDoc(2048);
        // JsonObject oPayload = oStatusDoc.createNestedObject("payload");
        JSON_DOC(oStatusDoc,2048);
        JsonObject oPayload = CreateJsonObject(oStatusDoc,"payload");
        
        Appl.writeStatusTo(oPayload);
        // Do it the old school way - do not use String here... Webserver destroys string info...
        int nSize = measureJson(oStatusDoc);
        char tBuffer[nSize*2];
        memset(tBuffer,'\0',sizeof(tBuffer));
        serializeJson(oPayload,&tBuffer,nSize*2);
        DEBUG_INFOS("WEB:/status -> %s",tBuffer);
        AsyncWebServerResponse *response = pRequest->beginResponse_P(200, "application/json",  tBuffer);
        pRequest->send(response);
    });

    on("/update", HTTP_POST, 
        [](AsyncWebServerRequest *request) {
            ApplLogInfo(F("WEB:/update (POST)"));
            AsyncWebServerResponse * response = request->beginResponse(200, "text/plain", "OK");
            response->addHeader("Connection", "close");
            request->send(response);
        },
        [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
            if (!index) {
                ApplLogInfoWithParms(F("Firmware update started : %s"),filename.c_str());
                Appl.MsgBus.sendEvent(nullptr,MSG_OTA_START,filename.c_str(),0);
                Update.runAsync(true);
                if (!Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000)) {
                    ApplLogError(F("Not enough space..."));
                    Appl.MsgBus.sendEvent(nullptr,MSG_OTA_ERROR,F("Not enough space"),0);
                }
            }
            if (!Update.hasError()) {
                Appl.MsgBus.sendEvent(nullptr,MSG_OTA_PROGRESS,nullptr,(index + len));
                if (Update.write(data, len) != len) {
                    ApplLogErrorWithParms(F("Writing to flash failed..."), filename.c_str());
                    ApplLogErrorWithParms(F("%s"),Update.getErrorString().c_str());
                    Appl.MsgBus.sendEvent(nullptr,MSG_OTA_ERROR,Update.getErrorString().c_str(),0);
                }
            }
            if (final) {
                if (Update.end(true)) {
                    ApplLogInfoWithParms(F("Firmware update finished (%uB)"),index + len);
                    if(!Update.hasError()) Appl.MsgBus.sendEvent(nullptr,MSG_REBOOT_REQUEST,F("Firmware update"),0);
                } else {
                    ApplLogError(F("Firmware update failed."));
                    ApplLogErrorWithParms(F("%s"),Update.getErrorString().c_str());
                }
            }
	});
    DEBUG_FUNC_END();
    
}

