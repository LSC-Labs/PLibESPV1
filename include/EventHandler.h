#pragma once 
#include <Arduino.h>
#include <functional>
#include <vector>

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
        virtual int receiveEvent(const void * pSender, int nMsg, const void * pMessage, int nClass) = 0;
};  

/**
 * Event Handler Manager
 */
class CEventHandler
{
    private:
     private:
        struct HandlerEntry {
            const char         *pszName = nullptr;
            IMsgEventReceiver  *pHandler = nullptr;
        };
        std::vector<HandlerEntry> m_oEventReceivers;
        
    public: 
        void registerEventReceiver(IMsgEventReceiver * pEventReceiver, const char *pszReceiverName = nullptr);
       
        /// @brief Send the message to the Message Event Receivers
        ///        To avoid receiving your own message, specify the Sender (this).
        ///        If you want to receive your onw message, specify nullptr here...
        /// @param pSender nullptr (receive own messages) or the senders address to avoid receiving own messages
        /// @param nMsgType use message number from msg.h - or specify owm message with MSG_USER_BASE + xxx
        /// @param pMessage The message for the receiver, depending on nMsgType
        /// @param nClass   Additional Flag for the receiving team
        int sendEvent(void *pSender, int nMsg, const void *pMessage, int nClass);
       
        void dumpReceiver() {
            for(HandlerEntry oEntry : m_oEventReceivers) {
                Serial.printf("EVH: - registered event receiver: %p - (%s)\n",
                              oEntry.pHandler,
                              oEntry.pszName ? oEntry.pszName : "-");
            }
        }


    
    };

