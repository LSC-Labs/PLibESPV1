#ifndef DEBUG_LSC_APPL
	#undef DEBUGINFOS
#endif

#include <Appl.h>
#include <Logging.h>
#include <ConfigHandler.h>
#include <FileSystem.h>
#include <SysStatus.h>
#include <LSCUtils.h>
#include <JsonHelper.h>

#define LSC_APPL_HIDDEN_PASSWORD       "******"


CAppl::CAppl() {
    Log = CEventLogger(&MsgBus);
	MsgBus.registerEventReceiver(this);
}  

void CAppl::init(const char *strAppName, const char *strAppVersion) {
    AppName 	= strAppName;
    AppVersion  = strAppVersion;
    if(m_oCfg.bLogToSerial) {
        MsgBus.registerEventReceiver(new CSerialLogWriter());
    }
	MsgBus.sendEvent(this,MSG_APPL_STARTING,nullptr,0);
	MsgBus.sendEvent(this,MSG_APPL_INITIALIZED,nullptr,0);
}

int CAppl::receiveEvent(const void * pSender, int nMsgType, const void * pMessage, int nClass) {
    switch(nMsgType) {
        case MSG_REBOOT_REQUEST:
            reboot(nClass > 0 ? nClass : 2000,true);
            break;
    }
	return(EVENT_MSG_RESULT_OK);
}

void CAppl::reboot(int nDelay, bool bForce) {
	Log.log("W",F("Reboot requested..."));
	int nResult = MsgBus.sendEvent(this,MSG_APPL_SHUTDOWN,nullptr,0);
	if(nResult == EVENT_MSG_RESULT_OK || bForce) {
		Log.log("W",F("Restarting system..."));
		delay(nDelay);
		ESP.restart();
	}
}

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


