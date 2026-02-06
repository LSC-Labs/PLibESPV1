// #pragma GCC diagnostic ignored "-Woverflow"
#ifndef DEBUG_LSC_WIFI
    #undef DEBUGINFOS
#endif

#include <Appl.h>
#include <Msgs.h>
#include <EventHandler.h>
#include <JsonHelper.h>
#include <WiFiController.h>
#include <LSCUtils.h>


const char * WIFI_AP_MODE           = "ap_mode";
const char * WIFI_AP_SSID           = "ap_ssid";
const char * WIFI_AP_CHANNEL        = "ap_channel";
const char * WIFI_AP_PASSWORD       = "ap_pwd";
const char * WIFI_AP_IPADDRESS      = "ap_ipaddress";
const char * WIFI_AP_SUBNETMASK     = "ap_subnet";
const char * WIFI_FALLBACK_MODE     = "fallback";
const char * WIFI_AP_HIDDEN         = "ap_hide";

const char * WIFI_SSID              = "ssid";
const char * WIFI_BSSID             = "bssid";
const char * WIFI_PASSWORD          = "wifi_pwd";
const char * WIFI_DHCP              = "dhcp";
const char * WIFI_IPADDRESS         = "ipaddress";
const char * WIFI_SUBNETMASK        = "subnet";
const char * WIFI_GATEWAY           = "gwip";
const char * WIFI_DNS               = "dnsip";

namespace LSC_WIFI {

    /**
     *  @brief Create the Default SSID for the Access Point
     */ 
    String getDefaultSSIDofAP()
    {
        uint8_t macAddr[6];
        WiFi.softAPmacAddress(macAddr);
        char ssid[strlen(WIFI_DEFAULT_AP_SSID_PREFIX) + 10];
        sprintf(ssid, WIFI_DEFAULT_AP_SSID_PREFIX "-%02x%02x%02x%02x",macAddr[2], macAddr[3], macAddr[4], macAddr[5]);
        return String(ssid);
    }
}

/**
 * Constructor

 */
CWiFiController::CWiFiController() {}



/**
 * @brief Create the Default SSID for the Access Point
 */ 
String CWiFiController::getDefaultSSIDofAP()
{
    return(LSC_WIFI::getDefaultSSIDofAP());
}



#pragma region "Interface to Config / Status handling"

/**
 * @brief Get the textual representation of the WiFi status
 */
String CWiFiController::getStatusText(int nWiFiStatus) {
    switch (nWiFiStatus)
    {
        case WL_IDLE_STATUS:         return F("Idle");
        case WL_NO_SSID_AVAIL:       return F("No SSID available");
        case WL_SCAN_COMPLETED:      return F("Scan completed");
        case WL_CONNECTED:           return F("Connected");
        case WL_CONNECT_FAILED:      return F("Connect failed");
        case WL_CONNECTION_LOST:     return F("Connection lost");
        case WL_WRONG_PASSWORD:      return F("Wrong password");
        case WL_DISCONNECTED:        return F("Disconnected");
        default:                     return F("Unknown status");
    }
}

/**
 * @brief Write your configuration into the json object
 */
void CWiFiController::writeConfigTo(JsonObject &oCfgNode,bool bHideCritical) {
    DEBUG_FUNC_START();
    // Hostname, Operation Mode and Fallback settings
    // oCfgNode["hostname"]        = Config.wifi_Hostname;
    oCfgNode[WIFI_AP_MODE]          = Config.accessPointMode;
    oCfgNode[WIFI_FALLBACK_MODE]    = Config.autoFallbackMode;

    // Access Point specific settings
    oCfgNode[WIFI_AP_SSID]          = Config.ap_ssid;
    oCfgNode[WIFI_AP_PASSWORD]      = bHideCritical ? WIFI_HIDDEN_PASSWORD : Config.ap_Password;
    oCfgNode[WIFI_AP_HIDDEN]        = Config.ap_hidden;
    oCfgNode[WIFI_AP_CHANNEL]       = Config.ap_channel;
    if(Config.ap_ipAddress.isSet())   oCfgNode[WIFI_AP_IPADDRESS] = Config.ap_ipAddress.toString(); 
    if(Config.ap_ipSubnetMask.isSet())oCfgNode[WIFI_AP_SUBNETMASK]= Config.ap_ipSubnetMask.toString();
    // WiFi specific settings
    oCfgNode[WIFI_SSID]            = Config.wifi_ssid;
    oCfgNode[WIFI_BSSID]           = Config.wifi_bssid;
    oCfgNode[WIFI_PASSWORD]        = bHideCritical ? WIFI_HIDDEN_PASSWORD : Config.wifi_Password;
    oCfgNode[WIFI_DHCP]            = Config.dhcpEnabled;
    if(Config.ipAddress.isSet())     oCfgNode[WIFI_IPADDRESS] = Config.ipAddress.toString();
    if(Config.ipSubnetMask.isSet())  oCfgNode[WIFI_SUBNETMASK]    = Config.ipSubnetMask.toString();
    if(Config.ipDNS.isSet())         oCfgNode[WIFI_DNS]     = Config.ipDNS.toString();
    if(Config.ipGateway.isSet())     oCfgNode[WIFI_GATEWAY]      = Config.ipGateway.toString();
    DEBUG_FUNC_END();
}

