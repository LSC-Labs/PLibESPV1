#pragma once

#include "Runtime.h"
#include "EventHandler.h"
#include "SimpleDelay.h"
#include "JsonNode.h"
#include "Network.h"
#include "DevelopmentHelper.h"
#include <queue>
#include <new>
#ifdef ESP32
    #include <mutex>
#endif

#ifndef WS_MAX_MESSAGE_SIZE
    #ifdef ESP32
        #define WS_MAX_MESSAGE_SIZE 32768   ///< Max. assembled message size (bytes)
    #else
        #define WS_MAX_MESSAGE_SIZE 8192    ///< Max. assembled message size (bytes)
    #endif
#endif

#ifndef WS_MAX_QUEUED_MESSAGES
    #define WS_MAX_QUEUED_MESSAGES 8        ///< Max. messages waiting for dispatch
#endif

/// @brief Owns a complete WebSocket message assembled from one or more frames.
/// @details The object is created in the async TCP context and dispatched later
/// from the application loop. The client may be gone by then, so only ClientId
/// and RemoteIP are kept as reliable data; pClient is re-resolved right before
/// dispatch and is nullptr if the client disconnected in the meantime.
class CWebSocketMessage {

    private:
        size_t                m_nFrameOffset       = 0;    // start of the current frame in the buffer

    public:
        /// @brief Total size of the assembled message (allocated bytes without terminator).
        size_t                MessageSize          = 0;
        /// @brief Zero-terminated assembled message buffer (nullptr if allocation failed).
        char                 *pSerializedMessage   = nullptr;
        /// @brief WebSocket message type, for example WS_TEXT or WS_BINARY.
        int                   MessageType          = 0;
        /// @brief Socket that received the message.
        AsyncWebSocket       *pSocket              = nullptr;
        /// @brief Client that sent the message. Only valid during dispatch, may be nullptr.
        AsyncWebSocketClient *pClient              = nullptr;
        /// @brief Id of the sending client (stable, safe to keep).
        uint32_t              ClientId             = 0;
        /// @brief Remote IP of the sending client, captured on receive.
        IPAddress             RemoteIP;

        /**
         * @brief Construct a new Web Socket Message object
         * @param pSocket   The WebSocket server instance
         * @param pClient   The client from which the message was received
         * @param nMessageSize Size of the message (or of the first frame of a fragmented message)
         * @param nType     Type of the message (e.g., text (WS_TEXT), binary (WS_BINARY), etc.)
         */
        CWebSocketMessage(AsyncWebSocket *pSocket, AsyncWebSocketClient *pClient, uint64_t nMessageSize, int nType = WS_BINARY) {
            DEBUG_FUNC_START_PARMS("size=%u,type=%d",(unsigned) nMessageSize,nType);
            this->pSocket       = pSocket;
            this->pClient       = pClient;
            this->MessageType   = nType;
            if(pClient) {
                this->ClientId  = pClient->id();
                this->RemoteIP  = pClient->remoteIP();
            }
            if(nMessageSize <= WS_MAX_MESSAGE_SIZE) {
                this->pSerializedMessage = (char *) malloc((size_t) nMessageSize + 1);
                if(this->pSerializedMessage) {
                    memset(this->pSerializedMessage,'\0',(size_t) nMessageSize + 1);
                    this->MessageSize = (size_t) nMessageSize;
                }
            }
            DEBUG_FUNC_END();
        };

        /// @brief Release the assembled message buffer.
        ~CWebSocketMessage() {
            if (pSerializedMessage != nullptr) {
                free(pSerializedMessage);
                pSerializedMessage = nullptr;
            }
        };

        CWebSocketMessage(const CWebSocketMessage&) = delete;
        CWebSocketMessage& operator=(const CWebSocketMessage&) = delete;

        /// @brief true if the message buffer could be allocated.
        bool isValid() const { return(pSerializedMessage != nullptr); }

