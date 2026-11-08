#pragma once 
#include <Runtime.h>
#include <functional>
#include <vector>
#include <queue>
#include <deque>
#ifdef ESP32
    #include <mutex>
#endif
#include <NamedValueTable.h>
#include <DevelopmentHelper.h>
#include <Registration.h>

#define EVENT_MSG_CALL_AGAIN_WHEN_ALL_OK -9
#define EVENT_MSG_RESULT_OK               0
#define EVENT_MSG_RESULT_WARN             1
#define EVENT_MSG_RESULT_ERROR            2
#define EVENT_MSG_RESULT_STOP_PROCESSING 99

#ifndef EVENT_DEFERRED_MAX_ENTRIES
    #define EVENT_DEFERRED_MAX_ENTRIES   32     ///< events waiting for delivery in the loop
#endif

class CJsonNode;


class CEventHandler;

/**
 * Interface for Message Event Receivers.
 * A receiver unregisters itself automatically from all message buses when it
 * is destroyed.
 */
class IMsgEventReceiver
{
    friend class CEventHandler;
    private:
        CRegistrations m_oEventRegistrations;   // message buses this receiver is registered in

    public:
        virtual ~IMsgEventReceiver() { m_oEventRegistrations.releaseAll(this); }
        virtual int receiveEvent(const void * pSender, int nMsg, const void * pMessage, int nMsgInfo) = 0;
};  

/**
 * Event Handler Manager (message bus).
 *
 * Receivers are always called in the loop context (Arduino loop task / cont).
 * Events from other contexts - AsyncTCP task on ESP32, SYS context on ESP8266
 * (web server, WebSocket, MQTT callbacks) or own FreeRTOS tasks - are put into a
 * queue and delivered in the loop:
 *  - sendEvent() without payload (pMessage == nullptr) is queued automatically.
 *  - postEventText() / postEventJson() copy the payload and are always queued.
 *  - postEvent() queues an event without payload.
 *  - sendEvent() WITH payload from another context is delivered directly (as
 *    before) - the bus cannot copy unknown payloads. Use postEventText() /
 *    postEventJson() there.
 * Queued events are delivered before the next event sent in the loop (keeps
 * the order) and at the latest by processDeferredEvents() (CAppl::dispatch()).
 */
class CEventHandler : public IRegistry
{
    public:
        /// @brief Function that tells if the caller runs in the loop context (replaceable for tests).
        typedef bool (*TContextProbe)();

    private:
        int  m_nDispatchDepth = 0;      // > 0 while sendEvent() is running (also nested)
        bool m_bPurgePending  = false;  // receivers were removed during dispatch

        /// @brief Event waiting for delivery in the loop.
        struct DeferredEvent {
            void   *pSender  = nullptr;
            int     nMsg     = 0;
            int     nClass   = 0;
            uint8_t eKind    = 0;       // 0 = no payload, 1 = text, 2 = JSON text
            char   *pszData  = nullptr; // owned copy of the payload
        };
        std::deque<DeferredEvent> m_tDeferredEvents;
        unsigned int  m_nDeferredDropped     = 0;      // lost because the queue was full
        bool          m_bProcessingDeferred  = false;
        #ifdef ESP32
            std::mutex m_oDeferredLock;
        #endif
        static TContextProbe s_fnContextProbe;

        /// @brief Put an event into the queue (copies pszData). false if the queue is full.
        bool enqueue(void *pSender, int nMsg, int nClass, uint8_t eKind, const char *pszData);
        /// @brief Deliver an event to all receivers (loop context only).
        int  dispatchEvent(void *pSender, int nMsg, const void *pMessage, int nClass);

        /// @brief Remove all entries of a receiver (deferred while dispatching).
        void removeReceiverEntries(const void *pReceiver) {
            if(m_nDispatchDepth > 0) {
                // Do not change the list while it is iterated - mark only, purge later
                for(auto pEntry : m_tReceiverTable.Entries) {
                    if((const void *) pEntry->value == pReceiver) { pEntry->value = nullptr; m_bPurgePending = true; }
                }
            } else {
                m_tReceiverTable.removeIf([pReceiver](CNamedValueEntry<IMsgEventReceiver *> *pEntry) {
                    return((const void *) pEntry->value == pReceiver);
                });
            }
        }

