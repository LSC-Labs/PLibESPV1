// #pragma GCC diagnostic ignored "-Woverflow"
#ifndef DEBUG_LSC_WIFI
    #undef DEBUGINFOS
#endif

#include <Appl.h>
#include <Msgs.h>
#include <EventHandler.h>
#include <JsonNode.h>
#include <WiFiController.h>
#include <LSCUtils.h>

// IPAddress::isSet() exists on ESP8266 only
static inline bool isIPSet(const IPAddress &oIP) {
    #ifdef ESP32
        return((uint32_t) oIP != 0);
    #else
        return(oIP.isSet());
    #endif
}


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
const char * WIFI_PHY_MODE          = "phymode";
const char * WIFI_IPADDRESS         = "ipaddress";
const char * WIFI_SUBNETMASK        = "subnet";
const char * WIFI_GATEWAY           = "gwip";
const char * WIFI_DNS               = "dnsip";

namespace LSC_WIFI {

    /**
     * @brief Create the fallback SSID used by the device access point.
     *
     * The SSID is built from the configured prefix and the last four bytes of
     * the soft AP MAC address so that several devices can run side by side.
     */ 
    String getDefaultSSIDofAP()
    {
        uint8_t macAddr[6];
        WiFi.softAPmacAddress(macAddr);
        char ssid[strlen(WIFI_DEFAULT_AP_SSID_PREFIX) + 10];
        snprintf(ssid, sizeof(ssid), "%s-%02x%02x%02x%02x", WIFI_DEFAULT_AP_SSID_PREFIX, macAddr[2], macAddr[3], macAddr[4], macAddr[5]);
        return String(ssid);
    }
}

/**
 * @brief Create a WiFi controller instance with default configuration/status.
 */
CWiFiController::CWiFiController() {}



/**
 * @brief Return the default access point SSID generated for this device.
 */ 
String CWiFiController::getDefaultSSIDofAP()
{
    return(LSC_WIFI::getDefaultSSIDofAP());
}



#pragma region "Interface to Config / Status handling"

/**
 * @brief Convert an Arduino WiFi status code into a readable text.
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
#ifndef ESP32
        case WL_WRONG_PASSWORD:      return F("Wrong password");
#endif
        case WL_DISCONNECTED:        return F("Disconnected");
        default:                     return F("Unknown status");
    }
}

/**
 * @brief Write the current WiFi configuration into a JSON node.
 *
 * Passwords are masked when requested so status/config endpoints can expose the
 * structure without leaking credentials.
 */
void CWiFiController::writeConfigTo(JsonNode &oCfgNode,bool bHideCritical) {
    DEBUG_FUNC_START();
    // Hostname, Operation Mode and Fallback settings
    // oCfgNode["hostname"]        = Config.wifi_Hostname;
    oCfgNode[WIFI_AP_MODE]          = Config.accessPointMode;
    oCfgNode[WIFI_FALLBACK_MODE]    = Config.autoFallbackMode;

    // Access Point specific settings
    oCfgNode[WIFI_AP_SSID]          = Config.ap_ssid;
    oCfgNode[WIFI_AP_PASSWORD]      = (const char *) (bHideCritical ? WIFI_HIDDEN_PASSWORD : Config.ap_Password.c_str());
    oCfgNode[WIFI_AP_HIDDEN]        = Config.ap_hidden;
    oCfgNode[WIFI_AP_CHANNEL]       = Config.ap_channel;
    if(isIPSet(Config.ap_ipAddress))   oCfgNode[WIFI_AP_IPADDRESS] = Config.ap_ipAddress.toString().c_str(); 
    if(isIPSet(Config.ap_ipSubnetMask))oCfgNode[WIFI_AP_SUBNETMASK]= Config.ap_ipSubnetMask.toString().c_str();
    // WiFi specific settings
    oCfgNode[WIFI_SSID]            = Config.wifi_ssid;
    oCfgNode[WIFI_BSSID]           = getConfigBSSIDAsString(); //  Config.wifi_bssid;
    oCfgNode[WIFI_PASSWORD]        = (const char *) (bHideCritical ? WIFI_HIDDEN_PASSWORD : Config.wifi_Password.c_str());
    oCfgNode[WIFI_DHCP]            = Config.dhcpEnabled;
    oCfgNode[WIFI_PHY_MODE]        = Config.phyMode;

    if(isIPSet(Config.ipAddress))     oCfgNode[WIFI_IPADDRESS]   = Config.ipAddress.toString().c_str();
    if(isIPSet(Config.ipSubnetMask))  oCfgNode[WIFI_SUBNETMASK]  = Config.ipSubnetMask.toString().c_str();
    if(isIPSet(Config.ipDNS))         oCfgNode[WIFI_DNS]         = Config.ipDNS.toString().c_str();
    if(isIPSet(Config.ipGateway))     oCfgNode[WIFI_GATEWAY]     = Config.ipGateway.toString().c_str();
    DEBUG_FUNC_END();
}

