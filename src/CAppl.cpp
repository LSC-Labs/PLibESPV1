/**
 * @file CAppl.cpp
 * @brief Application Framework Implementation
 * 
 * This file implements the core application framework for an ESP32-based smart device.
 * It provides:
 * - Application initialization and lifecycle management
 * - Message/Event bus dispatching and handling
 * - Module registration and management
 * - Configuration file I/O (JSON-based)
 * - Status reporting and monitoring
 * - System time and date utilities
 * - Diagnostic and system information functions
 * 
 * The CAppl class acts as the central hub for managing modules, handling configuration,
 * coordinating message passing, and managing system reboot operations.
 * 
 * @dependencies
 * - Appl.h (CAppl class definition)
 * - FileSystem.h (CFS class for file operations)
 * - SysStatus.h (CSysStatus class for system diagnostics)
 * - LSCUtils.h (Utility functions)
 * - Arduino JSON library
 * 
 * @note This implementation is designed for ESP32 microcontroller environments
 * @see CAppl class for detailed API documentation
 */
#ifndef DEBUG_LSC_APPL
	#undef DEBUGINFOS
#endif

#ifndef LSC_APPL_SERIAL_SPEED
	#define LSC_APPL_SERIAL_SPEED 115200
#endif

#include <Appl.h>
#include <new>
#include <FileSystem.h>
#include <SysStatus.h>
#include <LSCUtils.h>

#pragma region Constructor / init and Message Dispatching

/**
 * @brief Create the global application hub.
 *
 * The constructor initializes serial logging when enabled, wires the event log
 * to the message bus, registers the application itself as event receiver and
 * exposes the local variable table under the "cfg" config section.
 */
CAppl::CAppl() {
	if(LSC_APPL_SERIAL_SPEED > 0) Serial.begin(LSC_APPL_SERIAL_SPEED);
    Log.setEventHandler(&MsgBus);
	MsgBus.registerEventReceiver(this,"Appl");
	addConfigHandler("cfg",&Config);
}  

/**
 * @brief Initialize the application metadata and announce startup.
 * @param strAppName Name of the firmware/application.
 * @param strAppVersion Version string of the firmware/application.
 */
void CAppl::init(const char *strAppName, const char *strAppVersion) {
    AppName 	= strAppName;
    AppVersion  = strAppVersion;
	
    m_bInitialized = true;
    updateSerialLogWriter();        // also called after reading the config

	MsgBus.sendEvent(this,MSG_APPL_STARTING,nullptr,0);
	randomSeed(millis());
	MsgBus.sendEvent(this,MSG_APPL_INITIALIZED,nullptr,0);
}

/**
 * @brief Publish the application-started event after initialization finished.
 * @param pMsg Optional payload passed to registered event receivers.
 * @param nType Optional payload type/class.
 */
void CAppl::start(void * pMsg, int nType) {
	MsgBus.sendEvent(this,MSG_APPL_STARTED,pMsg,nType);
}


/**
 * @brief Dispatch a periodic loop message to all registered event receivers.
 * @param nMsgType Optional message class/type.
 * @param pMsg Optional message payload.
 */
void CAppl::dispatch(int nMsgType,const void *pMsg) {
	MsgBus.processDeferredEvents();	// events/log entries from other contexts (async callbacks)
	this->MsgBus.sendEvent(this,MSG_APPL_LOOP,pMsg,nMsgType);
	// Status copy for async callers (web server) - built here in the loop context
	if(!m_oStatusSnapshotDelay.isActive() || m_oStatusSnapshotDelay.isDone()) {
		updateStatusSnapshot();
		m_oStatusSnapshotDelay.restartWith(APPL_STATUS_SNAPSHOT_INTERVAL);
	}
	// System status: only on request (first one is built in advance, so the first request has data)
	if((m_bSystemStatusRequested || m_strSystemStatusSnapshot.length() == 0) &&
	   (!m_oSystemStatusSnapshotDelay.isActive() || m_oSystemStatusSnapshotDelay.isDone())) {
		m_bSystemStatusRequested = false;
		updateSystemStatusSnapshot();
		m_oSystemStatusSnapshotDelay.restartWith(APPL_SYSSTATUS_SNAPSHOT_INTERVAL);
	}
}

/**
 * @brief Registers or removes the serial log writer according to the config.
 *
 * Called by init() and after reading the config, so "logToSerial" also works
 * when the config is read after init(). Only one writer is created.
 */
