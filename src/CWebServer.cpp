#ifndef DEBUG_LSC_WEBSERVER
    #undef DEBUGINFOS
#endif
#include <WebServer.h>
#include <Security.h>
#include <FileSystem.h>
#include <JsonHelper.h>
#include <DevelopmentHelper.h>


#define WEBSERVER_STATUS_DOC_SIZE  2048

const char * WEBSERVER_AUTOREDIRECT_MODE       = "autoRedirectMode";

#pragma region Constructor 
/** 
 * @brief Constructor 
 * You have to register the module after creating the instance!
 * @param nPortNumber 
 */ 
CWebServer::CWebServer(int nPortNumber)
	: AsyncWebServer(nPortNumber) {}

#pragma endregion

#pragma region Module (Config / Status / MessageBus) Interface implementation

/**
 * @brief Read the configuration from a JSON object
 */
void CWebServer::readConfigFrom(JsonObject &oNode) {
    LSC::setJsonValue(oNode,WEBSERVER_AUTOREDIRECT_MODE,&Config.AutoRedirectMode);
}

/**
 * @brief Write the configuration to a JSON object
 */
void CWebServer::writeConfigTo(JsonObject &oCfgNode,bool bHideCritical) {
    oCfgNode[WEBSERVER_AUTOREDIRECT_MODE] = Config.AutoRedirectMode;
    // oCfgNode[WEBSERVER_CONFIG_PASSWORD_KEY] = bHideCritical ? WEBSERVER_HIDDEN_PASSWORD : WebServer::Config.Passwd;
}

/** 
 * @brief Write the status information to a JSON object 
 */
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
#pragma endregion

#pragma region Registered File Access Routes

void CWebServer::deliverFile(AsyncWebServerRequest *pRequest) { 
        DEBUG_FUNC_START_PARMS("%s",pRequest->url().c_str());
        if (!Config.authenticate(pRequest,"file",true)) {
            return pRequest->requestAuthentication();
		}
        AsyncWebServerResponse *response = pRequest->beginResponse(200, "text/plain", "Success");
        setNewAuthHeader(pRequest,response);
        // TODO: implement proper file delivery with correct content-type
		pRequest->send(response);
        DEBUG_FUNC_END();
    }
/**
 * @brief register the file access routes
 * /files/list - list files
 * /files/get/<filename> - get/download file
 * /files/upload - upload a file
 */
void CWebServer::registerFileAccess() {

	// Webserver - Startseite
    on("/files/upload", HTTP_GET, [this](AsyncWebServerRequest *pRequest) {
        if (!Config.authenticate(pRequest,"files/upload",true)) {
            return pRequest->requestAuthentication();
		}
        pRequest->send(200, "text/html", "<form method='POST' action='/files/upload' enctype='multipart/form-data'>"
                                        "<input type='file' name='file'>"
                                        "<input type='submit' value='Upload'>"
                                        "</form>");
    });

	 // Upload-Handler
    on("/files/upload", HTTP_POST, 
        [this](AsyncWebServerRequest *pRequest) {
            if (!Config.authenticate(pRequest,"files/upload",true)) {
                return pRequest->requestAuthentication();
		    }
            pRequest->send(200);
        }, 
        [this](AsyncWebServerRequest *pRequest, const String& strFilename, size_t index, uint8_t *pData, size_t nLen, bool bFinal) {
            static File oFile;
            CFS oFS;
            if (index == 0) {  // Open new file
                ApplLogInfoWithParms(F("Saving File: %s"), strFilename.c_str());
                oFile =  oFS.getBaseFS().open("/" + strFilename, "w");
            }
            if (oFile) {
                oFile.write(pData, nLen);
            }
            if (bFinal) {  // Close file
                oFile.close();
                // TODO: check if duplicate route registration runs into problems...
                std::function<void(	AsyncWebServerRequest *)> funcDeliverFile;
                funcDeliverFile = std::bind(&CWebServer::deliverFile,this,std::placeholders::_1);
                on("/files/get/" + strFilename, HTTP_GET, funcDeliverFile);
                ApplLogInfoWithParms(F("Upload finished: %s"), strFilename.c_str());
            }
        }
    );
	
	// Show list of files
    on("/files/list", HTTP_GET, [this](AsyncWebServerRequest *pRequest) {
        CFS oFS;        
        if (!Config.authenticate(pRequest,"files/list",true)) {
            return pRequest->requestAuthentication();
		}
        String strFileList = oFS.getFileList("/");
        pRequest->send(200, "application/json", strFileList);
    });

    // Register file download routes...
    CFS oFS;
    Dir oDirEntry = oFS.getBaseFS().openDir("/");
    

	// Prepare the bind of the own onWebSocketEvent Handler function
	std::function<void(	AsyncWebServerRequest *)> funcDeliverFile;
    funcDeliverFile = std::bind(&CWebServer::deliverFile,this,std::placeholders::_1);
    
    while (oDirEntry.next()) {    
        if(oDirEntry.isFile()) {
            on("/files/get/" + oDirEntry.fileName(), HTTP_GET, funcDeliverFile);
        } 
    }
}

