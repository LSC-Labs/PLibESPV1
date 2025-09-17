#include <EventHandler.h>
#include <Arduino.h>


int CEventHandler::sendEvent(void *pSender, int nMsg, const void *pMessage, int nClass) {
    int nTotalResult = EVENT_MSG_RESULT_OK;
    std::vector<IMsgEventReceiver*> tCallBackEventReceivers;
    for(IMsgEventReceiver *pEventReceiver : m_oEventReceivers) {
        if(pSender != pEventReceiver) {
            try {
                int nResult = pEventReceiver->receiveEvent(pSender,nMsg,pMessage,nClass);
                if(nResult == EVENT_MSG_CALL_AGAIN_WHEN_ALL_OK) {
                    tCallBackEventReceivers.push_back(pEventReceiver);
                } else {
                    if(nResult > nTotalResult) nTotalResult = nResult;
                }
            } catch(...) {
                Serial.println("[X] sending event..." );
            }
        }
        if(nTotalResult == EVENT_MSG_RESULT_STOP_PROCESSING) break;
    }
    // Call backs if all is ok..
    if(nTotalResult == EVENT_MSG_RESULT_OK) {
        for(IMsgEventReceiver *pEventReceiver : tCallBackEventReceivers) {
            try {
                int nResult = pEventReceiver->receiveEvent(pSender,nMsg,pMessage,nClass);
                if(nResult > nTotalResult) nTotalResult = nResult;
            } catch(...) {
                Serial.println("[X] sending event..." );
            }
        }
    }
    return(nTotalResult);
}