/**
 * @brief Read the configuration from the json object
 */
void CWiFiController::readConfigFrom(JsonObject &oNode) {
    DEBUG_FUNC_START();
    DEBUG_JSON_OBJ(oNode);
    // LSC::setValue(Config.wifi_Hostname,     oNode["hostname"]);
    LSC::setJsonValue(oNode,WIFI_FALLBACK_MODE, &  Config.autoFallbackMode);
    LSC::setJsonValue(oNode,WIFI_AP_MODE,       &  (Config.accessPointMode));
    LSC::setJsonValue(oNode,WIFI_AP_HIDDEN,     &  Config.ap_hidden);
    LSC::setJsonValue(oNode,WIFI_AP_CHANNEL,    &  Config.ap_channel);
    LSC::setJsonValue(oNode,WIFI_AP_IPADDRESS,     Config.ap_ipAddress);
    LSC::setJsonValue(oNode,WIFI_AP_SUBNETMASK,    Config.ap_ipSubnetMask);
    LSC::setJsonValue(oNode,WIFI_AP_SSID,          Config.ap_ssid);
    LSC::setJsonValueIfNot(oNode,WIFI_AP_PASSWORD, Config.ap_Password, WIFI_HIDDEN_PASSWORD);

    LSC::setJsonValue(oNode,WIFI_SSID,          Config.wifi_ssid);
    LSC::setJsonValueIfNot(oNode,WIFI_PASSWORD, Config.wifi_Password, WIFI_HIDDEN_PASSWORD);

    if(oNode["bssid"]) LSC::parseBytesToArray(Config.wifi_bssid, oNode["bssid"],':',sizeof(Config.wifi_bssid),16);
    LSC::setJsonValue(oNode,WIFI_DHCP,         &Config.dhcpEnabled);
    LSC::setJsonValue(oNode,WIFI_IPADDRESS,     Config.ipAddress);
    LSC::setJsonValue(oNode,WIFI_SUBNETMASK,    Config.ipSubnetMask);
    LSC::setJsonValue(oNode,WIFI_GATEWAY,       Config.ipGateway);
    LSC::setJsonValue(oNode,WIFI_DNS,           Config.ipDNS);
    DEBUG_FUNC_END();
} 

/**
 * @brief Write the status information into the json object
 */
void CWiFiController::writeStatusTo(JsonObject &oStatusNode) {
    DEBUG_FUNC_START();
    oStatusNode["accesspoint"] = Status.isInAccessPointMode;
    oStatusNode["stationmode"] = Status.isInStationMode,
    oStatusNode["isConnected"] = Status.isWiFiConnected;
    oStatusNode["startTime"]   = Status.startTimeInMillis;
    oStatusNode["stopTime"]    = Status.stopTimeInMillis;
    oStatusNode["restartTime"] = Status.restartTimeInMillis;
    oStatusNode["onlineTime"]  = Status.startTimeInMillis > 0 ? millis() - Status.startTimeInMillis : 0;
    struct ip_info oIPInfo;
    if ( Status.isInAccessPointMode)
	{
		wifi_get_ip_info(SOFTAP_IF, &oIPInfo);
		struct softap_config oConf;
		wifi_softap_get_config(&oConf);
		oStatusNode["ssid"]  = String(reinterpret_cast<char *>(oConf.ssid));
		oStatusNode["mac"]   = WiFi.softAPmacAddress();
	}
	else
	{
		wifi_get_ip_info(STATION_IF, &oIPInfo);
		struct station_config oConf;
		wifi_station_get_config(&oConf);
		oStatusNode["ssid"] = String(reinterpret_cast<char *>(oConf.ssid));
		oStatusNode["dns"] = WiFi.dnsIP().toString();
		oStatusNode["mac"] = WiFi.macAddress();
	}

    IPAddress ipaddr = IPAddress(oIPInfo.ip.addr);
	IPAddress gwaddr = IPAddress(oIPInfo.gw.addr);
	IPAddress nmaddr = IPAddress(oIPInfo.netmask.addr);

    oStatusNode["ip"]       = ipaddr.toString(); // getAddressAsString(ipaddr);
	oStatusNode["gateway"]  = gwaddr.toString(); // getAddressAsString(gwaddr);
	oStatusNode["netmask"]  = nmaddr.toString(); //getAddressAsString(nmaddr);
    oStatusNode["hostname"] = WiFi.getHostname();
    oStatusNode["rssi"] = WiFi.RSSI();
    DEBUG_FUNC_END();
}