    /*
        struct HandlerEntry {
            HandlerEntry() {}
            HandlerEntry(const char *pszName, IMsgEventReceiver *pHandler) {
                this->pszName  = pszName  ? strdup(pszName) : nullptr;
                this->pHandler = pHandler;
            }
            ~HandlerEntry() {
                if(pszName) {
                    DEBUG_INFOS("- releasing memory for name: %s",pszName);
                    // free((void*)pszName);
                    pszName = nullptr;
                }
            }   
            const char         * pszName = nullptr;
            IMsgEventReceiver  *pHandler = nullptr;
        };
        std::vector<HandlerEntry * > m_tEventReceivers;
*/

        CMultiNameUniqueValueTable<IMsgEventReceiver *> m_tReceiverTable;
        
    public: 
        CEventHandler() {}
        CEventHandler(const CEventHandler &) = delete;
        CEventHandler & operator=(const CEventHandler &) = delete;

        /// @brief Detach all receivers, so they do not access this bus later.
        ~CEventHandler();
        void registerEventReceiver(IMsgEventReceiver * pEventReceiver, const char *pszReceiverName = nullptr);

        /// @brief Remove a receiver from this bus (also safe while an event is dispatched).
        void unregisterEventReceiver(IMsgEventReceiver * pEventReceiver) {
            if(!pEventReceiver) return;
            removeReceiverEntries(pEventReceiver);
            pEventReceiver->m_oEventRegistrations.detach(this);
        }

        /// @brief true if the receiver is registered on this bus.
        bool isRegistered(IMsgEventReceiver * pEventReceiver) {
            return(pEventReceiver && m_tReceiverTable.hasValueEntry(pEventReceiver));
        }

        /// @brief IRegistry - a registered receiver is being destroyed.
        void forgetRegisteredObject(const void *pObject) override {
            removeReceiverEntries(pObject);
        }
       
        /**
         * @brief Send the message to the Message Event Receivers
         *        To avoid receiving your own message, specify the Sender (this).
         *        If static, or you want to receive your onw message, specify nullptr here...
         * 
         *        Processing:
         *        - The event message will be sent immediately to all recipients
         *
         * @param pSender nullptr (receive own messages) or the senders address to avoid receiving own messages
         * @param nMsgID use message number from msg.h - or specify owm message with MSG_USER_BASE + xxx
         * @param pMessage The message for the receiver, depending on nMsgType
         * @param nMsgType   Additional Flag for the receiving team
         */
        int sendEvent(void *pSender, int nMsgID, const void *pMessage, int nMsgType);

        /// @brief Queue an event without payload (delivered in the loop). false if the queue is full.
        bool postEvent(void *pSender, int nMsgID, int nMsgType = 0);
        /// @brief Queue an event with a copy of the text; receivers get a const char*.
        bool postEventText(void *pSender, int nMsgID, const char *pszText, int nMsgType = 0);
        /// @brief Queue an event with a copy of the JSON node; receivers get a JsonNode*.
        bool postEventJson(void *pSender, int nMsgID, CJsonNode &oNode, int nMsgType = 0);

        /// @brief Deliver the queued events (loop context only - CAppl::dispatch() calls it).
        void processDeferredEvents();
        /// @brief Number of queued events.
        size_t getDeferredEventCount();

        /// @brief true if the caller runs in the loop/setup context.
        static bool isLoopContext() { return(s_fnContextProbe()); }
        /// @brief Replace the context detection (tests). nullptr restores the default.
        static void setContextProbe(TContextProbe fnProbe);

        void dumpReceiver() {
            /*
            for(HandlerEntry * pEntry : m_tEventReceivers) {
                Serial.printf("EVH: - registered event receiver: %p - (%s)\n",
                              pEntry->pHandler,
                              pEntry->pszName ? pEntry->pszName : "-");
            }
            */
            for(CNamedValueEntry<IMsgEventReceiver *> * pEntry : this->m_tReceiverTable.Entries) {
                if(!pEntry->value) continue;
                Serial.printf("EVH: - registered event receiver: %p - (%s)\n",
                              pEntry->value,
                              pEntry->getKey());
            }
        }
    };

