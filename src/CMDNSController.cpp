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

bool CMDNSHandler::begin(const char* pszHostname) {
    DEBUG_FUNC_START_PARMS("\"%s\"",NULL_POINTER_STRING(pszHostname));
    if(pszHostname == nullptr) {
        pszHostname = Appl.getDeviceName().c_str();
    }
    DEBUG_INFOS("MDNS: start responder for hostname %s",pszHostname);
    bool bBegined = MDNS.begin(pszHostname);
    DEBUG_FUNC_END_PARMS("%d",bBegined);
    return bBegined;
}   

void CMDNSHandler::end() {
    DEBUG_FUNC_START();
    DEBUG_INFOS("MDNS: stopping responder");
    MDNS.end();
    DEBUG_FUNC_END();
}

int CMDNSHandler::receiveEvent(const void * pSender, int nMsg, const void * pMessage, int nClass) {
    int nResult = EVENT_MSG_RESULT_OK;
    switch(nMsg) {
        case MSG_WIFI_CONNECTED:
            {
                this->begin();
            }
            break;
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