/**
 * @brief Read WiFi configuration values from a JSON node.
 *
 * Masked password values are ignored to preserve the already configured secret.
 */
void CWiFiController::readConfigFrom(JsonNode &oCfgNode) {
    DEBUG_FUNC_START();
    DEBUG_JSON_OBJ(oCfgNode);
    // LSC::setValue(Config.wifi_Hostname,     oNode["hostname"]);
    oCfgNode.storeValueIf(WIFI_FALLBACK_MODE,   &  Config.autoFallbackMode);
    oCfgNode.storeValueIf(WIFI_AP_MODE,         &  Config.accessPointMode);
    oCfgNode.storeValueIf(WIFI_AP_HIDDEN,       &  Config.ap_hidden);
    int nChannel = Config.ap_channel;
    if(oCfgNode.storeValueIf(WIFI_AP_CHANNEL, &nChannel) && nChannel >= 1 && nChannel <= 13) {
        Config.ap_channel = nChannel;      // invalid channels would let every softAP() fail
    }
    oCfgNode.storeValueIf(WIFI_AP_SSID,            Config.ap_ssid);
    oCfgNode.storeValueIfNot(WIFI_AP_PASSWORD,     Config.ap_Password, WIFI_HIDDEN_PASSWORD);
    storeIPAddressIf(oCfgNode,WIFI_AP_IPADDRESS,   Config.ap_ipAddress);
    storeIPAddressIf(oCfgNode,WIFI_AP_SUBNETMASK,  Config.ap_ipSubnetMask);

    oCfgNode.storeValueIf(WIFI_SSID,               Config.wifi_ssid);
    oCfgNode.storeValueIfNot(WIFI_PASSWORD,        Config.wifi_Password, WIFI_HIDDEN_PASSWORD);

    const char *pszBSSID = oCfgNode.getValue(WIFI_BSSID);
    if(pszBSSID) {
        DEBUG_INFOS("WiFi: ---> setting BSSID : %s",pszBSSID);
        LSC::parseBytesToArray(Config.wifi_bssid, pszBSSID,':',sizeof(Config.wifi_bssid),16);
    }
    oCfgNode.storeValueIf(WIFI_DHCP,             & Config.dhcpEnabled);
    String strPhyMode;
    if(oCfgNode.storeValueIf(WIFI_PHY_MODE, strPhyMode)) {
        strPhyMode.toLowerCase();
        // Only valid values - otherwise the current mode is kept
        if(strPhyMode == "n" || strPhyMode == "g" || strPhyMode == "b") Config.phyMode = strPhyMode;
    }
    storeIPAddressIf(oCfgNode,WIFI_IPADDRESS,      Config.ipAddress);
    storeIPAddressIf(oCfgNode,WIFI_SUBNETMASK,     Config.ipSubnetMask);
    storeIPAddressIf(oCfgNode,WIFI_GATEWAY,        Config.ipGateway);
    storeIPAddressIf(oCfgNode,WIFI_DNS,            Config.ipDNS);
    DEBUG_FUNC_END();
} 


void CWiFiController::insertComponentDiscovery(const char * pszComponentName, JsonNode & oComponentArea, CMQTTController * pController) {
    String strCompID = Appl.getDeviceID();
    strCompID += "-wifi-rssi";
    (void) pController;
    char szValueTemplate[256];
    snprintf(szValueTemplate,sizeof(szValueTemplate),"{{ value_json.%s.rssi }}",pszComponentName ? pszComponentName : "");
    JsonNode * pSensor = oComponentArea.createObject("wifi-rssi");
    if(!pSensor) return;
    pSensor->setValue("p",                   "sensor");
    pSensor->setValue("device_class",        "signal_strength");
    pSensor->setValue("state_class",         "measurement");
    pSensor->setValue("entity_category",     "diagnostic");   // shown under "Diagnostic", not as main sensor
    pSensor->setValue("unit_of_measurement", "dBm");
    pSensor->setValue("value_template" ,     szValueTemplate);
    pSensor->setValue("unique_id",           strCompID.c_str());
}   

