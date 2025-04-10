#pragma once 

#include <functional>
#include <vector>

#define EVENT_MSG_RESULT_OK               0
#define EVENT_MSG_RESULT_WARN             1
#define EVENT_MSG_RESULT_ERROR            2
#define EVENT_MSG_RESULT_STOP_PROCESSING 99


class IMsgEventReceiver
{
    public:
        virtual int receiveEvent(void *pSender, int nMsgType, const void *pMessage, int nClass) = 0;
};  

class CEventHandler
{
    private:
        std::vector<IMsgEventReceiver*> m_oEventReceivers;
        
    public: 
        void registerEventReceiver(IMsgEventReceiver * pEventReceiver) {
            m_oEventReceivers.push_back(pEventReceiver);
        }
       
        /// @brief Send the message to the Message Event Receivers
        ///        To avoid receiving your own message, specify the Sender (this).
        ///        If you want to receive your onw message, specify nullptr here...
        /// @param pSender nullptr (receive own messages) or the senders address to avoid receiving own messages
        /// @param nMsgType use message number from msg.h - or specify owm message with MSG_USER_BASE + xxx
        /// @param pMessage The message for the receiver, depending on nMsgType
        /// @param nClass   Additional Flag for the receiving team
        int sendEvent(void *pSender, int nMsgType, const void *pMessage, int nClass);
};
