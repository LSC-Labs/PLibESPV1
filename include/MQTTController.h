#pragma once
#include <Runtime.h>
#ifdef NATIVE_RUNTIME
    #include <JsonNode.h>

    enum class AsyncMqttClientDisconnectReason {
        TCP_DISCONNECTED,
        MQTT_UNACCEPTABLE_PROTOCOL_VERSION,
        MQTT_IDENTIFIER_REJECTED,
        MQTT_SERVER_UNAVAILABLE,
        MQTT_MALFORMED_CREDENTIALS,
        MQTT_NOT_AUTHORIZED,
        ESP8266_NOT_ENOUGH_SPACE
    };

    struct AsyncMqttClientMessageProperties {};

    class AsyncMqttClient {
        public:
            bool connected() { return(false); }
            void connect() {}
    };
#else
    #include <AsyncMqttClient.h>
#endif
#include <ApplModule.h>
#include <NamedValueTable.h>
#include <SimpleDelay.h>

#include <queue>


#define MQTT_STATE_AVAILABLE       "online"
#define MQTT_STATE_UNAVAILABLE     "offline"
#define MQTT_STATE_BOOTING         "booting"

class MQTTMessage;
class CMQTTController;


/// @brief Runtime configuration for the MQTT broker connection and topics.
struct MQTTConfig {
    /// @brief true when MQTT should connect/publish.
    bool    isEnabled       = false;
    /// @brief true to generate the publish topic from device data.
    bool    useAutoTopic    = false;
    /// @brief Hostname or IP address of the MQTT broker.
    String  BrokerAddress;
    /// @brief TCP port of the MQTT broker.
    int     BrokerPort = 1883;
    /// @brief MQTT client id used by this device.
    String  ClientID;
    /// @brief MQTT broker username.
    String  UserName;
    /// @brief MQTT broker password.
    String  UserPassword;
    /// @brief Base topic used for device state/command topics.
    String  PublishTopicPrefix;
    /// @brief Heartbeat interval in seconds; 0 disables periodic heartbeat.
    int     PublishInterval = 60;

    /************* Home Assistant Config Area *****************/
    /// @brief true to publish Home Assistant MQTT discovery data.
    bool    useHA             = false;              // Use of Home Asisst
    /// @brief MQTT discovery prefix used by Home Assistant.
    String  HADiscoveryPrefix = "/homeassistant";   // Home Assistant Discovery Prefix
    /// @brief Home Assistant availability message for online.
    String  HAOnlineMessage   = "online";           // message when going online
    /// @brief Home Assistant availability message for offline.
    String  HAOfflineMessage  = "offline";          // message when ha is offline

};

/// @brief Runtime status of the MQTT connection.
struct MQTTStatus {
    /// @brief millis() value when connection started.
    unsigned long ConStart      = 0;       // Connection started
    /// @brief millis() value when connection ended.
    unsigned long ConEnd        = 0;       // Connection ended (could be more recent than ConStart)
    /// @brief Last disconnect reason reported by AsyncMqttClient.
    AsyncMqttClientDisconnectReason  DisConReason = AsyncMqttClientDisconnectReason::TCP_DISCONNECTED;
    /// @brief Text representation of the last disconnect reason.
    String                           DisConReasonString;
};

/**
 * Implement if needed
 * 
 */
class IHomeAssistantComponent {
    public:
        /**
         * insert your component registration data of HA AutoDiscovery (device).
         * When called, the component area is already created and you can insert your component data here.
         * ToDo: If your component has a discovery information, create a json object with the component name and insert
         *        the discovery information there. 
         * @see https://www.home-assistant.io/integrations/mqtt/#mqtt-discovery
         * 
         * This function enables you to access the full device discovery information and the active message broker controller.
         * Use the Message Broker controller to get further information like the command topic path...
         * @param pszComponentName Name of the component - same as registering this interface
         * @param oComponentArea JSON object representing the component area "cmps" in the HA discovery information. Insert your component information here.
         * @param pController Pointer to the MQTT controller
         */
        virtual void insertComponentDiscovery(const char * pszComponentName, JsonNode & oComponentArea, CMQTTController * pController) = 0;
};



/// @brief MQTT message broker module with heartbeat and HA discovery support.
class CMQTTController : public AsyncMqttClient, public ApplModule {
    private:
        size_t m_nMessageBufferSize          = 0;
        char * m_pszMessageBuffer            = nullptr;
        char * m_pszPublishStateTopic        = nullptr;
        char * m_pszDeviceCommandTopics      = nullptr;
        char * m_pszPublishAvailabilityTopic = nullptr;

        unsigned long m_ulLastHeartBeat = 0;
        std::queue<MQTTMessage * > m_tMessageQeue; // FiFo queue with Message pointer...
        CNamedValueTable<IHomeAssistantComponent *> m_tComponentHandlerByName;
        CSimpleDelay m_oTryConnectDelay;


    public:
        /// @brief MQTT connection and topic configuration.
        MQTTConfig Config;
        /// @brief Current MQTT connection status.
        MQTTStatus Status;

        /// @brief Create the MQTT controller with default config/status.
        CMQTTController();
        /// @brief Release owned topic/message buffers.
        ~CMQTTController();
        