/**
 * @brief Read an IP address from a JSON node when the key exists.
 * @return true if the key existed and could be parsed into the target address.
 */
bool CWiFiController::storeIPAddressIf(JsonNode & oCfgNode,const char *pszKeyName, IPAddress & pTarget) {
    DEBUG_FUNC_START_PARMS("-,%s,%p",pszKeyName,(void *) &pTarget);
    bool bResult = false;
    JsonNode * pElement = oCfgNode.getElement(pszKeyName);
    if(pElement) {
        const char *pszValue = pElement->getValue();
        if(!pszValue || !*pszValue) {
            pTarget = IPAddress();          // empty value => address cleared (not set)
            bResult = true;
        } else {
            // parse into a temporary - fromString() may leave a partial address on errors
            IPAddress oParsed;
            bResult = oParsed.fromString(pszValue);
            if(bResult) pTarget = oParsed;
        }
    }
    DEBUG_FUNC_END_PARMS("%d",bResult);
    return(bResult);
}
/**
 * @brief Write the currently active WiFi state into a JSON status node.
 *
 * The method reports mode, connection timing, interface addresses, hostname,
 * MAC address and signal strength for either AP or station mode.
 */
void CWiFiController::writeStatusTo(JsonNode &oStatusNode, int nLevel) {
    DEBUG_FUNC_START();
    oStatusNode["accesspoint"] = Status.isInAccessPointMode;
    oStatusNode["stationmode"] = Status.isInStationMode;
    // Station mode: report the live state (the connection may be lost and re-established by the ESP)
    oStatusNode["isConnected"] = Status.isInStationMode ? WiFi.isConnected() : Status.isWiFiConnected;
    oStatusNode["startTime"]   = Status.startTimeInMillis;
    oStatusNode["stopTime"]    = Status.stopTimeInMillis;
    oStatusNode["restartTime"] = Status.restartTimeInMillis;
    oStatusNode["onlineTime"]  = Status.startTimeInMillis > 0 ? millis() - Status.startTimeInMillis : 0;
#ifdef ESP32
    IPAddress ipaddr, gwaddr, nmaddr;
    if ( Status.isInAccessPointMode) {
        oStatusNode["ssid"] = WiFi.softAPSSID().c_str();
        oStatusNode["mac"]  = WiFi.softAPmacAddress().c_str();
        ipaddr = WiFi.softAPIP();
        gwaddr = WiFi.softAPIP();
        nmaddr = WiFi.softAPSubnetMask();
    } else {
        oStatusNode["ssid"] = WiFi.SSID().c_str();
        oStatusNode["dns"]  = WiFi.dnsIP().toString().c_str();
        oStatusNode["mac"]  = WiFi.macAddress().c_str();
        ipaddr = WiFi.localIP();
        gwaddr = WiFi.gatewayIP();
        nmaddr = WiFi.subnetMask();
    }
#else
    struct ip_info oIPInfo;
    if ( Status.isInAccessPointMode)
	{
		wifi_get_ip_info(SOFTAP_IF, &oIPInfo);
		// oConf.ssid is not zero terminated for 32 char SSIDs - use the String API
		oStatusNode["ssid"]  = WiFi.softAPSSID().c_str();
		oStatusNode["mac"]   = WiFi.softAPmacAddress().c_str();
	}
	else
	{
		wifi_get_ip_info(STATION_IF, &oIPInfo);
		oStatusNode["ssid"] = WiFi.SSID().c_str();
		oStatusNode["dns"] = WiFi.dnsIP().toString().c_str();
		oStatusNode["mac"] = WiFi.macAddress().c_str();
	}

    IPAddress ipaddr = IPAddress(oIPInfo.ip.addr);
	IPAddress gwaddr = IPAddress(oIPInfo.gw.addr);
	IPAddress nmaddr = IPAddress(oIPInfo.netmask.addr);
#endif

    oStatusNode["ip"]       = ipaddr.toString().c_str(); // getAddressAsString(ipaddr);
	oStatusNode["gateway"]  = gwaddr.toString().c_str(); // getAddressAsString(gwaddr);
	oStatusNode["netmask"]  = nmaddr.toString().c_str(); //getAddressAsString(nmaddr);
    oStatusNode["hostname"] = WiFi.getHostname();
#ifndef ESP32
    WiFiPhyMode_t eMode = WiFi.getPhyMode();
    oStatusNode["phymode"]  = eMode == WIFI_PHY_MODE_11B ? "b" : (eMode == WIFI_PHY_MODE_11G ? "g" : "n");
#endif
    // RSSI is only meaningful for a station connection
    if(Status.isInStationMode && WiFi.isConnected()) oStatusNode["rssi"] = WiFi.RSSI();
    DEBUG_FUNC_END();
}

