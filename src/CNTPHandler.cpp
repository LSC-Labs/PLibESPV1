#ifndef DEBUG_LSC_NTPHANDLER
        #undef DEBUGINFOS
#endif
#include <NTPHandler.h>
#include <LSCUtils.h>
#include <Msgs.h>
#ifdef ESP32
    #include <esp_sntp.h>
    // SNTP callback is a plain C function pointer - route it to the handler instance
    static CNTPHandler *s_pNTPHandlerInstance = nullptr;
    static void onSntpTimeSync(struct timeval *) {
        if(s_pNTPHandlerInstance) s_pNTPHandlerInstance->timeUpdatedByService();
    }
#else
    #include <coredecls.h>
#endif
#include <DevelopmentHelper.h>



#pragma region Module Interfaces

/**
 * @brief Writes NTP configuration into a JSON node.
 * @param oCfgNode Target configuration node.
 * @param bHideCritical Unused here; no NTP value is critical.
 */
void CNTPHandler::writeConfigTo( JsonNode &oCfgNode, bool bHideCritical) {
    (void) bHideCritical;
    oCfgNode["enabled"]        = Config.isEnabled;
    oCfgNode["server"]         = Config.NTPServer;
    oCfgNode["tz"]             = Config.TZ;
}

/**
 * @brief Reads NTP configuration from a JSON node.
 * @param oCfgNode Source configuration node. Missing values keep defaults.
 */
void CNTPHandler::readConfigFrom(JsonNode &oCfgNode) {
    oCfgNode.storeValueIf("enabled", & Config.isEnabled);
    oCfgNode.storeValueIf("server",    Config.NTPServer );
    // POSIX TZ string, i.e. "CET-1CEST,M3.5.0/02,M10.5.0/03" (applied by setup(), i.e. after reboot)
    oCfgNode.storeValueIf("tz",        Config.TZ );
}

/**
 * @brief Gets the current local time - thread safe (localtime_r).
 *
 * localtime() uses one static buffer for all callers; the status is also
 * written from the async web server (/status), so it must not be used here.
 * @return false if the time could not be converted.
 */
bool CNTPHandler::getLocalTime(struct tm &oTimeInfo) {
    time_t oNow = time(nullptr);
    m_oRawTime = oNow;
    return(localtime_r(&oNow, &oTimeInfo) != nullptr);
}

/**
 * @brief Writes current NTP/time status into a JSON node.
 *
 * The status contains the sync state, an ISO timestamp, seconds since epoch,
 * day of week (1 = Monday) and the ISO 8601 week number.
 *
 * @param oStatusObj Target status node.
 * @param nLevel Status verbosity, currently unused.
 */
void CNTPHandler::writeStatusTo( JsonNode &oStatusObj, int nLevel) {
    (void) nLevel;
    oStatusObj["enabled"] = Config.isEnabled;
    oStatusObj["valid"]   = hasValidTime();     // false: time not synchronized yet (1970...)
    struct tm oTimeInfo;
    if(!getLocalTime(oTimeInfo)) return;

    char szBuffer[40];
    // formats see: https://www.man7.org/linux/man-pages/man3/strftime.3.html
    strftime(szBuffer,sizeof(szBuffer),"%FT%T",&oTimeInfo);
    oStatusObj["iso"] = szBuffer;
    // seconds since epoch (UTC) - "%s" is a GNU extension, so it is formatted directly
    snprintf(szBuffer,sizeof(szBuffer),"%lld",(long long) m_oRawTime);
    oStatusObj["dt"] = szBuffer;
    strftime(szBuffer,sizeof(szBuffer),"%u",&oTimeInfo);
    oStatusObj["dow"] = szBuffer;
    // ISO 8601 week (%V) - "%W" counted the weeks from the first Monday, which
    // differs from the ISO/DIN week number at the beginning/end of a year
    strftime(szBuffer,sizeof(szBuffer),"%V",&oTimeInfo);
    oStatusObj["week"] = szBuffer;
}

#pragma endregion

#pragma region date / time queries

/**
 * @brief Gets the current native time_t value.
 * @return Current system time.
 */
time_t CNTPHandler::getNativeTime() {
    m_oRawTime = time(nullptr);
    return(m_oRawTime);
}

/**
 * @brief Gets the current local time as ISO-like date/time string.
 * @return Pointer to the internal timestamp buffer.
 */
const char * CNTPHandler::getISODateTime() {
    struct tm oTimeInfo;
    memset(m_szISODateTime,'\0',sizeof(m_szISODateTime));
    if(getLocalTime(oTimeInfo)) {
        strftime(m_szISODateTime,sizeof(m_szISODateTime),"%FT%T",&oTimeInfo);
    }
    return(m_szISODateTime);
}

#pragma endregion

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
    // Runs in the lwIP/SNTP context - only store values (no logging, no events)
    this->m_uLastUpdate = millis();
    this->m_bTimeValid  = true;     // separate flag - millis() may be 0
}

/**
 * @brief Initializes the SNTP/time service once (only if enabled in the config).
 *
 * The callback is registered before configTime() so successful updates can mark
 * this handler as having valid time. Server and time zone changes take effect
 * after a reboot.
 */
void CNTPHandler::setup(void) {
    DEBUG_FUNC_START();
    if(!m_bIsInitialized && Config.isEnabled && Config.NTPServer.length() > 0) {
        // lwIP (sntp_setservername) keeps only the POINTER to the server name.
        // Config.NTPServer may change at runtime (saveconfig) - use an own, stable copy.
        strncpy(m_szServer, Config.NTPServer.c_str(), sizeof(m_szServer) - 1);
        m_szServer[sizeof(m_szServer) - 1] = '\0';
        DEBUG_INFOS(" calling => configTime(\"%s\",\"%s\")",Config.TZ.c_str(),m_szServer);
        // The TZ string is copied (setenv) - no lifetime issue
        #ifdef ESP32
            s_pNTPHandlerInstance = this;
            sntp_set_time_sync_notification_cb(onSntpTimeSync);
            configTzTime(Config.TZ.c_str(), m_szServer);
        #else
            settimeofday_cb([this]() { this->timeUpdatedByService(); });
            configTime(Config.TZ.c_str(), m_szServer);
        #endif
        m_bIsInitialized = true;
    }
    DEBUG_FUNC_END();
}

/**
 * @brief Checks if the time service has reported at least one update.
 * @return true after timeUpdatedByService() has been called.
 */
bool CNTPHandler::hasValidTime() {
    return(this->m_bTimeValid);
}
