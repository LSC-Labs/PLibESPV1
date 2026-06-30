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


struct MQTTConfig {
    bool    isEnabled       = false;        // MQTT is enabled when true
    bool    useAutoTopic    = false;        // Generate a automatic topic
    String  BrokerAddress;                  // Hostname or IP of the MQTT Server 
    int     BrokerPort = 1883;              // Portnumber of the MQTT Server
    String  ClientID;                       // ClientID of own station
    String  UserName;                       // User name for logon
    String  UserPassword;                   // Password for logon
    String  PublishTopicPrefix;             // Topic Prefix to publish device information
    int     PublishInterval = 60;           // Heart beat intervall... default 1 Minute.

    /************* Home Assistant Config Area *****************/
    bool    useHA             = false;              // Use of Home Asisst
    String  HADiscoveryPrefix = "/homeassistant";   // Home Assistant Discovery Prefix
    String  HAOnlineMessage   = "online";           // message when going online
    String  HAOfflineMessage  = "offline";          // message when ha is offline

};

struct MQTTStatus {
    unsigned long ConStart      = 0;       // Connection started
    unsigned long ConEnd        = 0;       // Connection ended (could be more recent than ConStart)
    AsyncMqttClientDisconnectReason  DisConReason = AsyncMqttClientDisconnectReason::TCP_DISCONNECTED;
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



/**
 * Message Broker as a module
 * sends a heart beat as long as PublishInterval is set to > 0
 */
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
        MQTTConfig Config;
        MQTTStatus Status;

        CMQTTController();
        ~CMQTTController();
        
        void setup(int nPublishInterval = 60);
        void readConfigFrom(JsonNode &oCfg) override;
        void writeConfigTo( JsonNode &oCfg, bool bHideCritical) override;
        void writeStatusTo( JsonNode &oCfg, int nLevel) override;
        int  receiveEvent(const void * pSender, int nMsg, const void * pMessage, int nClass) override;
        void dispatch() override;

        bool enableConnection();
        
        bool isDeviceTopic(const char *pszTopic);
        bool isDeviceCommandTopic(const char *pszTopic);

        const char * getDeviceCommandBaseTopicPath();
        
        void publishDeviceState(JsonNode & oStateData);
        void publishDeviceState(const char * pszStateData);
        void publishDeviceTopic(String strTopic,      JsonNode     &oDataNode,   int nQOS = 0, bool bRetain = false);
        void publishDeviceTopic(const char *pszTopic, JsonNode     &oDataNode,   int nQOS = 0, bool bRetain = false);
        void publishDeviceTopic(const char *pszTopic, const char   *pszData,     int nQOS = 0, bool bRetain = false);
        virtual void publishHeartBeat(bool bForceSend = false);
    protected:
        void onMqttConnect(bool sessionPresent);
        void onMqttDisconnect(AsyncMqttClientDisconnectReason reason);
        void onMqttMessage(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total);

    /******************************* Home Assistant Section *********************************/
    protected:
        char * m_pszHomeAssistantStatusTopic = nullptr;
        void publishHomeAssistantDiscovery();      
    
    public:
        void registerHomeAssistantComponent(const char * pszName, IHomeAssistantComponent * pHandler);
};


class MQTTMessage {
    public:

    CMQTTController  * pController;
    /**
     * The received message from message broker..
     */
    char * Message;
    char * Topic;

    MQTTMessage(const char * pszTopic, const char *pszMessage, CMQTTController * pController = nullptr) : pController(pController) {
        Topic = strdup(pszTopic);
        Message = strdup(pszMessage);
    }

    ~MQTTMessage() {
        if(Message) {
            free(Message);
        }
        if(Topic) {
            free(Topic);
        }
    }

    bool isDeviceTopic() {
        return pController ? pController->isDeviceTopic(Topic) : false;
    }
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
