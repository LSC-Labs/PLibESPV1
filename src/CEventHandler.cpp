#include <EventHandler.h>


int CEventHandler::sendEvent(void *pSender, int nMsgType, const void *pMessage, int nClass) {
    int nTotalResult = EVENT_MSG_RESULT_OK;
    for(IMsgEventReceiver *pEventReceiver : m_oEventReceivers) {
        if(pSender == nullptr || pSender != pEventReceiver) {
            int nResult = pEventReceiver->receiveEvent(pSender,nMsgType,pMessage,nClass);
            if(nResult > nTotalResult) nTotalResult = nResult;
        }
        if(nTotalResult == EVENT_MSG_RESULT_STOP_PROCESSING) break;
    }
    return(nTotalResult);
}