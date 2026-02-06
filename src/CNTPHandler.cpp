
#include <NTPHandler.h>
#include <LSCUtils.h>
#include <Msgs.h>
#include <coredecls.h>
#include <DevelopmentHelper.h>
#include <JsonHelper.h>

// WiFiStatus getStatus() { return Status; }    

#pragma region Module Interfaces

/**
 * write the config to Json Object
 */
void CNTPHandler::writeConfigTo( JsonObject &oCfgNode, bool bHideCritical) {
        oCfgNode["enabled"]        = Config.isEnabled;
        oCfgNode["server"]         = Config.NTPServer;
}

/**
 * read the config from the Json Object
 */
void CNTPHandler::readConfigFrom(JsonObject &oCfgNode) {
        LSC::setJsonValue(oCfgNode,"enabled", & Config.isEnabled);
        LSC::setJsonValue(oCfgNode,"server",    Config.NTPServer );
}

/**
 * write the status to the Json Object
 */
void CNTPHandler::writeStatusTo( JsonObject &oStatusObj) {
        oStatusObj["enabled"] = Config.isEnabled;
        time(&m_oRawTime);
        struct tm* oTimeInfo;
        oTimeInfo = localtime(&m_oRawTime);

        char szBuffer[80];
        // formats see: https://www.man7.org/linux/man-pages/man3/strftime.3.html
        strftime(szBuffer,80,"%FT%T",oTimeInfo);
        oStatusObj["iso"] = szBuffer;
        strftime(szBuffer,80,"%s",oTimeInfo);
        oStatusObj["dt"] = szBuffer;
        strftime(szBuffer,80,"%u",oTimeInfo);
        oStatusObj["dow"] = szBuffer;
        strftime(szBuffer,80,"%W",oTimeInfo);
        oStatusObj["week"] = szBuffer;
}

#pragma endregion

#pragma date / time queries

/**
 * get the native time from the system
 */
time_t CNTPHandler::getNativeTime() {
        time(&m_oRawTime);
        return(m_oRawTime);
}

/**
 * get the ISO date and time from the system
 */
const char * CNTPHandler::getISODateTime() {
        time_t oNativeTime = getNativeTime();
        struct tm* oTimeInfo = localtime(&oNativeTime);
        memset(m_szISODateTime,'\0',sizeof(m_szISODateTime));
        strftime(m_szISODateTime,sizeof(m_szISODateTime),"%FT%T",oTimeInfo);
        return(m_szISODateTime);
}

/**
 * call setup, when the WiFi is connected in Station Mode - internet should be available
 */
int CNTPHandler::receiveEvent(const void * pSender, int nMsgType, const void * pMessage, int nType) {
        switch(nMsgType) {
                case MSG_WIFI_CONNECTED: // Wifi was connected... initialize this service...
                        DEBUG_INFO(" ## -> WiFi connected...");
                        if (nType == WIFI_STATION_MODE) {
                                DEBUG_INFO(" ## -> Starting setup of NTP Handler");
                                setup();
                        }
                        break;
        }
        return(EVENT_MSG_RESULT_OK);
}

/**
 * Callback of the DNS service
 */
void CNTPHandler::timeUpdatedByService() {
        DEBUG_FUNC_START();
        this->m_uLastUpdate = millis();
        DEBUG_FUNC_END();
}

/**
 * start the time service
 */
void CNTPHandler::setup(void) {
        DEBUG_FUNC_START();
        if(!m_bIsInitialized) {
                std::function<void(void)> funcOnEvent;
                // Prepare the bind of the own onWebSocketEvent Handler function
	        funcOnEvent = std::bind(&CNTPHandler::timeUpdatedByService,this);
                settimeofday_cb (funcOnEvent);
                DEBUG_INFOS(" calling => configTime(%s,%s)",Config.TZ.c_str(),Config.NTPServer.c_str());
                configTime(Config.TZ.c_str(), Config.NTPServer.c_str());
                m_bIsInitialized = true;
        }
        DEBUG_FUNC_END();
}

/**
 * check if a valid time is available
 */
bool CNTPHandler::hasValidTime() {
        return(this->m_uLastUpdate > 0);
}
