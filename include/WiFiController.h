#pragma once
/**
 * @file WiFiController.h
 * @author Peter L. (LSC Labs)
 * @brief WiFi Controller to handle Access Point and Station Mode
 * @version 1.0
 * How to use:
 * - Instantiate the WiFi Controller "CWiFiController oWiFi" in the global scope
 * 
 * - Register the controller as a module to the application message bus,
 *   and the configuration / status handling:
 * 
 *      => CWiFiController oWiFi("wifi");
 * 
 *    -or-
 *      => CWiFiController oWiFi;
 *      => Appl.registerModule("wifi",&oWiFi);
 * 
 *    -or-
 *      => CWiFiController oWiFi;
 *      => Appl.MsgBus.registerEventReceiver(&oWiFi);
 *      => Appl.addConfigHandler("wifi",&oWiFi);
 *      => Appl.addStatusHandler("wifi",&oWiFi);
 *   
 * - Call startWiFi(true) to start the WiFi with the configuration data,
 *   this will enable the WiFi in Station mode, or in Access Point mode.
 *   or startWiFi(false) to start a default Access Point
 */
#include <Appl.h>
#include <Network.h>
#include <ArduinoJson.h>

#ifndef WIFI_MODULE_DEFAULT_AP_IP
    #define WIFI_MODULE_DEFAULT_AP_IP     IPAddress(192,168,4,1)
#endif
#ifndef WIFI_MODULE_DEFAULT_AP_SUBNET
    #define WIFI_MODULE_DEFAULT_AP_SUBNET IPAddress(255,255,255,0)
#endif
/**
 * Defines the default SSID prefix for the access point
 * Default "Smart Device"
 * Don't use too long prefixes, because the SSID is limited to 32 characters
 * and the addes unique MAC info is 8 characters long.
 */
#ifndef WIFI_DEFAULT_AP_SSID_PREFIX
    #define WIFI_DEFAULT_AP_SSID_PREFIX "LSCLabs"
#endif

#ifndef WIFI_DEFAULT_AP_SSID
    #define WIFI_DEFAULT_AP_SSID CWiFiController::getDefaultSSIDofAP()
#endif

#ifndef WIFI_HIDDEN_PASSWORD
    #define WIFI_HIDDEN_PASSWORD HIDDEN_PASSWORD_MASK
#endif

#ifndef WIFI_DEFAULT_PASSWORD
    #define WIFI_AP_DEFAULT_PASSWORD ""
#endif



namespace LSC_WIFI {
    String getDefaultSSIDofAP();
}

class CWiFiController;

/**
 *  @brief WiFi Configuration structure
 */
struct WiFiConfig {
    // Is in access point (true) or in station mode
    bool accessPointMode   = true;

    // Access Point (AP) Settings
    String    ap_ssid           = LSC_WIFI::getDefaultSSIDofAP();
    String    ap_Password       = WIFI_AP_DEFAULT_PASSWORD;
    int       ap_channel        = 6;
    bool      ap_hidden         = false;
    IPAddress ap_ipAddress      = WIFI_MODULE_DEFAULT_AP_IP;
    IPAddress ap_ipSubnetMask   = WIFI_MODULE_DEFAULT_AP_SUBNET;

    // Wifi Station (STA) settings
    String    wifi_ssid;                          // SSID to join
    byte      wifi_bssid[6]     = {0, 0, 0, 0, 0, 0};
    String    wifi_Password;                      // WiFi join Password

    bool      dhcpEnabled = true;   // Use DHCP or not 

    // Fix IP Address setttings - if dhcpEnabled is false
    IPAddress ipAddress;
    IPAddress ipSubnetMask;
    IPAddress ipGateway;
    IPAddress ipDNS;

    // If join to network does not work, fallback to access point mode...
    bool autoFallbackMode = false;  // Fallback to AP Mode when STA (normal access) fails

    // Retries to connect to the WiFi network Default 10 Days every 20 seconds
    int  retryTimeoutSeconds = 20;
    int  retryCount = 3*60*24*10;   
};

/// @brief WiFi Status structure
struct WiFiStatus {
    unsigned long startTimeInMillis = 0;  // When started...
    unsigned long stopTimeInMillis  = 0;  // When stopped...
    unsigned long restartTimeInMillis = -1; // When to restart the WiFi...

    bool isWiFiConnected = false;
    bool isInAccessPointMode = false;
    bool isInStationMode = false;
    // contains error code, if connection to WLAN fails
    wl_status_t wifiStatus = WL_DISCONNECTED;
    int nRetryConnectCounter = 0;
};

/**
 * @brief WiFi Controller class to handle the WiFi connection.
 * Is implemented as a module.
 *  */
class CWiFiController : public IModule { 
    private:
        WiFiConfig    Config;   // Configuration of the WiFi module

    public:
        WiFiStatus  Status;     // Status of the WiFi module
        static String getDefaultSSIDofAP();

    public:
        CWiFiController();
 
        void writeConfigTo( JsonObject &oCfgObj, bool bHideCritical) override;
        void readConfigFrom(JsonObject &oCfgObj) override;
        void writeStatusTo( JsonObject &oStatusObj) override;
        void writeStatusToLog();
    
        /**
         * @brief setup the WiFi
         * @param bUseConfigData - use current Config Data ,
         *                         if false, open an default Access Point.
         *                         if true, load the configuration before using this call.
         */
        void startWiFi(bool bUseConfigData); 

        bool startAccessPoint(bool bUseConfigData);

        bool restartIfNeeded();

        void scanWiFi();
        int receiveEvent(const void * pSender, int nMsgId, const void * pMessage, int nType);


        private:
            String getStatusText(int nWiFiStatus);
            void disableWiFi();
            
            bool  startAccessPoint( const char * pszSSID,
                                    IPAddress apip, 
                                    IPAddress apsubnet, 
                                    bool bHidden, 
                                    const char * pszPassword = NULL);

            bool joinNetwork(const char * pszSSID, 
                            const char * pszPassword, 
                            byte bSSID[6]);

            void onWiFiScanResult(int nNumber);
};