/**
 * @brief Write the status information into the log
 */
void CWiFiController::writeStatusToLog() {
    JSON_DOC(oStatusDoc, 1024);
    JsonObject oStatusObj = oStatusDoc.to<JsonObject>();
    writeStatusTo(oStatusObj);
    String strPretty;
    serializeJsonPretty(oStatusDoc,strPretty);
    ApplLogVerboseWithParms(F("WiFi Status:\n%s"),strPretty.c_str());
}

#pragma endregion

/// @brief Listen to the application mesage bus
/// @param pSender 
/// @param nMsgId 
/// @param pMessage 
/// @param nType 
/// @return EVENT_MSG_RESULT_OK => continue processing
int CWiFiController::receiveEvent(const void * pSender, int nMsgId, const void * pMessage, int nType) {
    switch(nMsgId) {
        case MSG_WIFI_SCAN : scanWiFi(); break;
    }
    return(EVENT_MSG_RESULT_OK);
}


#pragma region "Starter for Access Point and Station Mode"

/** 
 * @brief start the access point
 *       - if it does not work by config start a default access point
 * @param bUseConfigData use config data
 * @return connected or not
 * */
bool CWiFiController::startAccessPoint(bool bUseConfigData)
{   bool bIsConnected = false;
    if(bUseConfigData) {
        bIsConnected = startAccessPoint(Config.ap_ssid.c_str(),
                                        Config.ap_ipAddress,
                                        Config.ap_ipSubnetMask,
                                        Config.ap_hidden,
                                        Config.ap_Password.c_str());
    }
    // Start the default access point, if config did not work...
    if(!bIsConnected) {    
        bIsConnected = startAccessPoint(getDefaultSSIDofAP().c_str(), 
                                        WIFI_MODULE_DEFAULT_AP_IP, 
                                        WIFI_MODULE_DEFAULT_AP_SUBNET, 
                                        false);
    }
    
    return bIsConnected;
}

/** 
 * @brief start the access point
 *       - if it does not work by config start a default access point
 * @param pszSSID      SSID of this node
 * @param ipAP         IPAddress of this node
 * @param ipSubnetAP   Subnet mask of this node. 
 * @param bHidden      if true, the SSID Network will be hidden...
 * @param pszPassword  Password for this Accesspoint
 * @return connected or not
 * */
bool CWiFiController::startAccessPoint(const char * pszSSID, 
                                        IPAddress ipAP, 
                                        IPAddress ipSubnetAP,
                                        bool bHidden, 
                                        const char *pszPassword)
{
    DEBUG_FUNC_START();
    if(pszSSID != nullptr) {
        ApplLogInfoWithParms(F("Starting WiFi Access Point: %s"),pszSSID);
        Appl.MsgBus.sendEvent(this,MSG_WIFI_STARTING,nullptr,WIFI_ACCESS_POINT_MODE);

        Status.isInStationMode      = false;
        Status.isInAccessPointMode  = true;
        Status.startTimeInMillis    = 0;
        Status.wifiStatus = WL_DISCONNECTED;

        WiFi.mode(WIFI_AP);
    
        // local-ip, gateway, subnet
        WiFi.softAPConfig(ipAP, ipAP, ipSubnetAP);
        Status.isWiFiConnected = WiFi.softAP(pszSSID, 
                                            pszPassword, 
                                            Config.ap_channel, 
                                            bHidden ? 1 : 0);
        if(Status.isWiFiConnected) {
            Status.wifiStatus = WL_IDLE_STATUS;
            Status.startTimeInMillis = millis();
            Appl.MsgBus.sendEvent(this,MSG_WIFI_CONNECTED,nullptr,WIFI_ACCESS_POINT_MODE);
        }
    }
    DEBUG_FUNC_END_PARMS("%d",Status.isWiFiConnected);
    return Status.isWiFiConnected;
}