void CAppl::updateSerialLogWriter() {
	if(!m_bInitialized) return;				// init() decides first
	if(m_oCfg.bLogToSerial && !m_pSerialLogWriter) {
		m_pSerialLogWriter = new (std::nothrow) CSerialLogWriter();
		if(m_pSerialLogWriter) MsgBus.registerEventReceiver(m_pSerialLogWriter,"SerialLogWriter");
	} else if(!m_oCfg.bLogToSerial && m_pSerialLogWriter) {
		delete m_pSerialLogWriter;			// unregisters itself from the bus
		m_pSerialLogWriter = nullptr;
	}
}

/**
 * @brief Handle application-level events before other modules continue.
 *
 * Reboot requests are delayed by one additional bus pass so modules can see the
 * shutdown event before ESP.restart() is executed.
 */
int CAppl::receiveEvent(const void * pSender, int nMsg, const void * pMessage, int nClass) {
    int nResult = EVENT_MSG_RESULT_OK;
	switch(nMsg) {
        case MSG_REBOOT_REQUEST:
			// As Appl is the first in Message queue, the rest will NOT be informed.
			// So send the message again to inform the rest, but dont do this,
			// if reboot is already pending, to avoid loops... (multi CAppl instances)
			if(!m_isRebootPending) {
				m_isRebootPending = true;
				nResult = EVENT_MSG_CALL_AGAIN_WHEN_ALL_OK;
			} else {
				reboot(nClass > 0 ? nClass : 3000,true);
			}
            break;
    }
	return(nResult);
}

#pragma endregion Constructor / init and Message Dispatching

#pragma region Module Registration and Rebooting

/**
 * @brief Register a module with configuration, status and event handling.
 * @param pszModuleName Name used for config/status sections and event logging.
 * @param pModule Pointer to the Module Interface
 */
void CAppl::registerModule(const char * pszModuleName, IModule * pModule) {
	DEBUG_FUNC_START_PARMS("%s,%p",NULL_POINTER_STRING(pszModuleName),pModule);
	if(pModule) {
		if(pszModuleName) {
			addConfigHandler(pszModuleName, pModule);
			addStatusHandler(pszModuleName, pModule);
		} 
		MsgBus.registerEventReceiver(pModule,pszModuleName);
	}
	DEBUG_FUNC_END();
}

/**
 * @brief Send shutdown events and restart the ESP after the given delay.
 * @param nDelay Delay in milliseconds before rebooting.
 * @param bForce true to reboot even if a receiver asks to delay shutdown.
 */
void CAppl::reboot(int nDelay, bool bForce) {
	Log.log("W",F("Rebooting..."));
	int nResult = MsgBus.sendEvent(this,MSG_APPL_SHUTDOWN,nullptr,0);
	if(nResult == EVENT_MSG_RESULT_OK || bForce) {
		Log.log("W",F("Restarting system..."));
		delay(nDelay);
		ESP.restart();
	}
}

#pragma endregion Module Registration and Rebooting

#pragma region Configuration Handling
/**
 * @brief Read application and registered-module settings from a JSON tree.
 * @param oJsonObj Root configuration object.
 */
void CAppl::readConfigFrom(JsonNode &oJsonObj) {
	DEBUG_FUNC_START();
	// Config.readConfigFrom(oJsonObj);
	// Speed oper
	oJsonObj.storeValueIf("logToSerial",&m_oCfg.bLogToSerial);
	oJsonObj.storeValueIf("traceMode",	&m_oCfg.bTraceMode);
	oJsonObj.storeValueIf("devicename",	 m_oCfg.strDeviceName);

	// Set the device password only, it is NOT the hidden appl password.
	// The mask (hidden password) and an empty value keep the current password -
	// an empty password would allow a web login without password.
	String strNewPwd;
	if(oJsonObj.storeValueIfNot("devicepwd",strNewPwd,HIDDEN_PASSWORD_MASK) && strNewPwd.length() > 0) {
		m_oCfg.strDevicePwd = strNewPwd;
	}
	if(m_oCfg.strDevicePwd.isEmpty())	m_oCfg.strDevicePwd  = DEFAULT_DEVICE_PWD;
	// An empty device name falls back to the default NAME (was: the default password)
	if(m_oCfg.strDeviceName.isEmpty()) 	m_oCfg.strDeviceName = DEFAULT_DEVICE_NAME;
	
	DEBUG_INFOS(" -- Device Name: %s",m_oCfg.strDeviceName.c_str());
	DEBUG_INFOS(" -- Device Pass: %s",m_oCfg.strDevicePwd.length() > 0 ? "(set)" : "(empty)");
	updateSerialLogWriter();

	// Then Iterate through registerd Config Handler
	CConfigHandler::readConfigFrom(oJsonObj);
	DEBUG_FUNC_END();
}

