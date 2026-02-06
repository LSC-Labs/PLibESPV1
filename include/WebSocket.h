#pragma once

#include <Network.h>
#include <DevelopmentHelper.h>
#include <ArduinoJson.h>
#include <EventHandler.h>
#include <SimpleDelay.h>
#include <queue>

/**
 * Encapsulates a captured message
 */
class CWebSocketMessage {

    public:
        size_t                MessageSize          = 0;         // total size of this message
        char                 *pSerializedMessage   = nullptr;   // serialized message content
        int                   MessageType          = 0;         // e.g., text (WS_TEXT), binary, etc.
        AsyncWebSocket       *pSocket              = NULL;      // Socket, receiving the message
        AsyncWebSocketClient *pClient              = NULL;      // Client, sending the message
        
        /**
         * @brief Construct a new Web Socket Message object
         * @param pSocket   The WebSocket server instance
         * @param pClient   The client from which the message was received
         * @param nMessageSize Size of the message
         * @param nType     Type of the message (e.g., text (WS_TEXT), binary (WS_BINARY), etc.)
         */
        CWebSocketMessage(AsyncWebSocket *pSocket, AsyncWebSocketClient *pClient, size_t nMessageSize, int nType = WS_BINARY) {
            DEBUG_FUNC_START_PARMS("size=%d,type=%d",nMessageSize,nType);
            this->pSocket       = pSocket;
            this->pClient       = pClient;
            this->MessageSize  = nMessageSize;
            this->MessageType   = nType;
            this->pSerializedMessage = (char *) malloc(nMessageSize + 1);
            memset(this->pSerializedMessage,'\0',nMessageSize + 1);
            DEBUG_FUNC_END();
        };

        ~CWebSocketMessage() {
            if (pSerializedMessage != NULL) {
                free(pSerializedMessage);
                pSerializedMessage = NULL;
            }
        };

        /**
         * @brief Set the message data (segment).
         * @param pData Pointer to the data to be stored
         * @param nIndex Index - starting point to write into the final serialized message
         * @param nDataLen Length of data, pData is pointing to
         * @returns this object for further processing...
         */
        CWebSocketMessage * setMessageData(uint8_t *pData, uint64_t nIndex, size_t nDataLen) {
            if((nIndex + nDataLen) <= MessageSize) {
                memcpy(pSerializedMessage + nIndex,pData,nDataLen);
            }
            return(this);
        }
        
};

struct WebSocketStatus {
    long uptime = millis();
};

#ifndef WS_NEEDS_AUTH
    #define WS_NEEDS_AUTH "saveconfig,getbackup,restorebackup,restart,factoryreset"
#endif

/// @brief Function pointer to register the routes
// typedef void (funcDispatchMessage)(const WebSocketMessage *pMessage);

class CWebSocket : public AsyncWebSocket, public IMsgEventReceiver {
    private:
        std::queue<CWebSocketMessage *> m_tMsgQueue;
        // CWebSocketMessage * m_pMsgQueue = NULL;             // received socket messages to be dispatched
        String              m_strNeedsAuth = WS_NEEDS_AUTH; // Simple auth string with names
        CSimpleDelay        m_oClientCleanupDelay = CSimpleDelay(60000); // cleanup every minute

    public:
        WebSocketStatus Status; // The status info of the Websocket

    public:
        CWebSocket(const char *pszSocketName, bool bRegisterOnMsgBus = true);
		void dispatchMessageQueue();
		virtual bool dispatchMessage(CWebSocketMessage *pMessage);
        virtual bool dispatchJsonMessage(JsonDocument &oJsonRequest, CWebSocketMessage *pMessage);   
		virtual String setNeedsAuth(const String &strCommands);
        virtual String getNeedsAuth();
        virtual bool inline needsAuth(String &strCommand);
        int receiveEvent(const void * pSender, int nMsgId, const void * pMessage, int nType);
        virtual void onWebSocketEvent(AsyncWebSocket *pServer, AsyncWebSocketClient *pClient, AwsEventType eType, void *arg, uint8_t *pData, size_t nLen);
		
        void ICACHE_FLASH_ATTR sendAccessDeniedMessage(JsonDocument &oDoc,AsyncWebSocketClient *pClient);
		void ICACHE_FLASH_ATTR sendJsonDocMessage(JsonDocument &oDoc, AsyncWebSocket *pSocket = nullptr, AsyncWebSocketClient *pClient = nullptr);
	
    private:
        // void addMessageToQueue(AsyncWebSocket *pSocket, AsyncWebSocketClient *pClient, int nMessageSize);
        bool checkAuth(JsonDocument &oRequestDoc, AsyncWebSocketClient *pClient);
        void addMessageToQueue(CWebSocketMessage *pMsgObj);
        void addMessageToQueue(AsyncWebSocket *pServer, AsyncWebSocketClient *pClient, uint8_t * pData, size_t nDataLen, int nDataType);
};