/**
 * @brief Serialize the current WiFi status and write it to the application log.
 */
void CWiFiController::writeStatusToLog() {
    JsonNode oStatusObj;
    writeStatusTo(oStatusObj,STATUS_LEVEL_INFO);    
    ApplLogVerboseWithParms(F("WiFi Status:\n%s"),oStatusObj.getAsJsonText());
}

#pragma endregion

/// @brief Listen to the application message bus and react to WiFi commands.
/// @param pSender Sender of the event (unused).
/// @param nMsgId Message id, currently only MSG_WIFI_SCAN is handled.
/// @param pMessage Optional event payload (unused).
/// @param nType Optional event type/class (unused).
/// @return EVENT_MSG_RESULT_OK => continue processing
int CWiFiController::receiveEvent(const void * pSender, int nMsgId, const void * pMessage, int nType) {
    switch(nMsgId) {
        case MSG_WIFI_SCAN : scanWiFi(); break;
        case MSG_APPL_LOOP :
            if(m_bScanRunning) {
                int nRC = WiFi.scanComplete();
                if(nRC != WIFI_SCAN_RUNNING) {
                    m_bScanRunning = false;
                    onWiFiScanResult(nRC);
                }
            }
            restartIfNeeded();      // scheduled reconnect (see startWiFi)
            break;
    }
    return(EVENT_MSG_RESULT_OK);
}


#pragma region "Starter for Access Point and Station Mode"

/** 
 * @brief Start access point mode.
 *
 * When configured AP startup fails, a deterministic fallback AP is started so
 * the device remains reachable for configuration.
 *
 * @param bUseConfigData true to use configured AP settings first.
 * @return true if AP mode is active.
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
 * @brief Start the ESP WiFi stack in access point mode with explicit settings.
 * @param pszSSID      SSID broadcast by this node.
 * @param ipAP         IPAddress of this node
 * @param ipSubnetAP   Subnet mask of this node. 
 * @param bHidden      if true, the SSID Network will be hidden...
 * @param pszPassword  Password for this Accesspoint
 * @return true if the access point was started successfully.
 * */
