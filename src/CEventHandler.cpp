#ifndef DEBUG_LSC_APPL
    #undef DEBUGINFOS
#endif
#include <EventHandler.h>
#include <JsonNode.h>
#include <Msgs.h>
#include <string.h>
#include <stdlib.h>

#if defined(ESP32)
    extern TaskHandle_t loopTaskHandle;         // Arduino loop task (cores/esp32/main.cpp)
    #define EVENT_DEFERRED_LOCK() std::lock_guard<std::mutex> oDeferredGuard(m_oDeferredLock)
#elif defined(NATIVE_RUNTIME)
    #define EVENT_DEFERRED_LOCK()
#else
    #include <coredecls.h>                      // can_yield()
    // ESP8266: SYS context and loop do not run in parallel - no lock needed
    #define EVENT_DEFERRED_LOCK()
#endif

namespace {
    /// @brief Default context detection.
    bool defaultContextProbe() {
        #if defined(ESP32)
            // Before the loop task exists (global constructors) everything is "loop"
            return(loopTaskHandle == nullptr || xTaskGetCurrentTaskHandle() == loopTaskHandle);
        #elif defined(NATIVE_RUNTIME)
            return(true);
        #else
            return(can_yield());                // false in the SYS context (lwIP callbacks)
        #endif
    }
}

CEventHandler::TContextProbe CEventHandler::s_fnContextProbe = defaultContextProbe;

void CEventHandler::setContextProbe(TContextProbe fnProbe) {
    s_fnContextProbe = fnProbe ? fnProbe : defaultContextProbe;
}

/**
 * @brief Detaches all receivers and frees queued events.
 */
CEventHandler::~CEventHandler() {
    for(auto pEntry : m_tReceiverTable.Entries) {
        if(pEntry->value) pEntry->value->m_oEventRegistrations.detach(this);
    }
    EVENT_DEFERRED_LOCK();
    for(auto &oEvent : m_tDeferredEvents) free(oEvent.pszData);
    m_tDeferredEvents.clear();
}
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
    } else if(pEventReceiver) {
        auto pEntry = m_tReceiverTable.set(pszReceiverName ? pszReceiverName : "-",pEventReceiver);
        if(pEntry) {
            pEventReceiver->m_oEventRegistrations.attach(this);
            DEBUG_INFOS("MsgBus: Registered new receiver... %p (%s)",pEntry->value,pEntry->getKey());
        }
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
    if(!isLoopContext()) {
        // Other context: events without payload are queued and delivered in the loop.
        if(pMessage == nullptr) {
            enqueue(pSender, nMsg, nClass, 0, nullptr);
            return(EVENT_MSG_RESULT_OK);
        }
        // A payload pointer can not be kept safely - deliver directly (legacy behavior).
        // Use postEventText()/postEventJson() for payloads from other contexts.
        return(dispatchEvent(pSender, nMsg, pMessage, nClass));
    }
    // Loop context: deliver queued events first - keeps the order of the events
    if(m_nDispatchDepth == 0) processDeferredEvents();
    return(dispatchEvent(pSender, nMsg, pMessage, nClass));
}

/**
 * @brief Delivers an event to all registered receivers.
 */
