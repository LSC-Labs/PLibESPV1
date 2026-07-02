#pragma once
#include <Runtime.h>
#include <SysStatus.h>
#include <JsonNode.h>
#include <ModuleInterface.h>
#include <Logging.h>
#include <Vars.h>
#include <DevelopmentHelper.h>



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
#ifndef STATUS_DOC_SIZE
    #define STATUS_DOC_SIZE 1024
#endif

#ifndef SYSSTATUS_DOC_SIZE
    #define SYSSTATUS_DOC_SIZE 512
#endif



// Default sizes for Json Config Documents (needed for JSON < 7)
#ifndef JSON_CONFIG_DOC_DEFAULT_SIZE
    #define JSON_CONFIG_DOC_DEFAULT_SIZE  2048 
#endif

#ifndef JSON_CONFIG_DEFAULT_NAME
    #define JSON_CONFIG_DEFAULT_FILE        "/default.json"
#endif

#ifndef JSON_APPL_CONFIG_FILE
    #define JSON_APPL_CONFIG_FILE           "/config.json"
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


/// @brief Runtime configuration of the global CAppl object.
struct ApplConfig {
    /// @brief true to attach a serial log writer during application init.
    bool    bLogToSerial     = true;
    /// @brief true when trace mode is enabled through configuration.
    bool    bTraceMode       = false;
    /// @brief Device name/hostname used by network-facing modules.
    String  strDeviceName    = DEFAULT_DEVICE_NAME;
    /// @brief Device password used by authentication-aware modules.
    String  strDevicePwd     = DEFAULT_DEVICE_PWD;

#ifdef DEBUG_LSC_FRONTEND
    /// @brief true to expose frontend debug information.
    bool bDebugFrontend     = true;    // Debug Mode for the HTML Frontend
#else
    /// @brief true to expose frontend debug information.
    bool bDebugFrontend     = false;    // Debug Mode for the HTML Frontend
#endif
};

/**
 * @brief Central application hub for modules, configuration, status and events.
 *
 * CAppl owns the message bus, central logger, variable table and reusable status
 * documents. Modules register here to participate in config loading/saving,
 * status reporting and application lifecycle events.
 */
class CAppl : public CConfigHandler, public CStatusHandler, IMsgEventReceiver {
    ApplConfig  m_oCfg;   
    CSysStatus  m_oSystemStatus;
    JsonNode    m_oStatus;
    JsonNode    m_oSysStatus;
    String      m_strStatus;
    String      m_strSysStatus;
    String      m_strIsoDateTime;

    
    // JSON_DOC(m_oStatusDoc,JSON_STATUS_DOC_DEFAULT_SIZE);
    bool m_bStatusChanged   = true;
    bool m_isRebootPending  = false;
    // CSimpleDelay m_oRebootDelay;
    unsigned long   m_ulLastStatusUpdate = 0;
    time_t          m_oRawTime;   
    char m_szISODateTime[64]  = {0};    // Buffer for ISO - Time
    char m_szCurTime[16]      = {0};    // Buffer for Time part of ISO
    char m_szCurDate[16]      = {0};    // Buffer for Date part of ISO
    char m_szUptimeBuffer[20] = {0};    // Buffer for Uptime Status

    public:
        /// @brief Human-readable application/firmware name.
        String AppName;
        /// @brief Human-readable application/firmware version.
        String AppVersion;
        /// @brief millis() value captured when the global CAppl object is created.
        const unsigned long StartTime = millis();

    public:

        // CSystemStatusHandler SystemStatus;
        /// @brief Application-local variable table, exported as "cfg".
        CVarTable      Config;
        /// @brief Application message bus used by modules and log writers.
        CEventHandler  MsgBus;
        /// @brief Event-based application logger.
        CEventLogger   Log;

        /**
         * @brief Register a module with configuration, status and event handling.
         * @param pszModuleName Name used for config/status sections and event logging.
         * @param pModule Module instance implementing IModule.
         */
        void registerModule(const char *pszModuleName, IModule * pModule);

        /**
         * @brief Write application and registered-module configuration.
         * @param oNode Target JSON node.
         * @param bHideCritical true to mask passwords/secrets.
         */
        void writeConfigTo(JsonNode &oNode,bool bHideCritical) override;

        /**
         * @brief Read application and registered-module settings from JSON.
         * @param oNode Source configuration node.
         */
        void readConfigFrom(JsonNode &oNode) override;
        // Flashstring is not supported by LittleFS !