/** 
 * @brief Read the configuration file and load settings into all modules.
 *
 * - Use the pszConfigFileName, given by the user or use JSON_APPL_CONFIG_FILE (/config.json).
 * - If this file is not in place, use JSON_CONFIG_DEFAULT_File (/default)
 * @param pszConfigFileName The configuration FileName (Default is /config.json)
 * @param nJsonDocSize      Unused (kept for compatibility, JsonNode allocates dynamically).
 * @return true, if the file could be read...
 * */
bool CAppl::readConfigFrom(const char *pszConfigFileName, int nJsonDocSize) {
	DEBUG_FUNC_START_PARMS("%s,%d",NULL_POINTER_STRING(pszConfigFileName),nJsonDocSize);
	CFS oFS;
	if(!pszConfigFileName) 					pszConfigFileName = JSON_APPL_CONFIG_FILE;
	if(!oFS.fileExists(pszConfigFileName))	pszConfigFileName = JSON_CONFIG_DEFAULT_FILE;
	
	JsonNode oCfgData;

    bool bResult = false;
	
    if(oFS.loadJsonContentFromFile(pszConfigFileName,oCfgData)) {
		DEBUG_INFOS("Configuration loaded from file: %s",pszConfigFileName);
		DEBUG_JSON_OBJ(oCfgData);
		migrateConfig(oCfgData);
        readConfigFrom(oCfgData);
		bResult = true;
    } else if(oFS.fileExists(pszConfigFileName)) {
		// The file is in place, but could not be read/parsed - the defaults are used !
		ApplLogErrorWithParms(F("Config file %s could not be loaded (invalid JSON ?) - using defaults"),pszConfigFileName);
	}
	DEBUG_FUNC_END_PARMS("%d",bResult);
    return(bResult);
}

/**
 * @brief Migrate legacy configuration keys into their current locations.
 *
 * Called after loading JSON from the file system and before registered handlers
 * read their sections. Since 2026-03-02, devicename and devicepwd are moved
 * from their previous web/wifi locations when present.
 */
void CAppl::migrateConfig(JsonNode & oCfgData) {

	JsonNode * pCfgWeb = oCfgData.getObject("web");
	if( pCfgWeb ) {
		// Old location of the device password in web - httpPasswd
		const char * pszPasswdKeyName = "httpPasswd";
		if(pCfgWeb->exists(pszPasswdKeyName)) {
			m_oCfg.strDevicePwd = pCfgWeb->getValue(pszPasswdKeyName);
			pCfgWeb->remove(pszPasswdKeyName);
		}
	}
	JsonNode * pCfgWiFi = oCfgData.getObject("wifi");
	if(pCfgWiFi) {
		const char * pszHostnameKey = "hostname";
		if(pCfgWiFi->exists(pszHostnameKey)) {
			m_oCfg.strDeviceName = pCfgWiFi->getValue(pszHostnameKey);
			pCfgWiFi->remove(pszHostnameKey);
		}

	}

	// Now iterate through the handler...
	CConfigHandler::migrateConfig(oCfgData,oCfgData);
}


/** 
 * @brief Persist the current configuration to the file system.
 *
 * The existing config file is loaded first so unknown keys are preserved. Delete
 * the file beforehand if a clean generated config is desired.
 *
 * @param pszConfigFileName The configuration FileName (Default is /config.json)
 * @param nJsonDocSize      Unused (kept for compatibility, JsonNode allocates dynamically).
 * @return true, if the file could be written...
 * */