#pragma endregion

#pragma region Registered Default Routes (login, status, ota, notfound)
/** 
 * @brief register the defaults:
 * - rewrite "/" to "/index.html"
 * - offer the "/login"
 * - offer the OTA page "/update"
 * - offer the default "/status" REST API
 * */
void CWebServer::registerDefaults() {
    DEBUG_FUNC_START();
    rewrite("/", "/index.html");

    /** 
     * Not found handler - for captive portal support
     * If not found and capture mode is active, redirect to "/", if a HTML page is requested
     */
    onNotFound([this](AsyncWebServerRequest *pRequest) {
        bool bIsHtmlPage = true;
        if(pRequest->url().indexOf(".") > -1) {
            bIsHtmlPage = pRequest->url().endsWith(".htm") || pRequest->url().endsWith(".html");
        }
        DEBUG_INFOS("WEB: Requested URL: %s (CaptureMode=%d) (isHtmlPage=%s)",pRequest->url().c_str(),Status.AutoRedirectMode, bIsHtmlPage ? "true" : "false");
        if(Status.AutoRedirectMode && bIsHtmlPage) {
            DEBUG_INFO("WEB: Captive portal mode - redirect to /");
            AsyncWebServerResponse *pResponse = pRequest->beginResponse(302, "text/plain", "Redirecting to captive portal");
            pResponse->addHeader("Location", "/");
            pRequest->send(pResponse);
        } else {
            ApplLogErrorWithParms(F("WEB: 404 Not found: %s"),pRequest->url().c_str());
            AsyncWebServerResponse *pResponse = pRequest->beginResponse(404, "text/plain", "Not found");
            pRequest->send(pResponse);
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

        JSON_DOC(oStatusDoc,WEBSERVER_STATUS_DOC_SIZE);
        JsonObject oPayload = CreateJsonObject(oStatusDoc,"payload");
        Appl.writeStatusTo(oPayload);

        // Do it the old school way - do not use String here... Webserver destroys string info...
        int nSize = measureJson(oStatusDoc);
        char tBuffer[nSize];
        memset(tBuffer,'\0',sizeof(tBuffer));
        serializeJson(oPayload,&tBuffer,nSize);

        DEBUG_INFOS("WEB:/status -> %s",tBuffer);
        AsyncWebServerResponse *pResponse = pRequest->beginResponse_P(200, "application/json",  tBuffer);
        pRequest->send(pResponse);
    });

    on("/update", HTTP_POST, 
        [](AsyncWebServerRequest *pRequest) {
            DEBUG_INFO("WEB:/update (POST) - 1");
            AsyncWebServerResponse * pResponse = pRequest->beginResponse(200, "text/plain", "OK");
            pResponse->addHeader("Connection", "close");
            pRequest->send(pResponse);
        },
        [this](AsyncWebServerRequest *pRequest, String strFilename, size_t index, uint8_t *data, size_t len, bool final) {
            DEBUG_INFO("WEB:/update (POST) - 2");
            if (!index) {
                ApplLogInfoWithParms(F("Firmware update started : %s"),strFilename.c_str());
                Appl.MsgBus.sendEvent(this,MSG_OTA_START,strFilename.c_str(),0);
                Update.runAsync(true);
                if (!Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000)) {
                    ApplLogError(F("Not enough space..."));
                    Appl.MsgBus.sendEvent(this,MSG_OTA_ERROR,F("Not enough space"),0);
                }
            }
            if (!Update.hasError()) {
                Appl.MsgBus.sendEvent(this,MSG_OTA_PROGRESS,nullptr,(index + len));
                if (Update.write(data, len) != len) {
                    ApplLogErrorWithParms(F("Writing to flash failed..."), strFilename.c_str());
                    ApplLogErrorWithParms(F("%s"),Update.getErrorString().c_str());
                    Appl.MsgBus.sendEvent(this,MSG_OTA_ERROR,Update.getErrorString().c_str(),0);
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

#pragma endregion
