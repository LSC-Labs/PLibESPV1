#ifndef DEBUG_LSC_NTPHANDLER
        #undef DEBUGINFOS
#endif
#include <NTPHandler.h>
#include <LSCUtils.h>
#include <Msgs.h>
#include <coredecls.h>
#include <DevelopmentHelper.h>
// #include <JsonHelper.h>


// WiFiStatus getStatus() { return Status; }    

#pragma region Module Interfaces

/**
 * @brief Writes NTP configuration into a JSON node.
 * @param oCfgNode Target configuration node.
 * @param bHideCritical Unused here; no NTP value is critical.
 */
void CNTPHandler::writeConfigTo( JsonNode &oCfgNode, bool bHideCritical) {
    oCfgNode["enabled"]        = Config.isEnabled;
    oCfgNode["server"]         = Config.NTPServer;
}

/**
 * @brief Reads NTP configuration from a JSON node.
 * @param oCfgNode Source configuration node. Missing values keep defaults.
 */
void CNTPHandler::readConfigFrom(JsonNode &oCfgNode) {
    oCfgNode.storeValueIf("enabled", & Config.isEnabled);
    oCfgNode.storeValueIf("server",    Config.NTPServer );
}

/**
 * @brief Writes current NTP/time status into a JSON node.
 *
 * The status contains an ISO timestamp, epoch-like seconds, day of week and
 * week number based on the current system time.
 *
 * @param oStatusObj Target status node.
 * @param nLevel Status verbosity, currently unused.
 */
void CNTPHandler::writeStatusTo( JsonNode &oStatusObj, int nLevel) {
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
 * @brief Gets the current native time_t value.
 * @return Current system time.
 */
time_t CNTPHandler::getNativeTime() {
    time(&m_oRawTime);
    return(m_oRawTime);
}

/**
 * @brief Gets the current local time as ISO-like date/time string.
 * @return Pointer to the internal timestamp buffer.
 */
const char * CNTPHandler::getISODateTime() {
time_t oNativeTime = getNativeTime();
struct tm* oTimeInfo = localtime(&oNativeTime);
memset(m_szISODateTime,'\0',sizeof(m_szISODateTime));
strftime(m_szISODateTime,sizeof(m_szISODateTime),"%FT%T",oTimeInfo);
return(m_szISODateTime);
}

/**
 * @brief Starts NTP setup when WiFi reports station-mode connectivity.
 * @return EVENT_MSG_RESULT_OK after processing.
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
 * @brief Callback invoked by the time service when the time was updated.
 */
void CNTPHandler::timeUpdatedByService() {
    DEBUG_FUNC_START();
    this->m_uLastUpdate = millis();
    DEBUG_FUNC_END();
}

/**
 * @brief Initializes the SNTP/time service once.
 *
 * The callback is registered before configTime() so successful updates can mark
 * this handler as having valid time.
 */
void CNTPHandler::setup(void) {
    DEBUG_FUNC_START();
    if(!m_bIsInitialized) {
        // std::function<void(void)> funcOnEvent;
        TrivialCB funcOnEvent;
        // Prepare the bind of the own onWebSocketEvent Handler function
        funcOnEvent = std::bind(&CNTPHandler::timeUpdatedByService,this);
        settimeofday_cb (funcOnEvent);
        DEBUG_INFOS(" calling => configTime(\"%s\",\"%s\")",Config.TZ.c_str(),Config.NTPServer.c_str());
        configTime(Config.TZ.c_str(), Config.NTPServer.c_str());
        m_bIsInitialized = true;
    }
    DEBUG_FUNC_END();
}

/**
 * @brief Checks if the time service has reported at least one update.
 * @return true after timeUpdatedByService() has been called.
 */
bool CNTPHandler::hasValidTime() {
    return(this->m_uLastUpdate > 0);
}