bool CAppl::saveConfig(const char *pszConfigFileName, int nJsonDocSize) {
	DEBUG_FUNC_START_PARMS("%s,%d",NULL_POINTER_STRING(pszConfigFileName),nJsonDocSize);
	if(!pszConfigFileName) pszConfigFileName = JSON_APPL_CONFIG_FILE;
	JsonNode oCfgDoc;
	CFS oFS;
	// load existing config file from the file system first, to keep unknown settings in place
	// then write the current config into the loaded document...
	oFS.loadJsonContentFromFile(pszConfigFileName,oCfgDoc);
	// Add the current timestamp
	oCfgDoc["TS"] = getISODateTime();
	writeConfigTo(oCfgDoc,false);

	bool bResult = oFS.saveJsonContentToFile(pszConfigFileName,oCfgDoc);
	DEBUG_FUNC_END_PARMS("%s",bResult ? "OK" : "ERROR");
    return(bResult);
}

/**
 * @brief Write the current configuration into a JsonNode
 * @param oJsonObj      The JsonNode to write the configuration to
 * @param bHideCritical If true, critical information like passwords will be hidden
 */
void CAppl::writeConfigTo(JsonNode &oJsonObj, bool bHideCritical) {
	DEBUG_FUNC_START_PARMS("%p,%d",&oJsonObj,bHideCritical);
	// Write all config values from the local var table first
	// then your own config data (ensures that appl config wins)
	Config.writeConfigTo(oJsonObj,bHideCritical);
	oJsonObj["logToSerial"] = m_oCfg.bLogToSerial;
	oJsonObj["traceMode"] 	= m_oCfg.bTraceMode;
	oJsonObj["devicename"] 	= m_oCfg.strDeviceName;
	oJsonObj["devicepwd"]  	= (const char*) (bHideCritical ? HIDDEN_PASSWORD_MASK : m_oCfg.strDevicePwd.c_str());

	// Iterate through registered Config Handler
	// This includes the own "cfg" form Config (IConfigHandler).
	CConfigHandler::writeConfigTo(oJsonObj,bHideCritical);
	DEBUG_JSON_OBJ(oJsonObj);
	DEBUG_FUNC_END();
}

#pragma endregion Configuration Handling

#pragma region Status Handling

/**
 * @brief Build and return the reusable application status node.
 *
 * The returned pointer is owned by this instance and remains valid until the
 * next status update or destruction. It must not be deleted by callers.
 */
JsonNode *  CAppl::getStatus(int nLevel) {
	m_oStatus.clear();
	writeStatusTo( m_oStatus,nLevel);
	return( & m_oStatus);
}

/**
 * @brief Return the current application status serialized as JSON text.
 *
 * The text is in a shared buffer that is rebuilt by every call - loop context
 * only. Other contexts (async web server) use getStatusSnapshot().
 */
const char * CAppl::getStatusAsText(int nLevel) {
    JsonNode * pStatus = getStatus(nLevel);
    return(pStatus->getAsJsonText());
}

/**
 * @brief Rebuilds the status snapshot (loop context, called by dispatch()).
 *
 * Uses an own JsonNode - the shared m_oStatus may be in use by the caller.
 */
void CAppl::updateStatusSnapshot() {
	JsonNode oStatus;
	writeStatusTo(oStatus, STATUS_LEVEL_INFO);
	String strStatus = oStatus.getAsJsonText();
	#ifdef ESP32
		std::lock_guard<std::mutex> oGuard(m_oStatusSnapshotLock);
	#endif
	m_strStatusSnapshot = strStatus;
}

/**
 * @brief Returns a copy of the status - safe from any context.
 */
String CAppl::getStatusSnapshot() {
	if(CEventHandler::isLoopContext()) {
		updateStatusSnapshot();			// fresh status in the loop
	}
	#ifdef ESP32
		std::lock_guard<std::mutex> oGuard(m_oStatusSnapshotLock);
	#endif
	return(m_strStatusSnapshot.length() > 0 ? m_strStatusSnapshot : String("{}"));
}

/**
 * @brief Rebuilds the system status snapshot (loop context, called by dispatch()).
 */
void CAppl::updateSystemStatusSnapshot() {
	JsonNode oStatus;
	writeSystemStatusTo(oStatus);
	String strStatus = oStatus.getAsJsonText();
	#ifdef ESP32
		std::lock_guard<std::mutex> oGuard(m_oStatusSnapshotLock);
	#endif
	m_strSystemStatusSnapshot = strStatus;
}

/**
 * @brief Returns a copy of the system status - safe from any context.
 *
 * Outside the loop the last snapshot is returned and a new one is requested
 * (built by the next dispatch()).
 */