/**
 * @brief join an existing network (Station Mode)
 * @param pszSSID SSID to join
 * @param pszPassword Password to use
 * @param bSSID BSSID to use (if 0,0,0,0,0,0 - do not use BSSID)
 * @return connected or not
 */
bool CWiFiController::joinNetwork(const char *pszSSID, const char *pszPassword, byte bSSID[6])
{
    DEBUG_FUNC_START();
    ApplLogInfoWithParms(F("Joining WiFi network %s"),pszSSID);
    Appl.MsgBus.sendEvent(this,MSG_WIFI_STARTING,nullptr,WIFI_STATION_MODE);

    Status.isInStationMode = true;
    Status.isInAccessPointMode = false;
    Status.startTimeInMillis = 0;
    Status.restartTimeInMillis = -1;    

    WiFi.mode(WIFI_STA);
    WiFi.persistent(false);

    
    if (!Config.dhcpEnabled)
    {
        DEBUG_INFOS(" - using static IP address %s", Config.ipAddress.toString().c_str());   
        WiFi.config(Config.ipAddress, Config.ipGateway, Config.ipSubnetMask, Config.ipDNS);
    }
    bool useBSSID = false;
    for (int i = 0; i < 6; i++)
    {
        if (bSSID[i] != 0) useBSSID = true;
    }
    if (useBSSID) {
        DEBUG_INFO(" - using fix bssid");
        Status.wifiStatus = WiFi.begin(Config.wifi_ssid, Config.wifi_Password, 0, Config.wifi_bssid);
    }
    else {
        DEBUG_INFOS(" - begin connection to %s/%s",Config.wifi_ssid.c_str(),Config.wifi_Password.c_str());
        Status.wifiStatus = WiFi.begin(Config.wifi_ssid, Config.wifi_Password);
    }         
    
    unsigned long now = millis();
    uint8_t nTimeout = 15; // define when to time out in seconds
    do {
        if (WiFi.isConnected()) break;
        delay(500);
    } while (millis() - now < nTimeout * 1000);

    if (WiFi.isConnected())
    {
        Appl.MsgBus.sendEvent(this,MSG_WIFI_CONNECTED,nullptr,WIFI_STATION_MODE);
        Status.isWiFiConnected  = true;
        Status.startTimeInMillis = millis();
    } else {
        Appl.MsgBus.sendEvent(this,MSG_WIFI_ERROR,nullptr,Status.wifiStatus);
        ApplLogErrorWithParms(F(" - failed to connect to %s"),Config.wifi_ssid.c_str());
        ApplLogErrorWithParms(F(" - status: %d (%s)"),Status.wifiStatus,getStatusText(Status.wifiStatus));
    }
    return(Status.isWiFiConnected );
}


/**
 * @brief start the setup of wifi (first call in usage)
 * Main entry point to start the WiFi.
 * 
 * If the instance has already a configuration, we can start the setup with enableWiFi).
 * Otherwise go into Default Acces Point mode
 * @param bUseConfigData if false - start default AP, if true, use the loaded config data.
 */
void CWiFiController::startWiFi(bool bUseConfigData)
{
    DEBUG_FUNC_START_PARMS("%d",bUseConfigData);
    Status.restartTimeInMillis = -1;
    if (!bUseConfigData)
    {
        WiFi.hostname( WIFI_DEFAULT_AP_SSID_PREFIX );
        startAccessPoint(false);
    } else
    {
        // possible useful event handlers for WiFi events
        // wifiConnectHandler = WiFi.onStationModeConnected(onWifiConnect);
        // wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);
        // wifiOnStationModeGotIPHandler = WiFi.onStationModeGotIP(onWifiGotIP);
        WiFi.hostname(Appl.getDeviceName());
        bool bIsConnected = false;
        if(Config.accessPointMode) {
            bIsConnected = startAccessPoint(true);
        } else {
            bIsConnected = joinNetwork(Config.wifi_ssid.c_str(), 
                                        Config.wifi_Password.c_str(), 
                                        Config.wifi_bssid);
            if(!bIsConnected) {
                if(Config.autoFallbackMode || Status.wifiStatus == WL_WRONG_PASSWORD ) {
                    Appl.Log.logInfo(F("... auto fallback to AP Mode"));
                    bIsConnected = startAccessPoint(true);
                }
                if(!bIsConnected) {
                    // If the connection has the wrong password... do not try to restart.
                    // Reconnect retry...
                    Appl.Log.logInfo(F("... trying reconnect to WiFi network in %d seconds"),Config.retryTimeoutSeconds);
                    Status.restartTimeInMillis = millis() + (Config.retryTimeoutSeconds * 1000); // Retry in 10 seconds
                }
                if(bIsConnected) Status.nRetryConnectCounter = 0;
            }
        } 
    }
    writeStatusToLog();
    DEBUG_FUNC_END();
};

