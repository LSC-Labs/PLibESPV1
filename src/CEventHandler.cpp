#ifndef DEBUG_LSC_APPL
    #undef DEBUGINFOS
#endif
#include <EventHandler.h>
#include <Arduino.h>
#include <DevelopmentHelper.h>

/**
 * @brief Registers an event receiver on the message bus.
 *
 * Receivers are unique by pointer. Registering the same receiver again is
 * ignored, even if another name is supplied.
 *
 * @param pEventReceiver Receiver object to notify on sendEvent().
 * @param pszReceiverName Optional diagnostic name for dumps and debug output.
 */
void CEventHandler::registerEventReceiver(IMsgEventReceiver *pEventReceiver, const char *pszReceiverName) {
    // bool bAlreadyRegistered = false;
    DEBUG_FUNC_START_PARMS("%p,%s",pEventReceiver,NULL_POINTER_STRING(pszReceiverName)); 
    if(m_tReceiverTable.hasValueEntry(pEventReceiver)) {
        DEBUG_INFOS("MsgBus: Receiver already registered... %p (%s)",pEventReceiver,pszReceiverName ? pszReceiverName : "-no name-");
        #if DEBUGINFOS
            dumpReceiver();
        #endif
    } else {
        auto pEntry = m_tReceiverTable.set(pszReceiverName ? pszReceiverName : "-",pEventReceiver);
        DEBUG_INFOS("MsgBus: Registered new receiver... %p (%s)",pEntry->value,pEntry->getKey());
    }
/*
    for(HandlerEntry * pEntry : m_tEventReceivers) {
        if(pEntry->pHandler == pEventReceiver) {
            DEBUG_INFOS("MsgBus: Receiver already registered... %p (%s)",pEventReceiver,pszReceiverName ? pszReceiverName : "-no name-");
            #if DEBUGINFOS
                dumpReceiver();
            #endif
            bAlreadyRegistered = true;
        }
    }
    if(!bAlreadyRegistered) {
        HandlerEntry * pNewEntry = new HandlerEntry(pszReceiverName,pEventReceiver);
        m_tEventReceivers.push_back(pNewEntry);
        DEBUG_INFOS("MsgBus: Registered new receiver... %p (%s)",pNewEntry->pHandler, pNewEntry->pszName ? pNewEntry->pszName : "-no name-");

        m_tReceiverTable.set(pszReceiverName,pEventReceiver);
        // DEBUG_INFOS("EvtBus: Registered new receiver... %p (%s)",pNewEntry->pHandler, pNewEntry->pszName ? pNewEntry->pszName : "-no name-");
        dumpReceiver();
    }
        */
    DEBUG_FUNC_END();
}

/**
 * @brief Sends an event to all registered receivers.
 *
 * The sender does not receive its own event. Receivers can return
 * EVENT_MSG_CALL_AGAIN_WHEN_ALL_OK to request a second callback only if the
 * first pass completes without warnings or errors.
 *
 * @param pSender Sender address, or nullptr for static/global events.
 * @param nMsg Message number, usually one of Msgs.h.
 * @param pMessage Optional message payload pointer.
 * @param nClass Additional message class or processing hint.
 * @return Highest receiver result, or EVENT_MSG_RESULT_STOP_PROCESSING if a
 *         receiver aborts processing.
 */
int CEventHandler::sendEvent(void *pSender, int nMsg, const void *pMessage, int nClass) {
    int nTotalResult = EVENT_MSG_RESULT_OK;
    std::vector<IMsgEventReceiver*> tCallBackEventReceivers;
    for(auto pEntry : m_tReceiverTable.Entries) {
        IMsgEventReceiver *pEventReceiver = pEntry->value;
        if(pEventReceiver && pSender != pEventReceiver) {
            #ifdef LSC_ENABLE_EXCEPTIONS
            try {
            #endif
                int nResult = pEventReceiver->receiveEvent(pSender,nMsg,pMessage,nClass);
                if(nResult == EVENT_MSG_CALL_AGAIN_WHEN_ALL_OK) {
                    tCallBackEventReceivers.push_back(pEventReceiver);
                } else {
                    if(nResult > nTotalResult) nTotalResult = nResult;
                }
            #ifdef LSC_ENABLE_EXCEPTIONS
            } catch(...) {
                Serial.println("[X] sending event..." );
            }
            #endif
        }
        if(nTotalResult == EVENT_MSG_RESULT_STOP_PROCESSING) break;
    }
    // Call backs if all is ok..
    if(nTotalResult == EVENT_MSG_RESULT_OK) {
        for(IMsgEventReceiver *pEventReceiver : tCallBackEventReceivers) {
            #ifdef LSC_ENABLE_EXCEPTIONS
            try {
            #endif
                int nResult = pEventReceiver->receiveEvent(pSender,nMsg,pMessage,nClass);
                if(nResult > nTotalResult) nTotalResult = nResult;
            #ifdef LSC_ENABLE_EXCEPTIONS
            } catch(...) {
                Serial.println("[X] sending event..." );
            }
            #endif
        }
    }
    return(nTotalResult);
}

/*int CEventHandler::sendEvent(void *pSender, int nMsg, const void *pMessage, int nClass) {
    int nTotalResult = EVENT_MSG_RESULT_OK;
    std::vector<IMsgEventReceiver*> tCallBackEventReceivers;
    for(HandlerEntry * pEntry : m_tEventReceivers) {
        IMsgEventReceiver *pEventReceiver = pEntry->pHandler;
        if(pEventReceiver && pSender != pEventReceiver) {
            #ifdef LSC_ENABLE_EXCEPTIONS
            try {
            #endif
                int nResult = pEventReceiver->receiveEvent(pSender,nMsg,pMessage,nClass);
                if(nResult == EVENT_MSG_CALL_AGAIN_WHEN_ALL_OK) {
                    tCallBackEventReceivers.push_back(pEventReceiver);
                } else {
                    if(nResult > nTotalResult) nTotalResult = nResult;
                }
            #ifdef LSC_ENABLE_EXCEPTIONS
            } catch(...) {
                Serial.println("[X] sending event..." );
            }
            #endif
        }
        if(nTotalResult == EVENT_MSG_RESULT_STOP_PROCESSING) break;
    }
    // Call backs if all is ok..
    if(nTotalResult == EVENT_MSG_RESULT_OK) {
        for(IMsgEventReceiver *pEventReceiver : tCallBackEventReceivers) {
            #ifdef LSC_ENABLE_EXCEPTIONS
            try {
            #endif
                int nResult = pEventReceiver->receiveEvent(pSender,nMsg,pMessage,nClass);
                if(nResult > nTotalResult) nTotalResult = nResult;
            #ifdef LSC_ENABLE_EXCEPTIONS
            } catch(...) {
                Serial.println("[X] sending event..." );
            }
            #endif
        }
    }
    return(nTotalResult);
}*/
