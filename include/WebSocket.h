#pragma once

#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <EventHandler.h>

/**
 * Structure of a captured message
 */
struct WebSocketMessage {
    char *pSerializedMessage        = NULL;
    AsyncWebSocket *pSocket         = NULL;
    AsyncWebSocketClient *pClient   = NULL;
    WebSocketMessage *pNextMessage  = NULL;
};
struct WebSocketStatus {
    long uptime = millis();
};

#ifndef WS_NEEDS_AUTH
    #define WS_NEEDS_AUTH "saveconfig,getbackup,restorebackup,restart,factoryreset"
#endif

/// @brief Function pointer to register the routes
typedef void (funcDispatchMessage)(const WebSocketMessage *pMessage);

class CWebSocket : public AsyncWebSocket, public IMsgEventReceiver {
    private:
        WebSocketStatus Status;
        WebSocketMessage *pMsgQueue = NULL;
        String strNeedsAuth = WS_NEEDS_AUTH;
    
    public:
        CWebSocket(const char *pszSocketName, bool bDontRegisterOnMsgBus = false);
		void dispatchMessageQueue(funcDispatchMessage pFuncDispatchMessage = nullptr);
		void ICACHE_FLASH_ATTR sendJsonDocMessage(JsonDocument &oDoc, AsyncWebSocket *pSocket = nullptr, AsyncWebSocketClient *pClient = nullptr);
		String setNeedsAuth(const String &strCommands);
        String getNeedsAuth();
        int receiveEvent(const void * pSender, int nMsgId, const void * pMessage, int nType);
        
	private:
        void onWebSocketEvent(AsyncWebSocket *pServer, AsyncWebSocketClient *pClient, AwsEventType eType, void *arg, uint8_t *pData, size_t nLen);
		void addMessageToQueue(AsyncWebSocket *pSocket, AsyncWebSocketClient *pClient, int nMessageSize);
        bool inline needsAuth(String &strCommand);
        bool checkAuth(JsonDocument &oRequestDoc, AsyncWebSocketClient *pClient);
		bool dispatchMessage(WebSocketMessage *pMessage);
		void ICACHE_FLASH_ATTR sendAccessDeniedMessage(JsonDocument &oDoc,AsyncWebSocketClient *pClient);
		void ICACHE_FLASH_ATTR sendStatus(AsyncWebSocket *pSocket, AsyncWebSocketClient *pClient);

	public:
		JsonObject createPayloadStructure(const __FlashStringHelper* pszCommand, const __FlashStringHelper* pszDataType, JsonDocument &oPayloadDoc, const char *pszData = nullptr);
		JsonObject createPayloadStructure(const char* pszCommand, const char *pszDataType, JsonDocument &oPayloadDoc,const char *pszData = nullptr);
		
};
