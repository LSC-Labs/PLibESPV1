#ifndef DEBUG_LSC_MDNS
    #undef DEBUGINFOS
#endif


#include <MDNSController.h>
#include <DevelopmentHelper.h>

CMDNSHandler::CMDNSHandler() {
    Appl.MsgBus.registerEventReceiver(this);
}

CMDNSHandler::~CMDNSHandler() {
    end();
}

/**
 * @brief Start the MDNS Responder with given hostname
 * Adds a HTTP service on port 80
 * @param pszHostname Hostname to use, if nullptr, the device name from Appl is used
 * @return true on success, false on failure
 * @todo: Make services configurable
 */
bool CMDNSHandler::begin(const char* pszHostname) {
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

void CMDNSHandler::end() {
    DEBUG_FUNC_START();
    DEBUG_INFOS("MDNS: stopping responder");
    MDNS.removeService(nullptr,"_http", "_tcp");
    MDNS.end();
    DEBUG_FUNC_END();
}

int CMDNSHandler::receiveEvent(const void * pSender, int nMsg, const void * pMessage, int nClass) {
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