        /**
         * @brief Extend the buffer for the next frame of a fragmented message.
         * @param nFrameLen Length of the new frame.
         * @return false if the size limit is exceeded or memory is exhausted.
         */
        bool addFrame(uint64_t nFrameLen) {
            if(!pSerializedMessage) return(false);
            if(nFrameLen > (uint64_t) (WS_MAX_MESSAGE_SIZE - MessageSize)) return(false);
            size_t nNewSize = MessageSize + (size_t) nFrameLen;
            char *pNew = (char *) realloc(pSerializedMessage, nNewSize + 1);
            if(!pNew) return(false);
            pSerializedMessage = pNew;
            memset(pSerializedMessage + MessageSize, '\0', nNewSize - MessageSize + 1);
            m_nFrameOffset = MessageSize;
            MessageSize    = nNewSize;
            return(true);
        }

        /**
         * @brief Set the message data (segment) of the current frame.
         * @param pData Pointer to the data to be stored
         * @param nIndex Offset inside the current frame
         * @param nDataLen Length of data, pData is pointing to
         * @returns this object for further processing...
         */
        CWebSocketMessage * setMessageData(const uint8_t *pData, uint64_t nIndex, size_t nDataLen) {
            if(pSerializedMessage && pData) {
                size_t nAvail = MessageSize - m_nFrameOffset;
                if(nIndex <= nAvail && nDataLen <= nAvail - (size_t) nIndex) {
                    memcpy(pSerializedMessage + m_nFrameOffset + (size_t) nIndex, pData, nDataLen);
                }
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
 * On ESP32 the socket callback runs in the AsyncTCP task, so the queue is
 * protected by a mutex. On ESP8266 callbacks run cooperatively (no lock needed).
 */
class CWebSocket : public AsyncWebSocket, public IMsgEventReceiver {
    private:
        std::queue<CWebSocketMessage *> m_tMsgQueue;
        #ifdef ESP32
            std::mutex      m_oQueueLock;   // queue is filled by AsyncTCP task, drained by loop()
        #endif
        String              m_strNeedsAuth = WS_NEEDS_AUTH; // Simple auth string with names
        CSimpleDelay        m_oClientCleanupDelay = CSimpleDelay(60000); // cleanup every minute

    public:
        WebSocketStatus Status; // The status info of the Websocket

    public:
        /// @brief Create a WebSocket endpoint and optionally register on the message bus.
        CWebSocket(const char *pszSocketName, bool bRegisterOnMsgBus = true);
        /// @brief Delete all pending messages.
        virtual ~CWebSocket();
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
        virtual bool needsAuth(String &strCommand);
        /// @brief React to application loop/status events.
        int receiveEvent(const void * pSender, int nMsgId, const void * pMessage, int nType);
        /// @brief AsyncWebSocket callback used to capture incoming frames/events.
        virtual void onWebSocketEvent(AsyncWebSocket *pServer, AsyncWebSocketClient *pClient, AwsEventType eType, void *arg, uint8_t *pData, size_t nLen);

        /// @brief Send a JSON access-denied response to a client.
        void ICACHE_FLASH_ATTR sendAccessDeniedMessage(JsonNode &oDoc,AsyncWebSocketClient *pClient);
        /// @brief Send a JSON access-denied response as reply to a queued message.
        void ICACHE_FLASH_ATTR sendAccessDeniedMessage(JsonNode &oDoc,CWebSocketMessage *pMessage);
        /// @brief Serialize and send a JSON document to one client or all clients.
		void ICACHE_FLASH_ATTR sendJsonDocMessage(JsonNode &oDoc, AsyncWebSocket *pSocket = nullptr, AsyncWebSocketClient *pClient = nullptr);
        /// @brief Serialize and send a JSON document as reply to a queued message (safe if the client is gone).
		void ICACHE_FLASH_ATTR sendJsonDocMessage(JsonNode &oDoc, CWebSocketMessage *pMessage);

    private:
        /// @brief Validate request credentials for authenticated commands.
        bool checkAuth(JsonNode &oRequestDoc, AsyncWebSocketClient *pClient);
        /// @brief Validate request credentials against a captured remote IP.
        bool checkAuth(JsonNode &oRequestDoc, const IPAddress &oRemoteIP);
        /// @brief Add an assembled message object to the dispatch queue (takes ownership).
        void addMessageToQueue(CWebSocketMessage *pMsgObj);
        /// @brief Remove the next message from the queue, nullptr if empty.
        CWebSocketMessage * popMessageFromQueue();
};
