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
 * @brief Starts the mDNS responder and advertises HTTP on port 80.
 * @param pszHostname Hostname to use. nullptr falls back to Appl.getDeviceName().
 * @return true on success.
 * @todo Make advertised services configurable.
 */
bool CMDNSController::begin(const char* pszHostname) {
    DEBUG_FUNC_START_PARMS("\"%s\"",NULL_POINTER_STRING(pszHostname));
    if(pszHostname == nullptr) {
        pszHostname = Appl.getDeviceName().c_str();
    }
    DEBUG_INFOS("MDNS: start responder for hostname %s",pszHostname);
    bool bBegined = MDNS.begin(pszHostname);
    MDNS.addService("_http", "_tcp", 80);
    DEBUG_FUNC_END_PARMS("%d",bBegined);
    return bBegined;
}   

/**
 * @brief Stops the mDNS responder and removes the HTTP service.
 */
void CMDNSController::end() {
    DEBUG_FUNC_START();
    DEBUG_INFOS("MDNS: stopping responder");
    MDNS.removeService(nullptr,"_http", "_tcp");
    MDNS.end();
    DEBUG_FUNC_END();
}

/**
 * @brief Handles application events for mDNS lifecycle and updates.
 *
 * MSG_APPL_LOOP updates a running responder, MSG_WIFI_CONNECTED starts it, and
 * MSG_WIFI_DISABLING stops it.
 *
 * @return EVENT_MSG_RESULT_OK.
 */
int CMDNSController::receiveEvent(const void * pSender, int nMsg, const void * pMessage, int nClass) {
    int nResult = EVENT_MSG_RESULT_OK;
    switch(nMsg) {
        // Regular update in program loop (Arduino style)
        case MSG_APPL_LOOP:
            {
                if(MDNS.isRunning()) {
                    MDNS.update();
                }
            }
            break;
        // Start MDNS when WiFi is connected
        case MSG_WIFI_CONNECTED:
            {
                this->begin();
            }
            break;
        // Stop MDNS when WiFi is disabling
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
