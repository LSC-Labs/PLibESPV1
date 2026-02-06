#ifndef DEBUG_LSC_APPL
	#undef DEBUGINFOS
#endif

#include <Appl.h>
#include <FileSystem.h>
#include <SysStatus.h>
#include <LSCUtils.h>

#pragma region Constructor / init and Message Dispatching

CAppl::CAppl() {
    Log = CEventLogger(&MsgBus);
	MsgBus.registerEventReceiver(this,__FUNCTION__);
}  

/**
 * @brief Initialize the Application Framework
 * @param strAppName    The name of the Application
 * @param strAppVersion The Version of the Application
 */
void CAppl::init(const char *strAppName, const char *strAppVersion) {
    AppName 	= strAppName;
    AppVersion  = strAppVersion;
	
    if(m_oCfg.bLogToSerial) {
        MsgBus.registerEventReceiver(new CSerialLogWriter(),"CSerialLogWriter");
    }
	MsgBus.sendEvent(this,MSG_APPL_STARTING,nullptr,0);
	MsgBus.sendEvent(this,MSG_APPL_INITIALIZED,nullptr,0);
}


/**
 * @brief Dispatch a periodic message to all registered Message Event Receivers
 * @param pMsg       Optional Message Pointer
 * @param nMsgClass Optional Message Class
 */
void CAppl::dispatch(const void *pMsg, int nMsgClass) {
	this->MsgBus.sendEvent(this,MSG_APPL_LOOP,pMsg,nMsgClass);
}

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
 * @brief Register a Module to the Application
 * - Register Config Handler
 * - Register Status Handler
 * - Register Message Event Receiver
 * @param pszModuleName The name of the module (Config / Status Handler Name)
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
 * @brief Reboot the System after nDelayMillis milliseconds
 * @param nDelayMillis Delay in milliseconds before rebooting
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
 * @brief Read the configuration from a JsonObject
 * @param oJsonObj The JsonObject to read the configuration from
 * TODO: Implement the local var table (Config) reading also...
 */
void CAppl::readConfigFrom(JsonObject &oJsonObj) {
	DEBUG_FUNC_START();
	// Config.readConfigFrom(oJsonObj);
	// Speed oper
	LSC::setJsonValue(oJsonObj,"logToSerial",&m_oCfg.bLogToSerial);
	LSC::setJsonValue(oJsonObj,"traceMode",&m_oCfg.bTraceMode);
	LSC::setJsonValue(oJsonObj,"devicename",m_oCfg.strDeviceName);

	// Set the device password only, it is NOT the hidden appl password
	// If it is empty, keep the default passwort.
	LSC::setJsonValueIfNot(oJsonObj,"devicepwd",m_oCfg.strDevicePwd,HIDDEN_PASSWORD_MASK);
	if(m_oCfg.strDeviceName.isEmpty()) m_oCfg.strDeviceName = DEFAULT_DEVICE_PWD;
	
	DEBUG_INFOS(" -- Device Name: %s",m_oCfg.strDeviceName.c_str());
	DEBUG_INFOS(" -- Device Pass: \"%s\"",m_oCfg.strDevicePwd.c_str());

	// Iterate through registerd Config Handler
	CConfigHandler::readConfigFrom(oJsonObj);
	DEBUG_FUNC_END();
}

/**
 * @brief Read the configuration from a JsonDocument
 * @param oJsonDoc The JsonDocument to read the configuration from	
 */
void CAppl::readConfigFrom(JsonDocument &oJsonDoc) {
	JsonObject oCfgData = GetJsonDocumentAsObject(oJsonDoc);
	readConfigFrom(oCfgData);
}


/** 
 * @brief read the configuration from the file system.
 * @param pszConfigFileName The configuration FileName (Default is /config.json)
 * @param nJsonDocSize      The size of the expected total size, othterwies JSON_CONFIG_DEFAULT_SIZE is used.
 * 							(obsolet for ArduinoJson >= 7)
 * @return true, if the file could be read...
 * */
