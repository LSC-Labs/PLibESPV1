#ifndef DEBUG_LSC_APPL
    #undef DEBUGINFOS
#endif
#include <EventHandler.h>
#include <Arduino.h>
#include <DevelopmentHelper.h>

/**
 * @brief Register an Event Receiver to the Event Handler
 * If a receiver is already registered, it will not be added again.
 * @param pEventReceiver Pointer to the Event Receiver to register
 */
void CEventHandler::registerEventReceiver(IMsgEventReceiver *pEventReceiver, const char *pszReceiverName) {
    bool bAlreadyRegistered = false;
    DEBUG_FUNC_START_PARMS("%p,%s",pEventReceiver,NULL_POINTER_STRING(pszReceiverName)); 
    for(HandlerEntry oEntry : m_oEventReceivers) {
        if(oEntry.pHandler == pEventReceiver) {
            DEBUG_INFOS("MsgBus: Receiver already registered... %p (%s)",pEventReceiver,pszReceiverName ? pszReceiverName : "-no name-");
            #if DEBUGINFOS
                dumpReceiver();
            #endif
            bAlreadyRegistered = true;
        }
    }
    if(!bAlreadyRegistered) {
        HandlerEntry oNewEntry;
        oNewEntry.pHandler = pEventReceiver;
        oNewEntry.pszName = pszReceiverName;
        m_oEventReceivers.push_back(oNewEntry);
        DEBUG_INFOS("MsgBus: Registered new receiver... %p (%s)",pEventReceiver,pszReceiverName ? pszReceiverName : "-no name-");
    }
    DEBUG_FUNC_END();
}

/**
 * send an event on the message bus 
 * 
 * @param pSender The address of the sender (will not receive the message) or nullptr, if it is a static method
 * @param nMsg The message number - use Msgs.h for default messages
 * @param pMessage The message as a pointer to the message to be sent
 * @param nClass A specialized instruction to process the message by receiver.
 * @returns EVENT_MSG_RESULT_OK, EVENT_MSG_RESULT_STOP_PROCESSING...
 */
int CEventHandler::sendEvent(void *pSender, int nMsg, const void *pMessage, int nClass) {
    int nTotalResult = EVENT_MSG_RESULT_OK;
    std::vector<IMsgEventReceiver*> tCallBackEventReceivers;
    for(HandlerEntry oEntry : m_oEventReceivers) {
        IMsgEventReceiver *pEventReceiver = oEntry.pHandler;
        if(pEventReceiver && pSender != pEventReceiver) {
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