/// @brief write the current configuration into the file system.
///        a current config file will be loaded first to avoid loosing unknown config data...
///        To get a clean config file, delete it first.
/// @param pszConfigFileName The configuration FileName (Default is /config.json)
/// @param nJsonDocSize      The size of the expected total size, othterwies JSON_CONFIG_DEFAULT_SIZE is used.
/// @return true, if the file could be written...
#if ARDUINOJSON_VERSION_MAJOR < 7
bool CAppl::saveConfig(const char *pszConfigFileName, int nJsonDocSize) {
	DEBUG_FUNC_START_PARMS("%s,%d",NULL_POINTER_STRING(pszConfigFileName),nJsonDocSize);
#else
bool CAppl::saveConfig(const char *pszConfigFileName) {
	DEBUG_FUNC_START_PARMS("%s,%d",NULL_POINTER_STRING(pszConfigFileName));
#endif
	if(!pszConfigFileName) pszConfigFileName = JSON_CONFIG_DEFAULT_NAME;
	#if ARDUINOJSON_VERSION_MAJOR < 7
		if(nJsonDocSize < 10)  nJsonDocSize 	 = JSON_CONFIG_DOC_DEFAULT_SIZE;
		DynamicJsonDocument oCfgDoc(nJsonDocSize);
	#else
		JsonDocument oCfgDoc;
	#endif
	CFS oFS;
	// load existing config file from the file system first, to keep unknown settings in place
	oFS.loadJsonContentFromFile(pszConfigFileName,oCfgDoc);
	JsonObject oCfgNode = oCfgDoc.to<JsonObject>();
	writeConfigTo(oCfgNode,false);
	bool bResult = oFS.saveJsonContentToFile(pszConfigFileName,oCfgDoc);
	DEBUG_FUNC_END_PARMS("%s",bResult ? "OK" : "ERROR");
    return(bResult);
}

void CAppl::writeConfigTo(JsonObject &oJsonObj, bool bHideCritical) {
	Config.writeConfigTo(oJsonObj,bHideCritical);
	oJsonObj["logToSerial"] = m_oCfg.bLogToSerial;
	oJsonObj["traceMode"] 	= m_oCfg.bTraceMode;
	oJsonObj["devicename"] 	= m_oCfg.strDeviceName;
	oJsonObj["devicepwd"]  	= bHideCritical ? LSC_APPL_HIDDEN_PASSWORD : m_oCfg.strDevicePwd;

	// Iterate through registered Config Handler
	CConfigHandler::writeConfigTo(oJsonObj,bHideCritical);
	DEBUG_FUNC_END_PARMS("memory used : %d bytes", oJsonObj.memoryUsage());

}

/**
 * @brief write the status into a JsonDocument...
 * As the document cannot be converted to a JsonObject, when it is empty,
 * write an entry first, and then convert it to an object... !
 */
void CAppl::writeStatusTo(JsonDocument &oStatusDoc) {
	oStatusDoc["now"] = millis();JsonObject oStatusObj = oStatusDoc.as<JsonObject>();
	writeStatusTo(oStatusObj);
}

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

void CAppl::writeSystemStatusTo(JsonDocument &oStatusDoc) {
	DEBUG_FUNC_START();
		oStatusDoc["now"] = millis();
		JsonObject oStatus = oStatusDoc.as<JsonObject>();
		writeSystemStatusTo(oStatus);
	DEBUG_FUNC_END();
}

void CAppl::writeSystemStatusTo(JsonObject &oStatusObj) {
    DEBUG_FUNC_START();
	if(!oStatusObj.isNull()) {
		CSysStatus oSysStatus;
		oSysStatus.writeStatusTo(oStatusObj);
	}
    DEBUG_FUNC_END();
}

void CAppl::readConfigFrom(JsonObject &oJsonObj) {
	DEBUG_FUNC_START();
	// Config.readConfigFrom(oJsonObj);
	// Speed oper
	LSC::setValue(&m_oCfg.bLogToSerial, oJsonObj["logToSerial"]);
	LSC::setValue(&m_oCfg.bTraceMode,   oJsonObj["traceMode"]);
	LSC::setValue(m_oCfg.strDeviceName, oJsonObj["devicename"]);
	/** Passwort only, if it is NOT the hidden appl password */
	String strPasswd = oJsonObj["devicepwd"];
    if(!strPasswd.equals(LSC_APPL_HIDDEN_PASSWORD)) {
       m_oCfg.strDevicePwd = strPasswd;
    }
	// Iterate through registerd Config Handler
	CConfigHandler::readConfigFrom(oJsonObj);
	DEBUG_FUNC_END();
}

void CAppl::readConfigFrom(JsonDocument &oJsonDoc) {
	JsonObject oCfgData = oJsonDoc.as<JsonObject>();
	readConfigFrom(oCfgData);
}

#if ARDUINOJSON_VERSION_MAJOR < 7
bool CAppl::readConfigFrom(const char *pszConfigFileName, int nJsonDocSize) {
	DEBUG_FUNC_START_PARMS("%s,%d",NULL_POINTER_STRING(pszConfigFileName),nJsonDocSize);
#else
bool CAppl::readConfigFrom(const char *pszConfigFileName) {
	DEBUG_FUNC_START_PARMS("%s",NULL_POINTER_STRING(pszConfigFileName));
#endif
	if(!pszConfigFileName) pszConfigFileName = JSON_CONFIG_DEFAULT_NAME;
	#if ARDUINOJSON_VERSION_MAJOR < 7
		if(nJsonDocSize < 10)  nJsonDocSize      = JSON_CONFIG_DOC_DEFAULT_SIZE;
		DynamicJsonDocument oCfgDoc(nJsonDocSize);
	#else
		JsonDocument oCfgDoc;
	#endif
    bool bResult = false;
	CFS oFS;
    if(oFS.loadJsonContentFromFile(pszConfigFileName,oCfgDoc)) {
		serializeJsonPretty(oCfgDoc,Serial);
        readConfigFrom(oCfgDoc);
		bResult = true;
    }
	DEBUG_FUNC_END_PARMS("%d",bResult);
    return(bResult);
}

const char * CAppl::getISODate() {
	if(sizeof(m_szCurDate) > 10) {
		getISODateTime();
		memset(m_szCurDate,'\0',sizeof(m_szCurDate));
		memcpy(m_szCurDate,m_szISODateTime,10);
	}
	return(m_szCurDate);
}

const char * CAppl::getISOTime() {
	if(sizeof(m_szCurTime) > 8) {
		getISODateTime();
		memset(m_szCurTime,'\0',sizeof(m_szCurTime));
		memcpy(m_szCurTime,&m_szISODateTime[11],8);
	}
	return(m_szCurTime);
}

time_t CAppl::getNativeTime() {
        time(&m_oRawTime);
        return(m_oRawTime);
}

const char * CAppl::getISODateTime() {
        time_t oNativeTime = getNativeTime();
        struct tm* oTimeInfo = localtime(&oNativeTime);
        memset(m_szISODateTime,'\0',sizeof(m_szISODateTime));
        strftime(m_szISODateTime,sizeof(m_szISODateTime),"%FT%T",oTimeInfo);
        return(m_szISODateTime);
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

/// @brief The global application object
CAppl Appl;