bool CAppl::readConfigFrom(const char *pszConfigFileName, int nJsonDocSize) {
	DEBUG_FUNC_START_PARMS("%s,%d",NULL_POINTER_STRING(pszConfigFileName),nJsonDocSize);
	if(!pszConfigFileName) pszConfigFileName = JSON_CONFIG_DEFAULT_NAME;
	JSON_DOC(oCfgDoc,nJsonDocSize);

    bool bResult = false;
	CFS oFS;
    if(oFS.loadJsonContentFromFile(pszConfigFileName,oCfgDoc)) {
		DEBUG_INFOS("Configuration loaded from file: %s",pszConfigFileName);
		DEBUG_JSON_OBJ(oCfgDoc);
        readConfigFrom(oCfgDoc);
		bResult = true;
    }
	DEBUG_FUNC_END_PARMS("%d",bResult);
    return(bResult);
}


/** 
 * @brief write the current configuration into the file system.
 *        a current config file will be loaded first to avoid loosing unknown config data...
 *        To get a clean config file, delete it first.
 * @param pszConfigFileName The configuration FileName (Default is /config.json)
 * @param nJsonDocSize      The size of the expected total size, othterwies JSON_CONFIG_DEFAULT_SIZE is used.
 * 							(obsolet for ArduinoJson >= 7)
 * @return true, if the file could be written...
 * */
bool CAppl::saveConfig(const char *pszConfigFileName, int nJsonDocSize) {
	DEBUG_FUNC_START_PARMS("%s,%d",NULL_POINTER_STRING(pszConfigFileName),nJsonDocSize);
	if(!pszConfigFileName) pszConfigFileName = JSON_CONFIG_DEFAULT_NAME;
	JSON_DOC(oCfgDoc,nJsonDocSize);
	CFS oFS;
	// load existing config file from the file system first, to keep unknown settings in place
	// then write the current config into the loaded document...
	oFS.loadJsonContentFromFile(pszConfigFileName,oCfgDoc);
	// Add the current timestamp
	oCfgDoc["TS"] = getISODateTime();
	JsonObject oCfgNode = GetJsonDocumentAsObject(oCfgDoc);
	writeConfigTo(oCfgNode,false);

	bool bResult = oFS.saveJsonContentToFile(pszConfigFileName,oCfgDoc);
	DEBUG_FUNC_END_PARMS("%s",bResult ? "OK" : "ERROR");
    return(bResult);
}

/**
 * @brief Write the current configuration into a JsonObject
 * @param oJsonObj      The JsonObject to write the configuration to
 * @param bHideCritical If true, critical information like passwords will be hidden
 */
void CAppl::writeConfigTo(JsonObject &oJsonObj, bool bHideCritical) {
	DEBUG_FUNC_START_PARMS("%p,%d",&oJsonObj,bHideCritical);
	// Write all config values from the local var table first
	// then your own config data (ensures that appl config wins)
	Config.writeConfigTo(oJsonObj,bHideCritical);
	oJsonObj["logToSerial"] = m_oCfg.bLogToSerial;
	oJsonObj["traceMode"] 	= m_oCfg.bTraceMode;
	oJsonObj["devicename"] 	= m_oCfg.strDeviceName;
	oJsonObj["devicepwd"]  	= bHideCritical ? HIDDEN_PASSWORD_MASK : m_oCfg.strDevicePwd;

	// Iterate through registered Config Handler
	CConfigHandler::writeConfigTo(oJsonObj,bHideCritical);
	DEBUG_JSON_OBJ(oJsonObj);
	DEBUG_FUNC_END();
}

#pragma endregion Configuration Handling

#pragma region Status Handling

/**
 * @brief write the status into a JsonDocument...
 * As the document cannot be converted to a JsonObject, when it is empty,
 * write an entry first, and then convert it to an object... !
 */
void CAppl::writeStatusTo(JsonDocument &oStatusDoc) {
	oStatusDoc["now"] = millis();
	JsonObject oStatusObj = GetJsonDocumentAsObject(oStatusDoc);
	writeStatusTo(oStatusObj);
}

/**
 * @brief Write the current status into a JsonObject
 * @param oStatusObj The JsonObject to write the status to
 */