        /// @brief Configure MQTT callbacks and heartbeat interval.
        void setup(int nPublishInterval = 60);
        /// @brief Read MQTT configuration from a JSON node.
        void readConfigFrom(JsonNode &oCfg) override;
        /// @brief Write MQTT configuration into a JSON node.
        void writeConfigTo( JsonNode &oCfg, bool bHideCritical) override;
        /// @brief Write MQTT connection status into a JSON node.
        void writeStatusTo( JsonNode &oCfg, int nLevel) override;
        /// @brief React to application/network lifecycle events.
        int  receiveEvent(const void * pSender, int nMsg, const void * pMessage, int nClass) override;
        /// @brief Dispatch queued MQTT messages and heartbeat work.
        void dispatch() override;

        /// @brief Return true if MQTT is enabled and has enough config to connect.
        bool enableConnection();
        
        /// @brief Return true if the topic belongs to this device topic prefix.
        bool isDeviceTopic(const char *pszTopic);
        /// @brief Return true if the topic is a command topic for this device.
        bool isDeviceCommandTopic(const char *pszTopic);

        /// @brief Return the base topic path used for device commands.
        const char * getDeviceCommandBaseTopicPath();
        
        /// @brief Publish the current device state from a JSON node.
        void publishDeviceState(JsonNode & oStateData);
        /// @brief Publish the current device state from serialized JSON/text.
        void publishDeviceState(const char * pszStateData);
        /// @brief Publish a JSON node below the device topic prefix.
        void publishDeviceTopic(String strTopic,      JsonNode     &oDataNode,   int nQOS = 0, bool bRetain = false);
        /// @brief Publish a JSON node below the device topic prefix.
        void publishDeviceTopic(const char *pszTopic, JsonNode     &oDataNode,   int nQOS = 0, bool bRetain = false);
        /// @brief Publish text below the device topic prefix.
        void publishDeviceTopic(const char *pszTopic, const char   *pszData,     int nQOS = 0, bool bRetain = false);
        /// @brief Publish availability/heartbeat messages when due or forced.
        virtual void publishHeartBeat(bool bForceSend = false);
    protected:
        /// @brief AsyncMqttClient connect callback.
        void onMqttConnect(bool sessionPresent);
        /// @brief AsyncMqttClient disconnect callback.
        void onMqttDisconnect(AsyncMqttClientDisconnectReason reason);
        /// @brief AsyncMqttClient message callback.
        void onMqttMessage(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total);

    /******************************* Home Assistant Section *********************************/
    protected:
        char * m_pszHomeAssistantStatusTopic = nullptr;
        /// @brief Publish Home Assistant discovery information for registered components.
        void publishHomeAssistantDiscovery();      
    
    public:
        /// @brief Register a component handler for Home Assistant discovery.
        void registerHomeAssistantComponent(const char * pszName, IHomeAssistantComponent * pHandler);
};


/// @brief Owns one received MQTT topic/payload pair until dispatch.
class MQTTMessage {
    public:

    /// @brief Controller used for topic helper checks; may be nullptr in tests.
    CMQTTController  * pController;
    /// @brief Received message payload from the message broker.
    char * Message;
    /// @brief Topic that carried the received message.
    char * Topic;

    char * DeviceCmdTopic; // Command topic for this device, if the message is a command topic.

    /// @brief Copy topic and payload into an owned message object.
    MQTTMessage(const char * pszTopic, const char *pszMessage, CMQTTController * pController = nullptr) : pController(pController) {
        Topic = strdup(pszTopic);
        Message = strdup(pszMessage);
        if(isDeviceCommandTopic()) {
            String strDeviceCmdTopic = pszTopic;
            size_t nPublishTopicLength = pController ? strlen(pController->getDeviceCommandBaseTopicPath()) : 0;
            strDeviceCmdTopic.remove(0,nPublishTopicLength + 1); // Remove the command topic prefix and the following slash 
            DeviceCmdTopic = strdup(strDeviceCmdTopic.c_str());
        } else {
            DeviceCmdTopic = nullptr;
        }
    }

    /// @brief Release copied topic and payload strings.
    ~MQTTMessage() {
        if(Message) {
            free(Message);
        }
        if(Topic) {
            free(Topic);
        }
    }

    /// @brief Return true if this message topic belongs to the device prefix.
    bool isDeviceTopic() {
        return pController ? pController->isDeviceTopic(Topic) : false;
    }
    /// @brief Return true if this message topic is a command topic.
    bool isDeviceCommandTopic() {
        return pController ? pController->isDeviceCommandTopic(Topic) : false;
    }
};

#ifdef NATIVE_RUNTIME
inline bool CMQTTController::isDeviceTopic(const char *pszTopic) {
    bool bIsDeviceTopic = false;
    if(pszTopic) {
        size_t nPublishTopicLength = Config.PublishTopicPrefix.length();
        if(strlen(pszTopic) > nPublishTopicLength && pszTopic[nPublishTopicLength] == '/') {
            String strTopicPrefix(pszTopic,nPublishTopicLength);
            bIsDeviceTopic = LSC::stricmp(strTopicPrefix.c_str(),Config.PublishTopicPrefix.c_str()) == 0;
        }
    }
    return(bIsDeviceTopic);
}

inline bool CMQTTController::isDeviceCommandTopic(const char *pszTopic) {
    bool bIsCommandTopic = false;
    if(isDeviceTopic(pszTopic)) {
        size_t nPublishTopicLength = Config.PublishTopicPrefix.length();
        bIsCommandTopic = LSC::stricmp(&pszTopic[nPublishTopicLength],"/cmd") == 0 ||
                          strncmp(&pszTopic[nPublishTopicLength],"/cmd/",5) == 0;
    }
    return(bIsCommandTopic);
}
#endif
