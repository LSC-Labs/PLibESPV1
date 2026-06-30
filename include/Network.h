#pragma once

#ifdef NATIVE_RUNTIME
#include <Runtime.h>

class IPAddress {
    String m_strAddress;

    public:
        IPAddress() {};
        IPAddress(uint8_t n1, uint8_t n2, uint8_t n3, uint8_t n4) {
            char szBuffer[16];
            snprintf(szBuffer,sizeof(szBuffer),"%u.%u.%u.%u",n1,n2,n3,n4);
            m_strAddress = szBuffer;
        }

        void fromString(const char *pszSource) {
            m_strAddress = pszSource;
        };

        String toString() const {
            return(m_strAddress);
        }

};

enum AwsEventType {
    WS_EVT_CONNECT,
    WS_EVT_DISCONNECT,
    WS_EVT_ERROR,
    WS_EVT_DATA
};

#define WS_TEXT   1
#define WS_BINARY 2

class AsyncWebSocket;
class AsyncWebSocketClient;

class AsyncWebSocketMessageBuffer {
    private:
        uint8_t *m_pData = nullptr;
        size_t   m_nSize = 0;

    public:
        AsyncWebSocketMessageBuffer(size_t nSize) : m_nSize(nSize) {
            m_pData = (uint8_t *) malloc(nSize);
        }

        ~AsyncWebSocketMessageBuffer() {
            if(m_pData) free(m_pData);
        }

        uint8_t *get() { return(m_pData); }
        size_t size() { return(m_nSize); }
};

class AsyncWebSocket {
    private:
        String m_strUrl;

    public:
        AsyncWebSocket(const char *pszUrl = "") : m_strUrl(pszUrl ? pszUrl : "") {}
        const char *url() { return(m_strUrl.c_str()); }
        void onEvent(...) {}
        void cleanupClients() {}
        AsyncWebSocketMessageBuffer *makeBuffer(size_t nSize) { return(new AsyncWebSocketMessageBuffer(nSize)); }
        void textAll(AsyncWebSocketMessageBuffer *pBuffer) { delete(pBuffer); }
};

class AsyncWebSocketClient {
    public:
        void *_tempObject = nullptr;
        uint32_t id() { return(0); }
        IPAddress remoteIP() { return(IPAddress()); }
        void text(AsyncWebSocketMessageBuffer *pBuffer) { delete(pBuffer); }
};

struct AwsFrameInfo {
    bool final = true;
    uint64_t index = 0;
    uint8_t num = 0;
    uint64_t len = 0;
    uint8_t opcode = WS_BINARY;
};


#else
    #if defined(ESP32) || defined(LIBRETINY)
    // #ifdef CONFIG_IDF_TARGET_ESP32
        #include <AsyncTCP.h>
        #include <WiFi.h>
    #elif defined(ESP8266)
        #include <ESP8266WiFi.h>
        #include <ESPAsyncTCP.h>
        #include <ESP8266mDNS.h>
    #elif defined(TARGET_RP2040) || defined(TARGET_RP2350) || defined(PICO_RP2040) || defined(PICO_RP2350)
        #include <RPAsyncTCP.h>
        #include <WiFi.h>
    #endif
    #include <ESPAsyncWebServer.h>
#endif
