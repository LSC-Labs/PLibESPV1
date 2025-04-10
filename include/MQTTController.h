#pragma once
#include <Arduino.h>
#include <AsyncMqttClient.h>
#include <ConfigHandler.h>


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


class CMQTTController : public AsyncMqttClient, public IConfigHandler, public IStatusHandler{
    private:
        size_t m_nMessageBufferSize = 0;
        char * m_pszMessageBuffer = nullptr;
        unsigned long m_ulLastHeartBeat = 0;

    public:
        MQTTConfig Config;
        MQTTStatus Status;

        CMQTTController();
        void setup();

        void readConfigFrom(JsonObject &oCfg) override;
        void writeConfigTo(JsonObject &oCfg, bool bHideCritical) override;
        void writeStatusTo(JsonObject &oCfg) override;


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
