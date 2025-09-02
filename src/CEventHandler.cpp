#include <EventHandler.h>
#include <Arduino.h>


int CEventHandler::sendEvent(void *pSender, int nMsgType, const void *pMessage, int nClass) {
    int nTotalResult = EVENT_MSG_RESULT_OK;
    for(IMsgEventReceiver *pEventReceiver : m_oEventReceivers) {
        if(pSender == nullptr || pSender != pEventReceiver) {
            try {
                int nResult = pEventReceiver->receiveEvent(pSender,nMsgType,pMessage,nClass);
                if(nResult > nTotalResult) nTotalResult = nResult;
            } catch(std::exception oEx) {
                Serial.println("[X] sending event..." );
                Serial.printf( "[X] %s\n",oEx.what());
            }
        }
        if(nTotalResult == EVENT_MSG_RESULT_STOP_PROCESSING) break;
    }
    return(nTotalResult);
}