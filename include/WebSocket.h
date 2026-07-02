#pragma once

#include "Runtime.h"
#include "EventHandler.h"
#include "SimpleDelay.h"
#include "JsonNode.h"
#include "Network.h"
#include "DevelopmentHelper.h"
#include <queue>

/// @brief Owns a complete WebSocket message assembled from one or more frames.
class CWebSocketMessage {

    public:
        /// @brief Total expected size of the assembled message.
        size_t                MessageSize          = 0;
        /// @brief Zero-terminated assembled message buffer.
        char                 *pSerializedMessage   = nullptr;
        /// @brief WebSocket message type, for example WS_TEXT or WS_BINARY.
        int                   MessageType          = 0;
        /// @brief Socket that received the message.
        AsyncWebSocket       *pSocket              = NULL;
        /// @brief Client that sent the message.
        AsyncWebSocketClient *pClient              = NULL;
        
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

        /// @brief Release the assembled message buffer.
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

/// @brief Runtime status of the WebSocket module.
struct WebSocketStatus {
    /// @brief millis() snapshot used as simple uptime/status marker.
    long uptime = millis();
};

#ifndef WS_NEEDS_AUTH
    #define WS_NEEDS_AUTH "saveconfig,getbackup,restorebackup,restart,factoryreset"
#endif

/// @brief Function pointer to register the routes
// typedef void (funcDispatchMessage)(const WebSocketMessage *pMessage);

/**
 * @brief WebSocket endpoint that queues messages and dispatches JSON commands.
 *
 * Incoming multi-frame messages are captured as CWebSocketMessage objects,
 * queued, and later dispatched from the application loop. Selected commands can
 * require authentication before they are processed.
 */
class CWebSocket : public AsyncWebSocket, public IMsgEventReceiver {
    private:
        std::queue<CWebSocketMessage *> m_tMsgQueue;
        // CWebSocketMessage * m_pMsgQueue = NULL;             // received socket messages to be dispatched
        String              m_strNeedsAuth = WS_NEEDS_AUTH; // Simple auth string with names
        CSimpleDelay        m_oClientCleanupDelay = CSimpleDelay(60000); // cleanup every minute

    public:
        WebSocketStatus Status; // The status info of the Websocket

    public:
        /// @brief Create a WebSocket endpoint and optionally register on the message bus.
        CWebSocket(const char *pszSocketName, bool bRegisterOnMsgBus = true);
        /// @brief Dispatch all queued WebSocket messages.
		void dispatchMessageQueue();
        /// @brief Dispatch one assembled WebSocket message.
		virtual bool dispatchMessage(CWebSocketMessage *pMessage);
        /// @brief Dispatch one parsed JSON WebSocket request.
        virtual bool dispatchJsonMessage(JsonNode &oJsonRequest, CWebSocketMessage *pMessage);   
        /// @brief Replace the comma-separated list of commands that require auth.
		virtual String setNeedsAuth(const String &strCommands);
        /// @brief Return the comma-separated list of commands that require auth.
        virtual String getNeedsAuth();
        /// @brief Return true if the command requires authentication.
        virtual bool inline needsAuth(String &strCommand);
        /// @brief React to application loop/status events.
        int receiveEvent(const void * pSender, int nMsgId, const void * pMessage, int nType);
        /// @brief AsyncWebSocket callback used to capture incoming frames/events.
        virtual void onWebSocketEvent(AsyncWebSocket *pServer, AsyncWebSocketClient *pClient, AwsEventType eType, void *arg, uint8_t *pData, size_t nLen);
		
        /// @brief Send a JSON access-denied response to a client.
        void ICACHE_FLASH_ATTR sendAccessDeniedMessage(JsonNode &oDoc,AsyncWebSocketClient *pClient);
        /// @brief Serialize and send a JSON document to one client or all clients.
		void ICACHE_FLASH_ATTR sendJsonDocMessage(JsonNode &oDoc, AsyncWebSocket *pSocket = nullptr, AsyncWebSocketClient *pClient = nullptr);
	
    private:
        // void addMessageToQueue(AsyncWebSocket *pSocket, AsyncWebSocketClient *pClient, int nMessageSize);
        /// @brief Validate request credentials for authenticated commands.
        bool checkAuth(JsonNode &oRequestDoc, AsyncWebSocketClient *pClient);
        /// @brief Add an assembled message object to the dispatch queue.
        void addMessageToQueue(CWebSocketMessage *pMsgObj);
};
