#ifndef DEBUG_LSC_MDNS
    #undef DEBUGINFOS
#endif


#include <MDNSController.h>
#include <DevelopmentHelper.h>

/** 
 * @brief Creates an mDNS controller and registers it on the application bus.
 */
CMDNSController::CMDNSController() {
    Appl.MsgBus.registerEventReceiver(this,"MDNSController");
}

/**
 * @brief Stops the mDNS responder before destruction.
 */
CMDNSController::~CMDNSController() {
    end();
}

/**
 * @brief Creates an mDNS controller with a custom bus registration name.
 */
CMDNSController::CMDNSController(const char *pszAutoregisterName) {
    Appl.MsgBus.registerEventReceiver(this,pszAutoregisterName);
}

/**
 * @brief Converts a name into a valid mDNS host label.
 *
 * Allowed are letters, digits and '-' (RFC 1123), max. 63 characters, no
 * leading/trailing '-'. Other characters (space, '_', '.', umlauts) become '-'.
 * An empty result falls back to WIFI_DEFAULT_AP_SSID_PREFIX / "esp".
 */
String CMDNSController::toHostLabel(const String &strName) {
    String strResult;
    for (size_t nIdx = 0; nIdx < strName.length() && strResult.length() < 63; nIdx++) {
        char c = strName[nIdx];
        bool bValid = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
        if (bValid) strResult += c;
        else if (strResult.length() > 0 && strResult[strResult.length() - 1] != '-') strResult += '-';
    }
    while (strResult.length() > 0 && strResult[strResult.length() - 1] == '-') {
        strResult.remove(strResult.length() - 1);
    }
    if (strResult.length() == 0) strResult = "esp";
    return(strResult);
}

/**
 * @brief Starts the mDNS responder and advertises HTTP on port 80.
 *
 * A running responder is stopped first, so a new start (i.e. after switching
 * from station to access point mode) binds to the current interface and does
 * not register the service twice.
 *
 * @param pszHostname Hostname to use. nullptr falls back to Appl.getDeviceName().
 * @return true on success.
 * @todo Make advertised services configurable.
 */
bool CMDNSController::begin(const char* pszHostname) {
    DEBUG_FUNC_START_PARMS("\"%s\"",NULL_POINTER_STRING(pszHostname));
    if(m_bRunning) end();
    // getDeviceName() returns a temporary String - keep the result alive while used
    String strHostname = toHostLabel(pszHostname ? String(pszHostname) : Appl.getDeviceName());
    DEBUG_INFOS("MDNS: start responder for hostname %s",strHostname.c_str());
    m_bRunning = MDNS.begin(strHostname.c_str());
    if(m_bRunning) {
        if(!MDNS.addService("http", "tcp", 80)) {
            ApplLogWarn(F("MDNS: could not register http service"));
        }
        ApplLogInfoWithParms(F("MDNS: responder started (%s.local)"),strHostname.c_str());
    } else {
        ApplLogErrorWithParms(F("MDNS: could not start responder (%s)"),strHostname.c_str());
    }
    DEBUG_FUNC_END_PARMS("%d",m_bRunning);
    return m_bRunning;
}   

/**
 * @brief Stops the mDNS responder (and its services) if it was started.
 */
void CMDNSController::end() {
    DEBUG_FUNC_START();
    if(m_bRunning) {
        DEBUG_INFOS("MDNS: stopping responder");
        MDNS.end();     // removes all services on ESP32 and ESP8266
        m_bRunning = false;
    }
    DEBUG_FUNC_END();
}

/**
 * @brief Handles application events for mDNS lifecycle and updates.
 *
 * MSG_APPL_LOOP updates a running responder (ESP8266), MSG_WIFI_CONNECTED
 * (re)starts it, MSG_WIFI_STARTING and MSG_WIFI_DISABLING stop it.
 *
 * @return EVENT_MSG_RESULT_OK.
 */
int CMDNSController::receiveEvent(const void * pSender, int nMsg, const void * pMessage, int nClass) {
    (void) pSender; (void) pMessage; (void) nClass;
    int nResult = EVENT_MSG_RESULT_OK;
    switch(nMsg) {
        // Regular update in program loop (Arduino style)
        case MSG_APPL_LOOP:
            {
                // ESP32 runs the responder in its own task - no update needed
                #ifndef ESP32
                    if(m_bRunning) MDNS.update();
                #endif
            }
            break;
        // Start MDNS when WiFi is connected
        case MSG_WIFI_CONNECTED:
            {
                this->begin();
            }
            break;
        // Stop MDNS when WiFi is (re)started or disabled - the interface changes
        case MSG_WIFI_STARTING:
        case MSG_WIFI_DISABLING:
            {
                this->end();
            }
            break;
        default:
            break;
    }
    return nResult;
}
