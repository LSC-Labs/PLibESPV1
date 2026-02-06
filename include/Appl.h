#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <ModuleInterface.h>
#include <Logging.h>
#include <Vars.h>
#include <DevelopmentHelper.h>
#include <JsonHelper.h>


/**
 * @file Appl.h
 * @author Peter L. (LSC Labs)
 * @brief Application Main Class
 * @version 1.0
 */


 /**
  * Macros for Application Logging
  * Use this macros, to log messages, so it is possible to undef them all at once,
  * if needed (for example in production code)
  */
#ifdef NO_LOGGING
    #define ApplLogInfo(oData)          NULL_FUNCTION
    #define ApplLogWarn(oData)          NULL_FUNCTION
    #define ApplLogVerbose(oData)       NULL_FUNCTION
    #define ApplLogError(oData)         NULL_FUNCTION

    #define ApplLogInfoWithParms(str,...)     NULL_FUNCTION
    #define ApplLogVerboseWithParms(str,...)  NULL_FUNCTION
    #define ApplLogWarnWithParms(str,...)     NULL_FUNCTION
    #define ApplLogErrorWithParms(str,...)    NULL_FUNCTION

    #define ApplLogTrace(oData)                 NULL_FUNCTION
    #define ApplLogTraceWithParms(str,...)      NULL_FUNCTION
#else
    #define ApplLogInfo(oData)          Appl.Log.log("I",oData)
    #define ApplLogWarn(oData)          Appl.Log.log("W",oData)
    #define ApplLogVerbose(oData)       Appl.Log.log("V",oData)
    #define ApplLogError(oData)         Appl.Log.log("E",oData)

    #define ApplLogInfoWithParms(str,...)     Appl.Log.log("I",str,__VA_ARGS__)
    #define ApplLogVerboseWithParms(str,...)  Appl.Log.log("V",str,__VA_ARGS__)
    #define ApplLogWarnWithParms(str,...)     Appl.Log.log("W",str,__VA_ARGS__)
    #define ApplLogErrorWithParms(str,...)    Appl.Log.log("E",str,__VA_ARGS__)
#endif

// Enable Trace Logging, if TRACE is defined - Default is NO trace...
#ifdef TRACE
    #define ApplLogTrace(oData)                 Appl.Log.log("T",oData)
    #define ApplLogTraceWithParms(str,...)      Appl.Log.log("T",str,__VA_ARGS__)
#else
    #define ApplLogTraceWithParms(str,...)      NULL_FUNCTION
    #define ApplLogTrace(oData)                 NULL_FUNCTION
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
#ifndef HIDDEN_PASSWORD_MASK
    #define HIDDEN_PASSWORD_MASK            "********"
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
    bool m_isRebootPending = false;
    // CSimpleDelay m_oRebootDelay;
    unsigned long m_ulLastStatusUpdate = 0;
    time_t  m_oRawTime;   
    char m_szISODateTime[32];   // Buffer for ISO - Time
    char m_szCurTime[16];       // Buffer for Time part of ISO
    char m_szCurDate[16];       // Buffer for Date part of ISO

    public:
        String AppName;
        String AppVersion;
        const unsigned long StartTime = millis();

    public:

        // CSystemStatusHandler SystemStatus;
        CVarTable      Config;
        CEventHandler  MsgBus;
        CEventLogger   Log;
        void registerModule(const char *pszModuleName, IModule * pModule);
        void writeConfigTo(JsonObject &oNode,bool bHideCritical) override;

        void readConfigFrom(JsonDocument &oDoc);
        void readConfigFrom(JsonObject &oNode) override;
        // Flashstring is not supported by LittleFS !

        bool readConfigFrom(const char *pszFileName, int nJsonDocSize = JSON_CONFIG_DOC_DEFAULT_SIZE);   // Load config from Files
        bool saveConfig(const char *pszFileName = JSON_CONFIG_DEFAULT_NAME,  int nJsonDocSize = JSON_CONFIG_DOC_DEFAULT_SIZE);   // Load config from Files
       
        void writeStatusTo(JsonDocument &oDoc);
        void writeStatusTo(JsonObject &oNode) override;
        void writeSystemStatusTo(JsonDocument &oDoc);
        void writeSystemStatusTo(JsonObject &oNode);
        String getUpTime();
        time_t getNativeTime();
        const char * getISODateTime();
        const char * getISODate();
        const char * getISOTime();
        String getDeviceName() { return(m_oCfg.strDeviceName);}
        void setDeviceName(const char *pszName) { m_oCfg.strDeviceName = String(pszName); }  
        void setDeviceName(String strName) { m_oCfg.strDeviceName = strName; } 
        void setDevicePwd(const char *pszName) { m_oCfg.strDevicePwd = String(pszName); }  
        void setDevicePwd(String strName) { m_oCfg.strDevicePwd = strName; }      
        String getDevicePwd() { return(m_oCfg.strDevicePwd);}

    public:
        CAppl();
        void dispatch(const void *pMsg = nullptr, int nMsgClass = 0);
        int receiveEvent(const void * pSender, int nMsgType, const void * pMessage, int nClass);
        void init(const char *strAppName, const char *strAppVersion);
        void sayHello();
        void printDiag();
        void reboot(int nDelayMillis = 2000, bool bForce = false);
        
};

extern CAppl Appl;
