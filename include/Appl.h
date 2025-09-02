#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <EventHandler.h>
#include <StatusHandler.h>
#include <Logging.h>
#include <Vars.h>
#include <Msgs.h>
#include <DevelopmentHelper.h>
#include <JsonHelper.h>



#define ApplLogInfo(oData)          Appl.Log.log("I",oData)
#define ApplLogWarn(oData)          Appl.Log.log("W",oData)
#define ApplLogVerbose(oData)       Appl.Log.log("V",oData)
#define ApplLogError(oData)         Appl.Log.log("E",oData)

#define ApplLogInfoWithParms(str,...)     Appl.Log.log("I",str,__VA_ARGS__)
#define ApplLogVerboseWithParms(str,...)  Appl.Log.log("V",str,__VA_ARGS__)
#define ApplLogWarnWithParms(str,...)     Appl.Log.log("W",str,__VA_ARGS__)
#define ApplLogErrorWithParms(str,...)    Appl.Log.log("E",str,__VA_ARGS__)


#ifdef TRACE
    #define ApplLogTrace(oData)                 Appl.Log.log("T",oData)
    #define ApplLogTraceWithParms(str,...)      Appl.Log.log("T",str,__VA_ARGS__)
#else
    #define ApplLogTraceWithParms(str,...)      ((void*)0)
    #define ApplLogTrace(oData)                 ((void*)0)
#endif

// Default sizes for Json Status Documents (needed for JSON < 7)
#ifndef JSON_STATUS_DOC_DEFAULT_SIZE
    #define JSON_STATUS_DOC_DEFAULT_SIZE 2048
#endif
// Default sizes for Json Config Documents (needed for JSON < 7)
#ifndef JSON_CONFIG_DOC_DEFAULT_SIZE
    #define JSON_CONFIG_DOC_DEFAULT_SIZE  2048 
#endif


#ifndef JSON_CONFIG_DEFAULT_NAME
    #define JSON_CONFIG_DEFAULT_NAME        "/config.json"
#endif

#ifndef DEFAULT_DEVICE_NAME
    #define DEFAULT_DEVICE_NAME             "LSC-Device"
#endif
#ifndef DEFAULT_DEVICE_PWD
    #define DEFAULT_DEVICE_PWD              "admin"
#endif


/// @brief Configuration structure of CAppl object
struct ApplConfig {
    bool bLogToSerial       = true;             // Log to serial port
    bool bTraceMode         = false;            // Trace Mode on / off
    String strDeviceName    = DEFAULT_DEVICE_NAME;     // Devicename (hostname) of the device
    String strDevicePwd     = DEFAULT_DEVICE_PWD;      // Default device password

#ifdef DEBUG_LSC_FRONTEND
    bool bDebugFrontend     = true;    // Debug Mode for the HTML Frontend
#else
    bool bDebugFrontend     = false;    // Debug Mode for the HTML Frontend
#endif
};

class CAppl : public CConfigHandler, public CStatusHandler, IMsgEventReceiver {
    ApplConfig m_oCfg;   
    JsonDocument *m_pStatusDoc = nullptr;
    // JSON_DOC(m_oStatusDoc,JSON_STATUS_DOC_DEFAULT_SIZE);
    bool m_bStatusChanged = true;
    unsigned long m_ulLastStatusUpdate = 0;

    public:
        String AppName;
        String AppVersion;
        const unsigned long StartTime = millis();

    public:

        // CSystemStatusHandler SystemStatus;
        CVarTable      Config;
        CEventHandler  MsgBus;
        CEventLogger   Log;
        void writeConfigTo(JsonObject &oNode,bool bHideCritical) override;

        void readConfigFrom(JsonDocument &oDoc);
        void readConfigFrom(JsonObject &oNode) override;
        // Flashstring is not supported by LittleFS !
        #if ARDUINOJSON_VERSION_MAJOR < 7
            bool readConfigFrom(const char *pszFileName,                int nJsonDocSize = JSON_CONFIG_DOC_DEFAULT_SIZE);   // Load config from Files
            bool saveConfig(const char *pszFileName = JSON_CONFIG_DEFAULT_NAME,  int nJsonDocSize = JSON_CONFIG_DOC_DEFAULT_SIZE);   // Load config from Files
        #else
            bool readConfigFrom(const char *pszFileName = JSON_CONFIG_DEFAULT_NAME);   // Load config from Files
            bool saveConfig(const char *pszFileName = JSON_CONFIG_DEFAULT_NAME);   // Load config from Files
        #endif
       
        void writeStatusTo(JsonDocument &oDoc);
        void writeStatusTo(JsonObject &oNode) override;
        void writeSystemStatusTo(JsonDocument &oDoc);
        void writeSystemStatusTo(JsonObject &oNode);
        String getUpTime();
        String getDeviceName() { return(m_oCfg.strDeviceName);}
        void setDeviceName(const char *pszName) { m_oCfg.strDeviceName = String(pszName); }  
        void setDeviceName(String strName) { m_oCfg.strDeviceName = strName; } 
        void setDevicePwd(const char *pszName) { m_oCfg.strDevicePwd = String(pszName); }  
        void setDevicePwd(String strName) { m_oCfg.strDevicePwd = strName; }      
        String getDevicePwd() { return(m_oCfg.strDevicePwd);}

    public:
        CAppl();
        int receiveEvent(const void * pSender, int nMsgType, const void * pMessage, int nClass);
        void init(const char *strAppName, const char *strAppVersion);
        void sayHello();
        void printDiag();
        void reboot(int nDelayMillis = 2000, bool bForce = false);
        
};

extern CAppl Appl;