        /**
         * @brief Read a configuration file and load settings into all modules.
         * @param pszFileName Config file path, or nullptr for JSON_APPL_CONFIG_FILE.
         * @param nJsonDocSize Legacy JSON document size for older ArduinoJson versions.
         * @return true if a configuration file was loaded successfully.
         */
        bool readConfigFrom(const char *pszFileName, int nJsonDocSize = JSON_CONFIG_DOC_DEFAULT_SIZE);   // Load config from Files

        /**
         * @brief Migrate legacy configuration keys into their current locations.
         * @param oDoc Root configuration node to migrate in place.
         */
        void migrateConfig(JsonNode &oDoc);

        /**
         * @brief Persist the current configuration to the file system.
         * @param pszFileName Config file path.
         * @param nJsonDocSize Legacy JSON document size for older ArduinoJson versions.
         * @return true if the configuration was written successfully.
         */
        bool saveConfig(const char *pszFileName = JSON_APPL_CONFIG_FILE,  int nJsonDocSize = JSON_CONFIG_DOC_DEFAULT_SIZE);   // Load config from Files

        /**
         * @brief Build and return the reusable application status node.
         * @param nLevel Requested status detail level.
         * @return Pointer owned by CAppl; do not delete it.
         */
        JsonNode    * getStatus(int nLevel = STATUS_LEVEL_INFO);

        /**
         * @brief Return the current application status serialized as JSON text.
         */
        const char  * getStatusAsText(int nLevel = STATUS_LEVEL_INFO);

        /**
         * @brief Return the compact state view of this device.
         */
        JsonNode    * getState();

        /**
         * @brief Return the compact device state serialized as JSON text.
         */
        const char  * getStateAsText();

        /**
         * @brief Write application metadata and module status into a JSON node.
         */
        void writeStatusTo(JsonNode & oNode, int nStatusLevel) override;

        /**
         * @brief Write lower-level system diagnostics into a JSON node.
         */
        void writeSystemStatusTo(JsonNode &oNode);

        /**
         * @brief Return the native system time.
         */
        time_t getNativeTime();

        /**
         * @brief Return application uptime as HH:MM:SS.
         */
        const char * getUpTime();

        /**
         * @brief Return current local date/time as YYYY-MM-DDTHH:MM:SS.
         */
        const char * getISODateTime();

        /**
         * @brief Return current local date as YYYY-MM-DD.
         */
        const char * getISODate();

        /**
         * @brief Return current local time as HH:MM:SS.
         */
        const char * getISOTime();

        /**
         * @brief Return the stable chip/device identifier from system status.
         */
        const char * getDeviceID();

        /// @brief Return the configured device name.
        String getDeviceName() { return(m_oCfg.strDeviceName);}
        /// @brief Set the configured device name from a C string.
        void setDeviceName(const char *pszName) { m_oCfg.strDeviceName = String(pszName); }  
        /// @brief Set the configured device name from a String.
        void setDeviceName(String strName) { m_oCfg.strDeviceName = strName; } 
        /// @brief Set the configured device password from a C string.
        void setDevicePwd(const char *pszName) { m_oCfg.strDevicePwd = String(pszName); }  
        /// @brief Set the configured device password from a String.
        void setDevicePwd(String strName) { m_oCfg.strDevicePwd = strName; }      
        /// @brief Return the configured device password.
        String getDevicePwd() { return(m_oCfg.strDevicePwd);}

    public:
        /**
         * @brief Create the global application hub.
         */
        CAppl();

        /**
         * @brief Dispatch a periodic loop message to all registered receivers.
         */
        void dispatch(int nMsgType = 0,const void *pMsg = nullptr);

        /**
         * @brief Handle application-level events such as reboot requests.
         */
        int receiveEvent(const void * pSender, int nMsgType, const void * pMessage, int nClass);

        /**
         * @brief Initialize the application metadata and announce startup.
         */
        void init(const char *strAppName, const char *strAppVersion);

        /**
         * @brief Publish the application-started event after initialization finished.
         */
        void start(void * pData = nullptr, int nType = 0 );

        /**
         * @brief Print a short application banner to Serial.
         */
        void sayHello();

        /**
         * @brief Print flash/heap diagnostics and verify flash size configuration.
         */
        void printDiag();

        /**
         * @brief Send shutdown events and restart the ESP after the given delay.
         */
        void reboot(int nDelayMillis = 2000, bool bForce = false);
        
};

/// @brief Global application instance used by modules and logging macros.
extern CAppl Appl;