/** 
 * @brief restart the WiFi if needed
 * If the wifi is not connected and the restart time is reached, try to restart the WiFi
 * @return connected or not
 */
bool CWiFiController::restartIfNeeded() {
    if(!Status.isWiFiConnected) {
        if(Status.restartTimeInMillis > millis()) {
            if(Config.retryCount < Status.nRetryConnectCounter) {
                Status.nRetryConnectCounter++;
                startWiFi(true);
            } else {
                disableWiFi();
            }
        }
    }
    return Status.isWiFiConnected;
}

/**
 * @brief disable the WiFi
 */
void CWiFiController::disableWiFi()
{
    DEBUG_FUNC_START();
    Appl.Log.logInfo(F("Turn wifi off."));
    Appl.MsgBus.sendEvent(this,MSG_WIFI_DISABLING,nullptr,0);
    WiFi.disconnect(true);
    WiFi.softAPdisconnect(true);
    Appl.MsgBus.sendEvent(this,MSG_WIFI_DISABLED,nullptr,0);
    Status.stopTimeInMillis = millis();
    Status.isInStationMode = false;
    Status.isInAccessPointMode = false;
    Status.isWiFiConnected = false;
    Status.restartTimeInMillis = -1;
    DEBUG_FUNC_END();
}

#pragma endregion

#pragma region "WiFi Scan"
/**
 * @brief Start a WiFi Scan
 * the result will be send via MSG_WIFI_SCAN_RESULT event on the message bus
 */
void CWiFiController::scanWiFi() {
    DEBUG_FUNC_START();
    // Use Member function of this object - and broadcast to all...
    std::function<void(int)> printWiFiScanResult = std::bind(&CWiFiController::onWiFiScanResult,this,std::placeholders::_1);
    WiFi.scanNetworksAsync(printWiFiScanResult,true);
}

/**
 * @brief Callback function for WiFi Scan Result
 * send the result via message bus
 */
void CWiFiController::onWiFiScanResult(int nNetworksFound) {
	DEBUG_FUNC_START_PARMS("%d",nNetworksFound);
    // sort by RSSI (Signal Strength - highest first)
	int tIndices[nNetworksFound];
	for (int i = 0; i < nNetworksFound; i++)
	{
		tIndices[i] = i;
	}
	for (int i = 0; i < nNetworksFound; i++)
	{
		for (int j = i + 1; j < nNetworksFound; j++)
		{
			if (WiFi.RSSI(tIndices[j]) > WiFi.RSSI(tIndices[i]))
			{
				std::swap(tIndices[i], tIndices[j]);
			}
		}
	}

    JSON_DOC(oRootDoc,512);
	oRootDoc["command"] = "update";
	oRootDoc["data"] 	= "ssidlist";
	JsonArray oScanResult = CreateJsonArray(oRootDoc,"payload");
	for (int i = 0; i < 10 && i < nNetworksFound; ++i)
	{
		// JsonObject oItem = oScanResult.createNestedObject();
        JsonObject oItem = CreateEmptyJsonObject(oScanResult);
		oItem["ssid"] 		= WiFi.SSID(tIndices[i]);
		oItem["bssid"] 		= WiFi.BSSIDstr(tIndices[i]);
		oItem["rssi"] 		= WiFi.RSSI(tIndices[i]);
		oItem["channel"] 	= WiFi.channel(tIndices[i]);
		oItem["enctype"] 	= WiFi.encryptionType(tIndices[i]);
		oItem["hidden"] 	= WiFi.isHidden(tIndices[i]) ? true : false;
	}
    Appl.MsgBus.sendEvent(this,MSG_WIFI_SCAN_RESULT,&oRootDoc,0);
	// sendJsonDocMessage(oRootDoc,this,nullptr);
	WiFi.scanDelete();
	DEBUG_FUNC_END();
}

#pragma endregion