bool CWiFiController::startAccessPoint(const char * pszSSID, 
                                        IPAddress ipAP, 
                                        IPAddress ipSubnetAP,
                                        bool bHidden, 
                                        const char *pszPassword)
{
    DEBUG_FUNC_START();
    Status.isWiFiConnected = false;
    if(pszSSID != nullptr && *pszSSID) {
        ApplLogInfoWithParms(F("Starting WiFi Access Point: %s"),pszSSID);
        Appl.MsgBus.sendEvent(this,MSG_WIFI_STARTING,nullptr,WIFI_ACCESS_POINT_MODE);

        Status.isInStationMode      = false;
        Status.isInAccessPointMode  = true;
        Status.startTimeInMillis    = 0;
        Status.wifiStatus = WL_DISCONNECTED;

        WiFi.mode(WIFI_AP);
    
        // local-ip, gateway, subnet (fall back to defaults if not configured)
        if(!isIPSet(ipAP))       ipAP       = WIFI_MODULE_DEFAULT_AP_IP;
        if(!isIPSet(ipSubnetAP)) ipSubnetAP = WIFI_MODULE_DEFAULT_AP_SUBNET;
        WiFi.softAPConfig(ipAP, ipAP, ipSubnetAP);
        int nChannel = (Config.ap_channel >= 1 && Config.ap_channel <= 13) ? Config.ap_channel : 6;
        Status.isWiFiConnected = WiFi.softAP(pszSSID, 
                                            pszPassword, 
                                            nChannel, 
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
 * @brief Join an existing WiFi network in station mode.
 * @param pszSSID SSID to join
 * @param pszPassword Password to use
 * @param bSSID BSSID to use (if 0,0,0,0,0,0 - do not use BSSID)
 * @return true if the station connection was established before the timeout.
 */
bool CWiFiController::joinNetwork(const char *pszSSID, const char *pszPassword, byte bSSID[6])
{
    DEBUG_FUNC_START();
    if(!pszSSID)     pszSSID = "";
    if(!pszPassword) pszPassword = "";
    ApplLogInfoWithParms(F("Joining WiFi network %s"),pszSSID);
    Appl.MsgBus.sendEvent(this,MSG_WIFI_STARTING,nullptr,WIFI_STATION_MODE);

    Status.isInStationMode = true;
    Status.isInAccessPointMode = false;
    Status.isWiFiConnected = false;         // reset - otherwise a previous connection is reported
    Status.startTimeInMillis = 0;
    Status.restartTimeInMillis = WIFI_NO_RESTART;
    m_oRestartDelay.stop();

    WiFi.persistent(false);                 // before mode(), so nothing is written to flash
    WiFi.mode(WIFI_STA);
#ifndef ESP32
    // PHY mode before begin() - "g" avoids SDK crashes on HT/11n changes of the router
    WiFiPhyMode_t ePhyMode = WIFI_PHY_MODE_11N;
    if(Config.phyMode == "g")      ePhyMode = WIFI_PHY_MODE_11G;
    else if(Config.phyMode == "b") ePhyMode = WIFI_PHY_MODE_11B;
    if(!WiFi.setPhyMode(ePhyMode)) {
        ApplLogWarnWithParms(F("WiFi: PHY mode %s could not be set"), Config.phyMode.c_str());
    }
#endif

    if (!Config.dhcpEnabled)
    {
        if(isIPSet(Config.ipAddress) && isIPSet(Config.ipSubnetMask)) {
            DEBUG_INFOS(" - using static IP address %s", Config.ipAddress.toString().c_str());   
            WiFi.config(Config.ipAddress, Config.ipGateway, Config.ipSubnetMask, Config.ipDNS);
        } else {
            ApplLogWarn(F("WiFi: static IP configured without address/subnet - using DHCP"));
        }
    }
    bool useBSSID = false;
    for (int i = 0; bSSID && i < 6; i++)
    {
        if (bSSID[i] != 0) useBSSID = true;
    }
    if (useBSSID) {
        DEBUG_INFO(" - using fix bssid");
        WiFi.begin(pszSSID, pszPassword, 0, bSSID);
    }
    else {
        DEBUG_INFOS(" - begin connection to %s",pszSSID);
        WiFi.begin(pszSSID, pszPassword);
    }         
    
    // Blocking wait (max. 15s) - delay() yields, so the async TCP stack keeps running
    CSimpleDelay oTimeout(15000);
    while (!WiFi.isConnected() && !oTimeout.isDone()) {
        delay(500);
    }

    // The result of begin() is only the start state - the real reason is known now
    Status.wifiStatus = WiFi.status();
    if (WiFi.isConnected())
    {
        Status.isWiFiConnected  = true;
        Status.startTimeInMillis = millis();
        Status.nRetryConnectCounter = 0;
        Appl.MsgBus.sendEvent(this,MSG_WIFI_CONNECTED,nullptr,WIFI_STATION_MODE);
    } else {
        Appl.MsgBus.sendEvent(this,MSG_WIFI_ERROR,nullptr,Status.wifiStatus);
        ApplLogErrorWithParms(F(" - failed to connect to %s"),pszSSID);
        ApplLogErrorWithParms(F(" - status: %d (%s)"),(int) Status.wifiStatus,getStatusText(Status.wifiStatus).c_str());
    }
    DEBUG_FUNC_END_PARMS("%d",Status.isWiFiConnected);
    return(Status.isWiFiConnected );
}


/**
 * @brief Start WiFi using either default AP mode or the loaded configuration.
 * 
 * Station mode failures optionally fall back to AP mode or schedule a retry,
 * depending on the configured fallback/retry settings.
 *
 * @param bUseConfigData if false - start default AP, if true, use the loaded config data.
 */
void CWiFiController::startWiFi(bool bUseConfigData)
{
    DEBUG_FUNC_START_PARMS("%d",bUseConfigData);
    Status.restartTimeInMillis = WIFI_NO_RESTART;
    m_oRestartDelay.stop();
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
#ifdef ESP32
                if(Config.autoFallbackMode) {
#else
                if(Config.autoFallbackMode || Status.wifiStatus == WL_WRONG_PASSWORD ) {
#endif
                    Appl.Log.logInfo(F("... auto fallback to AP Mode"));
                    bIsConnected = startAccessPoint(true);
                }
                if(!bIsConnected) {
                    // If the connection has the wrong password... do not try to restart.
                    // Reconnect retry...
                    Appl.Log.logInfo(F("... trying reconnect to WiFi network in %d seconds"),Config.retryTimeoutSeconds);
                    unsigned long ulDelay = (unsigned long) (Config.retryTimeoutSeconds > 0 ? Config.retryTimeoutSeconds : 1) * 1000UL;
                    // restartTimeInMillis: end time for the status display only - timing by m_oRestartDelay
                    Status.restartTimeInMillis = m_oRestartDelay.restartWith(ulDelay);
                }
                if(bIsConnected) Status.nRetryConnectCounter = 0;
            }
        } 
    }
    writeStatusToLog();
    DEBUG_FUNC_END();
};

/** 
 * @brief Retry or disable WiFi when a scheduled reconnect is pending.
 * @return true if WiFi is connected after the check.
 */
bool CWiFiController::restartIfNeeded() {
    if(!Status.isWiFiConnected && m_oRestartDelay.isActive()) {
        if(m_oRestartDelay.isDone()) {
            m_oRestartDelay.stop();
            Status.restartTimeInMillis = WIFI_NO_RESTART;
            if(Status.nRetryConnectCounter < Config.retryCount) {
                Status.nRetryConnectCounter++;
                startWiFi(true);        // schedules the next retry if it fails again
            } else {
                ApplLogWarn(F("WiFi: max. reconnect attempts reached"));
                disableWiFi();
            }
        }
    }
    return Status.isWiFiConnected;
}

/**
 * @brief Disconnect station/AP mode and publish the corresponding bus events.
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
    Status.restartTimeInMillis = WIFI_NO_RESTART;
    m_oRestartDelay.stop();
    DEBUG_FUNC_END();
}

#pragma endregion

#pragma region "WiFi Scan"
/**
 * @brief Start an asynchronous WiFi scan.
 *
 * The result is delivered by onWiFiScanResult and broadcast via
 * MSG_WIFI_SCAN_RESULT.
 */
void CWiFiController::scanWiFi() {
    DEBUG_FUNC_START();
    // Start async (incl. hidden networks), the result is polled in MSG_APPL_LOOP.
    // Same on ESP32 and ESP8266 - keeps the JSON/MsgBus work in the loop context
    // instead of the WiFi event task (ESP32) / SYS context (ESP8266).
    // A request while a scan is running is answered by the running scan.
    if(!m_bScanRunning) {
        int nRC = WiFi.scanNetworks(true, true);
        m_bScanRunning = (nRC == WIFI_SCAN_RUNNING);
        if(!m_bScanRunning) onWiFiScanResult(nRC);
    }
    DEBUG_FUNC_END();
}

/**
 * @brief Build and send the JSON payload for an asynchronous scan result.
 *
 * Networks are sorted by signal strength and capped to the strongest entries to
 * keep the event payload small.
 */
void CWiFiController::onWiFiScanResult(int nNetworksFound) {
	DEBUG_FUNC_START_PARMS("%d",nNetworksFound);
    // Negative values are error codes (scan failed) - report an empty list
    if(nNetworksFound < 0) nNetworksFound = 0;
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
    CJsonNode oRootDoc;
	oRootDoc["command"] = "update";
	oRootDoc["data"] 	= "ssidlist";
    JsonNode * pScanResult = oRootDoc.createArray("payload");
	for (int i = 0; pScanResult && i < 10 && i < nNetworksFound; ++i)
	{
        JsonNode * pItem = pScanResult->createObject();
        pItem->setValue("ssid", WiFi.SSID(tIndices[i]).c_str());
		pItem->setValue("bssid", WiFi.BSSIDstr(tIndices[i]).c_str());
		pItem->setValue("rssi", WiFi.RSSI(tIndices[i]));
		pItem->setValue("channel", WiFi.channel(tIndices[i]));
		pItem->setValue("enctype", WiFi.encryptionType(tIndices[i]));
#ifdef ESP32
		pItem->setValue("hidden", WiFi.SSID(tIndices[i]).length() == 0);
#else
		pItem->setValue("hidden", WiFi.isHidden(tIndices[i]) ? true : false);
#endif
	}
    Appl.MsgBus.sendEvent(this,MSG_WIFI_SCAN_RESULT,&oRootDoc,0);
	// sendJsonDocMessage(oRootDoc,this,nullptr);
	WiFi.scanDelete();
	DEBUG_FUNC_END();
}

#pragma endregion
