#pragma once

#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

/**
 * Structure of a captured message
 */
struct WebSocketMessage {
    char *pSerializedMessage;
    AsyncWebSocket *pSocket;
    AsyncWebSocketClient *pClient;
    WebSocketMessage *pNextMessage;
};
struct WebSocketStatus {
    long uptime = millis();
};

#define WS_DEFAULT_NEEDS_AUTH = "getconfig,saveconfig,getbackup,restorebackup,restart,factoryreset"

class CWebSocket : public AsyncWebSocket {
    private:
        WebSocketStatus Status;
        WebSocketMessage *pMsgQueue;
        String strNeedsAuth = "WS_DEFAULT_NEEDS_AUTH";
       
    
    public:
        CWebSocket(const char *pszSocketName);
        void onWebSocketEvent(AsyncWebSocket *pServer, AsyncWebSocketClient *pClient, AwsEventType eType, void *arg, uint8_t *pData, size_t nLen);
		void dispatchMessageQueue();
		void addMessageToQueue(AsyncWebSocket *pSocket, AsyncWebSocketClient *pClient, int nMessageSize);
		void ICACHE_FLASH_ATTR sendJsonDocMessage(JsonDocument &oDoc, AsyncWebSocket *pSocket, AsyncWebSocketClient *pClient);
		
	private:
        bool inline needsAuth(String &strCommand);
        bool checkAuth(JsonDocument &oRequestDoc, AsyncWebSocketClient *pClient);
		void dispatchMessage(WebSocketMessage *pMessage);
		void ICACHE_FLASH_ATTR sendAccessDeniedMessage(JsonDocument &oDoc,AsyncWebSocketClient *pClient);
		void ICACHE_FLASH_ATTR sendStatus(AsyncWebSocket *pSocket, AsyncWebSocketClient *pClient);
		void onWiFiScanResult(int nCount);

		public:
		JsonObject createPayloadStructure(const __FlashStringHelper* pszCommand, const __FlashStringHelper* pszDataType, JsonDocument &oPayloadDoc, const char *pszData = nullptr);
		JsonObject createPayloadStructure(const char* pszCommand, const char *pszDataType, JsonDocument &oPayloadDoc,const char *pszData = nullptr);
		
};
