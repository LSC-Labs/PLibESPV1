#pragma once 
#include <Arduino.h>
#include <functional>
#include <vector>
#include <queue>
#include <NamedValueTable.h>
#include <DevelopmentHelper.h>

#define EVENT_MSG_CALL_AGAIN_WHEN_ALL_OK -9
#define EVENT_MSG_RESULT_OK               0
#define EVENT_MSG_RESULT_WARN             1
#define EVENT_MSG_RESULT_ERROR            2
#define EVENT_MSG_RESULT_STOP_PROCESSING 99


/**
 * Interface for Message Event Receivers
 */
class IMsgEventReceiver
{
    public:
        virtual int receiveEvent(const void * pSender, int nMsg, const void * pMessage, int nMsgInfo) = 0;
};  

/**
 * Event Handler Manager
 */
class CEventHandler
{
    private:
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
        ~CEventHandler() {
            // for(HandlerEntry * pEntry : m_tEventReceivers) delete(pEntry);
        }
        void registerEventReceiver(IMsgEventReceiver * pEventReceiver, const char *pszReceiverName = nullptr);
       
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

        void dumpReceiver() {
            /*
            for(HandlerEntry * pEntry : m_tEventReceivers) {
                Serial.printf("EVH: - registered event receiver: %p - (%s)\n",
                              pEntry->pHandler,
                              pEntry->pszName ? pEntry->pszName : "-");
            }
            */
            for(CNamedValueEntry<IMsgEventReceiver *> * pEntry : this->m_tReceiverTable.Entries) {
                Serial.printf("EVH: - registered event receiver: %p - (%s)\n",
                              pEntry->value,
                              pEntry->getKey());
            }
        }
    };