int CEventHandler::dispatchEvent(void *pSender, int nMsg, const void *pMessage, int nClass) {
    int nTotalResult = EVENT_MSG_RESULT_OK;
    std::vector<IMsgEventReceiver*> tCallBackEventReceivers;
    // Receivers may register/unregister (or be destroyed) while the event is dispatched:
    // - iterate by index over the receivers present at the start (push_back may reallocate)
    // - removed receivers are set to nullptr and purged after the outermost dispatch
    m_nDispatchDepth++;
    size_t nCount = m_tReceiverTable.Entries.size();
    for(size_t nIdx = 0; nIdx < nCount && nIdx < m_tReceiverTable.Entries.size(); nIdx++) {
        IMsgEventReceiver *pEventReceiver = m_tReceiverTable.Entries[nIdx]->value;
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
            // skip receivers removed/destroyed during the first pass
            if(!m_tReceiverTable.hasValueEntry(pEventReceiver)) continue;
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
    m_nDispatchDepth--;
    if(m_nDispatchDepth == 0 && m_bPurgePending) {
        m_bPurgePending = false;
        m_tReceiverTable.removeIf([](CNamedValueEntry<IMsgEventReceiver *> *pEntry) { return(pEntry->value == nullptr); });
    }
    return(nTotalResult);
}

#pragma region Deferred events (queue for events from other contexts)

/**
 * @brief Puts an event into the queue.
 * @param eKind 0 = no payload, 1 = text, 2 = JSON text.
 * @return false if the queue is full or memory is exhausted (event dropped).
 */
bool CEventHandler::enqueue(void *pSender, int nMsg, int nClass, uint8_t eKind, const char *pszData) {
    DeferredEvent oEvent;
    oEvent.pSender = pSender;
    oEvent.nMsg    = nMsg;
    oEvent.nClass  = nClass;
    oEvent.eKind   = eKind;
    if(eKind != 0) {
        oEvent.pszData = strdup(pszData ? pszData : "");
        if(!oEvent.pszData) { m_nDeferredDropped++; return(false); }
    }
    EVENT_DEFERRED_LOCK();
    if(m_tDeferredEvents.size() >= EVENT_DEFERRED_MAX_ENTRIES) {
        m_nDeferredDropped++;
        free(oEvent.pszData);
        return(false);
    }
    m_tDeferredEvents.push_back(oEvent);
    return(true);
}

bool CEventHandler::postEvent(void *pSender, int nMsgID, int nMsgType) {
    return(enqueue(pSender, nMsgID, nMsgType, 0, nullptr));
}

bool CEventHandler::postEventText(void *pSender, int nMsgID, const char *pszText, int nMsgType) {
    return(enqueue(pSender, nMsgID, nMsgType, 1, pszText));
}

bool CEventHandler::postEventJson(void *pSender, int nMsgID, CJsonNode &oNode, int nMsgType) {
    return(enqueue(pSender, nMsgID, nMsgType, 2, oNode.getAsJsonText()));
}

size_t CEventHandler::getDeferredEventCount() {
    EVENT_DEFERRED_LOCK();
    return(m_tDeferredEvents.size());
}

/**
 * @brief Delivers the queued events in their order (loop context only).
 *
 * Text payloads are passed as const char*, JSON payloads are parsed into a
 * JsonNode and passed as JsonNode*. Lost events (queue full) are reported once
 * as warning log entry.
 */
void CEventHandler::processDeferredEvents() {
    if(m_bProcessingDeferred || !isLoopContext()) return;
    m_bProcessingDeferred = true;
    for(;;) {
        DeferredEvent oEvent;
        bool bHasEvent = false;
        unsigned int nDropped = 0;
        {
            EVENT_DEFERRED_LOCK();
            if(!m_tDeferredEvents.empty()) {
                oEvent = m_tDeferredEvents.front();
                m_tDeferredEvents.pop_front();
                bHasEvent = true;
            } else {
                nDropped = m_nDeferredDropped;
                m_nDeferredDropped = 0;
            }
        }
        if(!bHasEvent) {
            if(nDropped > 0) {
                char szBuffer[64];
                snprintf(szBuffer, sizeof(szBuffer), "MsgBus: %u queued events dropped (queue full)", nDropped);
                dispatchEvent(nullptr, MSG_LOG_ENTRY, szBuffer, 'W');
            }
            break;
        }
        if(oEvent.eKind == 2) {
            CJsonNode oNode;
            if(oNode.parse(oEvent.pszData)) dispatchEvent(oEvent.pSender, oEvent.nMsg, &oNode, oEvent.nClass);
        } else {
            dispatchEvent(oEvent.pSender, oEvent.nMsg, oEvent.pszData, oEvent.nClass);
        }
        free(oEvent.pszData);
    }
    m_bProcessingDeferred = false;
}

#pragma endregion

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