void CAppl::writeStatusTo(JsonObject &oStatusObj) {
	DEBUG_FUNC_START();
	oStatusObj["now"] 			= millis();	
	oStatusObj["prog_name"] 	= AppName;
	oStatusObj["prog_version"] 	= AppVersion;
	oStatusObj["uptime"] 		= getUpTime();
	oStatusObj["starttime"]     = StartTime;
	oStatusObj["datetime"]		= getISODateTime();
	if(m_oCfg.bDebugFrontend) {
		oStatusObj["DebugMode"]     = "1";
	}
	// Systemstatus becomes a own function call, to avoid
	// system operations on a high frequence...
	// writeSystemStatusTo(oStatusObj);
	// Iterate through additional registered Status Handler by
	// calling the base class member (this is NOT a static function ;-)
	CStatusHandler::writeStatusTo(oStatusObj);
	DEBUG_FUNC_END();
}

/**
 * @brief Write the system status into a JsonDocument
 * @param oStatusDoc The JsonDocument to write the system status to
 */
void CAppl::writeSystemStatusTo(JsonDocument &oStatusDoc) {
	DEBUG_FUNC_START();
		oStatusDoc["now"] = millis();
		JsonObject oStatus = GetJsonDocumentAsObject(oStatusDoc);
		writeSystemStatusTo(oStatus);
	DEBUG_FUNC_END();
}

/**
 * @brief Write the system status into a JsonObject
 * @param oStatusObj The JsonObject to write the system status to
 */
void CAppl::writeSystemStatusTo(JsonObject &oStatusObj) {
    DEBUG_FUNC_START();
	if(!oStatusObj.isNull()) {
		CSysStatus oSysStatus;
		oSysStatus.writeStatusTo(oStatusObj);
	}
    DEBUG_FUNC_END();
}

#pragma endregion Status Handling

#pragma region Time and Date functions 

/**
 * @brief Get the Uptime of the System as String HH:MM:SS
 * @return String with the Uptime
 */
String CAppl::getUpTime() {
	unsigned long ulUpTime = millis() - this->StartTime;
	// 3600000 milliseconds in an hour
	long nHours   = ulUpTime / 3600000; 		// 1000 * 60 * 60
	ulUpTime = ulUpTime - 3600000 * nHours;		// Remove hours

	// 60000 milliseconds in a minute
	long nMinutes = ulUpTime / 60000;			// 1000 * 60
	ulUpTime = ulUpTime - 60000 * nMinutes;		// Remove minutes

	// 1000 milliseconds in a second		
	long nSeconds = ulUpTime / 1000;			// 1000 ms
	
	char tUpTimeBuffer[20];
	sprintf(tUpTimeBuffer, "%02lu:%02lu:%02lu", nHours, nMinutes, nSeconds);
	return(String(tUpTimeBuffer));
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
        struct tm* oTimeInfo = localtime(&oNativeTime);
        memset(m_szISODateTime,'\0',sizeof(m_szISODateTime));
        strftime(m_szISODateTime,sizeof(m_szISODateTime),"%FT%T",oTimeInfo);
        return(m_szISODateTime);
}

#pragma endregion Time and Date functions

#pragma region Diagnostic and Display Functions

/**
 * @brief Print a Hello Message to the Serial Console
 */
void CAppl::sayHello() {
	char strBuff[100];
	sprintf(strBuff, "%s v%s", AppName.c_str(), AppVersion.c_str());
	Serial.println("");
	Serial.println(strBuff);
	for(size_t i = 0; i < strlen(strBuff); i++) {
		Serial.print("=");
	}
	Serial.println("");
}   

/// @brief print some diagnostic information
/// https://42project.net/groesse-des-esp8266-flash-speicher-sowie-chip-id-ausgeben-und-mit-der-konfiguration-ueberpruefen/
void CAppl::printDiag() {

	CSysStatus oSysStatus;
    uint32_t    realSize = oSysStatus.getFlashChipRealSize(); // ESP.getFlashChipRealSize();
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
		Serial.println("Flash Chip configuration wrong!\n");
	}
	else
	{
		Serial.println("Flash Chip configuration ok.\n");
	}
}

#pragma endregion Diagnostic and Display Functions

/// @brief The global application object
CAppl Appl;

