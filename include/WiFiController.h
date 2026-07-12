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
#include <MQTTController.h>

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
    /**
     * @brief Create the fallback SSID used by the device access point.
     *
     * The SSID is built from the configured prefix and the last four bytes of
     * the soft AP MAC address so multiple devices remain distinguishable.
     */
    String getDefaultSSIDofAP();
}

class CWiFiController;

/// @brief Runtime configuration for access point and station mode.
struct WiFiConfig {
    /// @brief true for access point mode, false for station mode.
    bool accessPointMode   = true;

    /// @brief Access point SSID used when this device creates its own network.
    String    ap_ssid           = LSC_WIFI::getDefaultSSIDofAP();
    /// @brief Access point password.
    String    ap_Password       = WIFI_AP_DEFAULT_PASSWORD;
    /// @brief Access point WiFi channel.
    int       ap_channel        = 6;
    /// @brief true to hide the AP SSID from normal scans.
    bool      ap_hidden         = false;
    /// @brief Static IP address of the access point interface.
    IPAddress ap_ipAddress      = WIFI_MODULE_DEFAULT_AP_IP;
    /// @brief Subnet mask of the access point interface.
    IPAddress ap_ipSubnetMask   = WIFI_MODULE_DEFAULT_AP_SUBNET;

    /// @brief SSID to join in station mode.
    String    wifi_ssid;                          // SSID to join
    /// @brief Optional fixed BSSID to join; all zero means "do not pin BSSID".
    byte      wifi_bssid[6]     = {0, 0, 0, 0, 0, 0};
    /// @brief Password used when joining the configured station network.
    String    wifi_Password;                      // WiFi join Password

    /// @brief true to use DHCP in station mode.
    bool      dhcpEnabled = true;

    /// @brief Static station IP address used when DHCP is disabled.
    IPAddress ipAddress;
    /// @brief Static station subnet mask used when DHCP is disabled.
    IPAddress ipSubnetMask;
    /// @brief Static station gateway used when DHCP is disabled.
    IPAddress ipGateway;
    /// @brief Static station DNS server used when DHCP is disabled.
    IPAddress ipDNS;

    /// @brief true to fall back to AP mode when station mode cannot connect.
    bool autoFallbackMode = false;  // Fallback to AP Mode when STA (normal access) fails

    /// @brief Delay between station reconnect attempts in seconds.
    int  retryTimeoutSeconds = 20;
    /// @brief Maximum number of reconnect attempts before WiFi is disabled.
    int  retryCount = 3*60*24*10;   
};

/// @brief WiFi Status structure
struct WiFiStatus {
    /// @brief millis() value when WiFi/AP mode was started.
    unsigned long startTimeInMillis = 0;
    /// @brief millis() value when WiFi was stopped.
    unsigned long stopTimeInMillis  = 0;
    /// @brief millis() value for the next scheduled reconnect, or -1 if none.
    unsigned long restartTimeInMillis = -1;

    /// @brief true when the active WiFi mode is connected/started.
    bool isWiFiConnected = false;
    /// @brief true when the controller currently runs in AP mode.
    bool isInAccessPointMode = false;
    /// @brief true when the controller currently runs in station mode.
    bool isInStationMode = false;
    /// @brief Last Arduino WiFi status, including station connection errors.
    wl_status_t wifiStatus = WL_DISCONNECTED;
    /// @brief Number of reconnect attempts already performed.
    int nRetryConnectCounter = 0;
};

/**
 * @brief Application module that controls WiFi AP/station mode.
 *
 * The controller reads/writes its configuration through IConfigHandler, publishes
 * status through IStatusHandler and reacts to WiFi scan requests from the
 * application message bus.
 */
class CWiFiController : public IModule, public IHomeAssistantComponent { 
    private:
        String m_strBuffer;
        WiFiConfig    Config;   // Configuration of the WiFi module
        const char *getConfigBSSIDAsString() {
            char szTemp[4];
            bool bHasValues = false;
            bool bIsFirst = true;
            m_strBuffer = "";
            for(size_t nIdx = 0; nIdx < sizeof(Config.wifi_bssid); nIdx++) {
                if(Config.wifi_bssid[nIdx] != 0) bHasValues = true;
                snprintf(szTemp,sizeof(szTemp),"%02x",Config.wifi_bssid[nIdx]);
                if(!bIsFirst) m_strBuffer += ":";
                m_strBuffer += szTemp;
            }
            return(bHasValues ? m_strBuffer.c_str() : "");
        }

    public:
        WiFiStatus  Status;     // Status of the WiFi module

        /**
         * @brief Return the default access point SSID generated for this device.
         */
        static String getDefaultSSIDofAP();

    public:
        /**
         * @brief Create a WiFi controller instance with default configuration/status.
         */
        CWiFiController();
 
        /**
         * @brief Write the current WiFi configuration into a JSON node.
         * @param oCfgObj Target config node.
         * @param bHideCritical true to mask passwords.
         */
        void writeConfigTo( JsonNode &oCfgObj, bool bHideCritical) override;

        /**
         * @brief Read WiFi configuration values from a JSON node.
         * @param oCfgObj Source config node.
         */
        void readConfigFrom(JsonNode &oCfgObj) override;

        /**
         * @brief Write the currently active WiFi state into a JSON status node.
         * @param oStatusObj Target status node.
         * @param nLevel Requested status detail level.
         */
        void writeStatusTo( JsonNode &oStatusObj, int nLevel = STATUS_LEVEL_INFO) override;

        void insertComponentDiscovery(const char * pszComponentName, JsonNode & oComponentArea, CMQTTController * pController) override;    

        /**
         * @brief Serialize the current WiFi status and write it to the application log.
         */
        void writeStatusToLog();
    
        /**
         * @brief Start WiFi using either default AP mode or loaded configuration.
         * @param bUseConfigData false to start the default AP, true to use current config.
         */
        void startWiFi(bool bUseConfigData); 

        /**
         * @brief Start access point mode.
         * @param bUseConfigData true to use configured AP settings first.
         * @return true if AP mode is active.
         */
        bool startAccessPoint(bool bUseConfigData);

        /**
         * @brief Retry or disable WiFi when a scheduled reconnect is pending.
         * @return true if WiFi is connected after the check.
         */
        bool restartIfNeeded();

        /**
         * @brief Start an asynchronous WiFi scan.
         */
        void scanWiFi();

        /**
         * @brief Listen to the application message bus and react to WiFi commands.
         * @return EVENT_MSG_RESULT_OK to continue event processing.
         */
        int receiveEvent(const void * pSender, int nMsgId, const void * pMessage, int nType);


        private:
            /**
             * @brief Convert an Arduino WiFi status code into readable text.
             */
            String getStatusText(int nWiFiStatus);

            /**
             * @brief Disconnect station/AP mode and publish the corresponding bus events.
             */
            void disableWiFi();
            
            /**
             * @brief Start access point mode with explicit network settings.
             */
            bool  startAccessPoint( const char * pszSSID,
                                    IPAddress apip, 
                                    IPAddress apsubnet, 
                                    bool bHidden, 
                                    const char * pszPassword = NULL);

            /**
             * @brief Join an existing WiFi network in station mode.
             */
            bool joinNetwork(const char * pszSSID, 
                            const char * pszPassword, 
                            byte bSSID[6]);

            /**
             * @brief Build and send the JSON payload for an asynchronous scan result.
             */
            void onWiFiScanResult(int nNumber);

            /**
             * @brief Read an IP address from a JSON node when the key exists.
             */
            bool storeIPAddressIf(JsonNode & oCfgData,const char *pszKeyName, IPAddress & pTarget);
};