String CAppl::getSystemStatusSnapshot() {
	if(CEventHandler::isLoopContext()) {
		updateSystemStatusSnapshot();	// fresh status in the loop
	} else {
		m_bSystemStatusRequested = true;
	}
	#ifdef ESP32
		std::lock_guard<std::mutex> oGuard(m_oStatusSnapshotLock);
	#endif
	return(m_strSystemStatusSnapshot.length() > 0 ? m_strSystemStatusSnapshot : String("{}"));
}

/**
 * @brief Return the compact state view of this device.
 * @returns a CJsonNode pointer (do not delete, it is managed by this instance)
 */
JsonNode * CAppl::getState() { return(getStatus(STATUS_LEVEL_STATE)); }

/**
 * @brief Return the compact device state serialized as JSON text.
 * @returns pointer to a json formated state string (do not delete or free this pointer !)
 */
const char * CAppl::getStateAsText() { 
	return(getStatus(STATUS_LEVEL_STATE)->getAsJsonText()); 
}
/**
 * @brief Write application metadata and module status into a JSON node.
 * @param oStatusObj Target JSON node.
 * @param nLevel Detail level requested by the caller.
 */
void CAppl::writeStatusTo(JsonNode &oStatusObj, int nLevel = STATUS_LEVEL_INFO) {
	DEBUG_FUNC_START();
	oStatusObj.setValue("now",			millis());
	oStatusObj.setValue("prog_name",	AppName);
	oStatusObj.setValue("prog_version",	AppVersion);
	oStatusObj.setValue("uptime",		getUpTime());
	oStatusObj.setValue("starttime",	StartTime);
	oStatusObj.setValue("datetime",		getISODateTime());
	if(m_oCfg.bDebugFrontend) {
		oStatusObj.setValue("DebugMode",true);
	}
	// Systemstatus becomes a own function call, to avoid
	// system operations on a high frequence...
	// writeSystemStatusTo(oStatusObj);
	// Iterate through additional registered Status Handler by
	// calling the base class member (this is NOT a static function ;-)
	CStatusHandler::writeStatusTo(oStatusObj,nLevel);
	DEBUG_FUNC_END();
}


/**
 * @brief Write lower-level system diagnostics into a JSON node.
 * @param oStatusNode Target JSON node.
 */
void CAppl::writeSystemStatusTo(JsonNode &oStatusNode) {
	DEBUG_FUNC_START();
	oStatusNode.setValue("now",millis());
	// CSysStatus oSysStatus;
	m_oSystemStatus.writeStatusTo(oStatusNode,STATUS_LEVEL_INFO);
    DEBUG_FUNC_END();
}

#pragma endregion Status Handling

#pragma region Time and Date functions 

/**
 * @brief Return the time since boot as HH:MM:SS (hours not limited to 24).
 */
const char * CAppl::getUpTime() {
	// 64 bit microseconds since boot - millis() wraps after ~49.7 days
	#ifdef ESP32
		uint64_t ullUpSeconds = (uint64_t) esp_timer_get_time() / 1000000ULL;
	#else
		uint64_t ullUpSeconds = micros64() / 1000000ULL;
	#endif
	unsigned long ulHours   = (unsigned long) (ullUpSeconds / 3600ULL);
	unsigned int  nMinutes  = (unsigned int) ((ullUpSeconds / 60ULL) % 60ULL);
	unsigned int  nSeconds  = (unsigned int) (ullUpSeconds % 60ULL);
	snprintf(m_szUptimeBuffer,sizeof(m_szUptimeBuffer), "%02lu:%02u:%02u", ulHours, nMinutes, nSeconds);
	return(m_szUptimeBuffer);
}


/**
 * @brief Get the ISO Date (YYYY-MM-DD)
 * @return The ISO Date as const char *
 */
const char * CAppl::getISODate() {
	if(sizeof(m_szCurDate) > 10) {
		getISODateTime();
		memset(m_szCurDate,'\0',sizeof(m_szCurDate));
		memcpy(m_szCurDate,m_szISODateTime,10);
	}
	return(m_szCurDate);
}

/**
 * @brief Get the ISO Time (HH:MM:SS)
 * @return The ISO Time as const char *
 */
const char * CAppl::getISOTime() {
	if(sizeof(m_szCurTime) > 8) {
		getISODateTime();
		memset(m_szCurTime,'\0',sizeof(m_szCurTime));
		memcpy(m_szCurTime,&m_szISODateTime[11],8);
	}
	return(m_szCurTime);
}

