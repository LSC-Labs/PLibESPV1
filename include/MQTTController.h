#pragma once
#include <Arduino.h>
#include <AsyncMqttClient.h>
#include <ApplModule.h>
#include <queue>


struct MQTTConfig {
    bool    isEnabled       = false;        // MQTT is enabled when true
    bool    useHA           = false;        // Use of Home Asisst
    bool    useAutoTopic    = false;        // Generate a automatic topic
    String  BrokerAddress;                  // Hostname or IP of the MQTT Server 
    int     BrokerPort = 1883;              // Portnumber of the MQTT Server
    String  ClientID;                       // ClientID of own station
    String  UserName;                       // User name for logon
    String  UserPassword;                   // Password for logon
    String  PublishTopic;                   // Topic name for publish (same as subscribe)
    String  SubscribeTopic;                 // Topic to subscribe
    int     PublishInterval = 60;           // Heart beat intervall... default 1 Minute.
};

struct MQTTStatus {
    unsigned long LastConStart;
    unsigned long ConEnd;
    bool          hasConError = false;
    AsyncMqttClientDisconnectReason  DisConReason;
    String                           DisConReasonString;
};

class MQTTMessage {
    public:

    /**
     * The received message from message broker..
     */
    char * Message;

    MQTTMessage(const char *pszMessage) {
        Message = strdup(pszMessage);
    }

    ~MQTTMessage() {
        if(Message) {
            free(Message);
        }
    }
};
/**
 * Message Broker as a module
 */
class CMQTTController : public AsyncMqttClient, public ApplModule {
    private:
        bool   m_bRealtimeMode = false;
        size_t m_nMessageBufferSize = 0;
        char * m_pszMessageBuffer = nullptr;
        char * m_pszLastWillTopic = nullptr;   // see last will - has to stay valid as long as mqtt client exists
        char * m_pszLastWillPayload = nullptr; // see Last will - has to stay valid as long as mqtt client exists
        unsigned long m_ulLastHeartBeat = 0;
        std::queue<MQTTMessage * > m_tMessageQeue; // FiFo queue with Message pointer...

    public:
        MQTTConfig Config;
        MQTTStatus Status;

        CMQTTController();
        ~CMQTTController();
        
        void setup();

        void readConfigFrom(JsonObject &oCfg) override;
        void writeConfigTo(JsonObject &oCfg, bool bHideCritical) override;
        void writeStatusTo(JsonObject &oCfg) override;
        int receiveEvent(const void * pSender, int nMsg, const void * pMessage, int nClass) override;
        void dispatch() override;

        bool isSessionActiv();
        void publishEvent(String strTopic, JsonDocument &oDataDoc,   bool bToHA = false);
        void publishEvent(String strTopic, JsonObject   &oDataNode,  bool bToHA = false);
        void publishEvent(const char *pszTopic, const char *pszData,  bool bToHA = false);
        virtual void publishHeartBeat(bool bForceSend = false);
        virtual void publishBoot();
    protected:
        void onMqttConnect(bool sessionPresent);
        void onMqttDisconnect(AsyncMqttClientDisconnectReason reason);
        void onMqttMessage(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total);
        
};