/**
 * @brief Get the native time_t value of the system
 * @return The native time_t value
 */
time_t CAppl::getNativeTime() {
        time(&m_oRawTime);
        return(m_oRawTime);
}

/**
 * @brief Get the ISO DateTime (YYYY-MM-DDTHH:MM:SS)
 * @return The ISO DateTime as const char *
 */
const char * CAppl::getISODateTime() {
        time_t oNativeTime = getNativeTime();
        // localtime_r: thread safe - the status is also written from the async web server
        struct tm oTimeInfo;
        memset(m_szISODateTime,'\0',sizeof(m_szISODateTime));
        if(localtime_r(&oNativeTime, &oTimeInfo)) {
            strftime(m_szISODateTime,sizeof(m_szISODateTime),"%FT%T",&oTimeInfo);
        }
        return(m_szISODateTime);
}


/**
 * @brief Return the stable chip/device identifier from system status.
 */
const char * CAppl::getDeviceID() {
	return(m_oSystemStatus.getChipID());
}
#pragma endregion Time and Date functions

#pragma region Diagnostic and Display Functions

/**
 * @brief Print a Hello Message to the Serial Console
 */
void CAppl::sayHello() {
	char szBuff[100];
	snprintf(szBuff,sizeof(szBuff), "%s v%s", AppName.c_str(), AppVersion.c_str());
	Serial.println("");
	Serial.println(szBuff);
	for(size_t i = 0; i < strlen(szBuff); i++) {
		Serial.print("=");
	}
	Serial.println("");
}   

/// @brief Print flash/heap diagnostics and verify flash size configuration.
/// @see https://42project.net/groesse-des-esp8266-flash-speicher-sowie-chip-id-ausgeben-und-mit-der-konfiguration-ueberpruefen/
void CAppl::printDiag() {

	// CSysStatus oSysStatus;
    uint32_t    realSize = m_oSystemStatus.getFlashChipRealSize(); // ESP.getFlashChipRealSize();
	uint32_t    ideSize  = ESP.getFlashChipSize();
	FlashMode_t ideMode  = ESP.getFlashChipMode();
	// Serial.printf("Flash real id     : %08X\n", ESP.getFlashChipId());
	Serial.printf("Flash real size   : %u\n\n", realSize);
	
	Serial.printf("Flash ide  size   : %u\n", ideSize);
	Serial.printf("Flash ide speed   : %u\n", ESP.getFlashChipSpeed());
	Serial.printf("Flash ide mode    : %s\n", (ideMode == FM_QIO ? "QIO" : ideMode == FM_QOUT ? "QOUT"
																	     : ideMode == FM_DIO  ? "DIO"
																	     : ideMode == FM_DOUT ? "DOUT"
																		 : "UNKNOWN"));
	Serial.printf("Free heap size    : %u\n", ESP.getFreeHeap());
	// Serial.printf("Free stack content: %u\n", ESP.getFreeContStack());

	if (ideSize != realSize)
	{
		Serial.println("Flash Chip configuration wrong!");
	}
	else
	{
		Serial.println("Flash Chip configuration ok.");
	}
}

#pragma endregion Diagnostic and Display Functions

#pragma region Global application object (nifty counter)

namespace {
    // Storage for Appl. The constexpr constructor makes it constant initialized
    // (before any dynamic initialization), so the reference "Appl" below is valid
    // from the start - the object itself is constructed by the first CApplInitializer.
    union ApplStorage {
        char  cDummy;
        CAppl oAppl;
        constexpr ApplStorage() : cDummy(0) {}
        ~ApplStorage() {}       // destruction is done by the last CApplInitializer
    };
    ApplStorage  s_oApplStorage;
    unsigned int s_nApplInitCounter = 0;   // zero initialized before any constructor runs
}

/// @brief The global application object
CAppl & Appl = s_oApplStorage.oAppl;

LSC::CApplInitializer::CApplInitializer() {
    if(s_nApplInitCounter++ == 0) new (&s_oApplStorage.oAppl) CAppl();
}

LSC::CApplInitializer::~CApplInitializer() {
    if(--s_nApplInitCounter == 0) s_oApplStorage.oAppl.~CAppl();
}

#pragma endregion
