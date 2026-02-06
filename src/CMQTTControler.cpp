#ifndef DEBUG_LSC_MQTT
    #undef DEBUGINFOS
#endif

#include <Arduino.h>
#include <ArduinoJson.h>
#include <Appl.h>
#include <Network.h>
#include <MQTTController.h>
#include <LSCUtils.h>
#include <Msgs.h>


#define MQTT_RECV_MESSAGE_BUFFER_SIZE 2048

const char * MQTT_HIDDEN_PASSWORD       = "******";
const char * MQTT_CONFIG_ENABLED        = "enabled";
const char * MQTT_CONFIG_BROCKERPORT    = "port";
const char * MQTT_CONFIG_BROCKERADDRESS = "host";
const char * MQTT_CONFIG_USERNAME       = "user";
const char * MQTT_CONFIG_USERPASSWORD   = "passwd";
const char * MQTT_CONFIG_PUBLISHTOPIC   = "topic";
const char * MQTT_CONFIG_PUBLISHINTERVAL= "syncrate";
const char * MQTT_CONFIG_USEAUTOTOPIC   = "autotopic";
const char * MQTT_CONFIG_USEHA          = "useha";


// Messages for Home Assistance
// -> https://github.com/home-assistant/core/blob/dev/homeassistant/components/mqtt/abbreviations.py
#define HA_MSG_AVAILABILITY "/avty"

#pragma region Constructor and configuration

CMQTTController::CMQTTController() {
       
}

CMQTTController::~CMQTTController() {
    if(m_pszLastWillPayload) free(m_pszLastWillPayload);
    if(m_pszLastWillTopic) free(m_pszLastWillTopic);
    
}
/**
 * The session is activ, if MQTT is enabled by user settings
 * and the session is established and connected..
 * If it is not connected but enabled, the session will be started.
 * If a error occured, try to reconnect, if the last error is not critical for 
 * the connection.
 */
bool CMQTTController::isSessionActiv() {
    if(Config.isEnabled) {
        if(!connected()) {
            // If not connected, try to connect now...
            // if failed, wait 5 secondes to avoid DOS
            if(Status.LastConStart == 0) connect();
            else if(Status.ConEnd + 5000 < millis()) { 
                bool bTryConnect = true;
                switch(Status.DisConReason) {
                    case AsyncMqttClientDisconnectReason::MQTT_NOT_AUTHORIZED: 
                    case AsyncMqttClientDisconnectReason::MQTT_MALFORMED_CREDENTIALS: 
                    case AsyncMqttClientDisconnectReason::MQTT_UNACCEPTABLE_PROTOCOL_VERSION:
                            bTryConnect = false;
                            break;
                    default: // keep the compiler quiet.
                            break;
                }
                if(bTryConnect) {
                    Appl.MsgBus.sendEvent(this,MSG_MQTT_STARTING,this,0);
                    connect();
                }
            }
        }
    }
    return(Config.isEnabled && connected());
}

void CMQTTController::readConfigFrom(JsonObject &oCfg) {
    DEBUG_FUNC_START();
    DEBUG_JSON_OBJ(oCfg);

    LSC::setJsonValue(oCfg,MQTT_CONFIG_ENABLED,         &   Config.isEnabled);
    LSC::setJsonValue(oCfg,MQTT_CONFIG_USEHA,           &   Config.useHA);;

    LSC::setJsonValue(oCfg,MQTT_CONFIG_BROCKERPORT,     &   Config.BrokerPort);
    LSC::setJsonValue(oCfg,MQTT_CONFIG_BROCKERADDRESS,      Config.BrokerAddress);
    LSC::setJsonValue(oCfg,MQTT_CONFIG_USERNAME,            Config.UserName);
    LSC::setJsonValue(oCfg,MQTT_CONFIG_PUBLISHTOPIC,        Config.PublishTopic);
    LSC::setJsonValue(oCfg,MQTT_CONFIG_PUBLISHINTERVAL, &   Config.PublishInterval);
    LSC::setJsonValue(oCfg,MQTT_CONFIG_USEAUTOTOPIC,    &   Config.useAutoTopic);

    LSC::setJsonValueIfNot(oCfg,MQTT_CONFIG_USERPASSWORD,Config.UserPassword, MQTT_HIDDEN_PASSWORD);
    Config.SubscribeTopic = Config.PublishTopic;
}

void CMQTTController::writeConfigTo(JsonObject &oCfg, bool bHideCritical) {
    DEBUG_FUNC_START();
    if(oCfg) {
        oCfg[MQTT_CONFIG_ENABLED]           = Config.isEnabled;
        oCfg[MQTT_CONFIG_BROCKERPORT]       = Config.BrokerPort;
        oCfg[MQTT_CONFIG_BROCKERADDRESS]    = Config.BrokerAddress;
        oCfg[MQTT_CONFIG_USERNAME]          = Config.UserName;
        oCfg[MQTT_CONFIG_USERPASSWORD]      = bHideCritical ? MQTT_HIDDEN_PASSWORD : Config.UserPassword;
        oCfg[MQTT_CONFIG_PUBLISHTOPIC]      = Config.PublishTopic;
        oCfg[MQTT_CONFIG_USEAUTOTOPIC]      = Config.useAutoTopic;
        oCfg[MQTT_CONFIG_PUBLISHINTERVAL]   = Config.PublishInterval;
        oCfg[MQTT_CONFIG_USEAUTOTOPIC]      = Config.useAutoTopic;
        oCfg[MQTT_CONFIG_USEHA]             = Config.useHA;
    }
    DEBUG_JSON_OBJ(oCfg);
    
}

/**
 * write the status
 */
void CMQTTController::writeStatusTo(JsonObject &oStatus) {
    oStatus["isEnabled"]    = Config.isEnabled;
    oStatus["isConnected"]  = connected();
    oStatus["started"] = Status.LastConStart;
    if(!connected() && Status.hasConError) {
        oStatus["disconTS"]        = Status.ConEnd;
        oStatus["disconReasonRC"]  = (int) Status.DisConReason;
        oStatus["disconReason"]    = Status.DisConReasonString;
    }
}
#pragma endregion

#pragma region Publish functions like a JsonDocument to the Message Broker
/**
 * publish a json document on the message broker.
 */
void CMQTTController::publishEvent(String strTopic, JsonDocument &oData, bool bToHA)
{
    // JsonObject oNode = oData.as<JsonObject>();
    JsonObject oNode = GetJsonDocumentAsObject(oData);
    publishEvent(strTopic, oNode,bToHA);
}

/**
 * publish an JsonObject on the Message Broker
 */
void CMQTTController::publishEvent(String strTopic, JsonObject &oData,  bool bToHA)
{
	if (isSessionActiv())
	{
		String mqttBuffer;
		serializeJson(oData, mqttBuffer);
        publishEvent(strTopic.c_str(), mqttBuffer.c_str(), bToHA);
	}
}

/**
 * Publish a string on the Message Broker
 */
void CMQTTController::publishEvent(const char *pszTopic, const char * pszData, bool bToHA)
{
    DEBUG_FUNC_START_PARMS("\"%s\",\"%s\"",NULL_POINTER_STRING(pszTopic),NULL_POINTER_STRING(pszData));
	if (isSessionActiv() && pszData && pszTopic)
	{
        if(bToHA) {
            if(Config.useHA) {
                String strMsgTopic = "homeassistant/";
                strMsgTopic += pszTopic;
                publish(strMsgTopic.c_str(), 0, false, pszData);
            }
        } else {
            String strMsgTopic(Config.PublishTopic + "/" + pszTopic);
            publish(strMsgTopic.c_str(), 0, false, pszData);
        }
	}
}

/**
 * Publish a book information and the status
 */
void CMQTTController::publishBoot()
{
    if(isSessionActiv()) {
        publishEvent("app",Appl.AppName.c_str());
        publishEvent("version",Appl.AppVersion.c_str());
        publishEvent("ip",WiFi.localIP().toString().c_str());
        publishEvent("status","booting");

        JSON_DOC_STATIC(oData,512);        
        oData["type"]       = "boot";
        oData["clientid"]   = this->getClientId();
        oData["prog"]       = Appl.AppName;
        oData["version"]    = Appl.AppVersion;
        oData["time"]       = Appl.StartTime;
        oData["uptime"]     = Appl.getUpTime();
        oData["ip"]         = WiFi.localIP().toString();
        publishEvent(String("info"),oData);
    }
}

/**
 * Publish a heart beat, when publish interval is set
 */
void CMQTTController::publishHeartBeat(bool bForceSend) {
    if(isSessionActiv() && Config.PublishInterval > 0) {
        // Immediatly, if it is the first time - otherwise use the user publish intervall...
        unsigned long ulNextPublish = 0;
        if(m_ulLastHeartBeat > 0) ulNextPublish = (Config.PublishInterval * 1000) + m_ulLastHeartBeat;
        // Time to start ?
        if(bForceSend || ulNextPublish < millis()) {
            DEBUG_INFOS("MQTT sending hartbeat... %d",Config.PublishInterval);
            String strUpTime = Appl.getUpTime();
            publishEvent("uptime",strUpTime.c_str());
            publishEvent("status","online");
            JSON_DOC_STATIC(oHeartBeat,256);
            // Send Json Object with details...
            oHeartBeat["type"]      = "heartbeat";
            oHeartBeat["clientid"]  = this->getClientId();
            oHeartBeat["time"]      = millis();
            oHeartBeat["uptime"]    = strUpTime;
            oHeartBeat["ip"]        = WiFi.localIP().toString();
            publishEvent(String("info"),oHeartBeat);
            m_ulLastHeartBeat = millis();
        }
    }
}
#pragma endregion

/**
 * Trigger, when message broker connected
 */
void CMQTTController::onMqttConnect(bool sessionPresent)
{
    DEBUG_FUNC_START_PARMS("%d",sessionPresent);
    Status.DisConReason = AsyncMqttClientDisconnectReason::TCP_DISCONNECTED;
    Status.DisConReasonString = "";
    Status.LastConStart = millis();
	publishBoot();
	String strTopic(Config.SubscribeTopic);
    if(strTopic.length() > 0) {
        strTopic += "/cmd";
        subscribe(strTopic.c_str(), 2);
    }
    ApplLogInfoWithParms("MQTT session established (%d)",sessionPresent);
    Appl.MsgBus.sendEvent(this,MSG_MQTT_CONNECTED,this,0);

}

/**
 * Trigger, when message broker disconnected
 */
void CMQTTController::onMqttDisconnect(AsyncMqttClientDisconnectReason oReason)
{
    Status.ConEnd = millis();
	Status.DisConReason = oReason;
	switch (oReason)
	{
        case (AsyncMqttClientDisconnectReason::TCP_DISCONNECTED):
            Status.DisConReasonString = F("TCP Disconnected");
            break;
        case (AsyncMqttClientDisconnectReason::MQTT_UNACCEPTABLE_PROTOCOL_VERSION):
            Status.DisConReasonString  = F("MQTT unacceptable protocol version");
            break;
        case (AsyncMqttClientDisconnectReason::MQTT_IDENTIFIER_REJECTED):
            Status.DisConReasonString  = F("MQTT identifier rejected");
            break;
        case (AsyncMqttClientDisconnectReason::MQTT_SERVER_UNAVAILABLE):
            Status.DisConReasonString  = F("MQTT server unavailable");
            break;
        case (AsyncMqttClientDisconnectReason::MQTT_MALFORMED_CREDENTIALS):
            Status.DisConReasonString  = F("MQTT malformed credentials");
            break;
        case (AsyncMqttClientDisconnectReason::MQTT_NOT_AUTHORIZED):
            Status.DisConReasonString  = F("MQTT not authorized");
            break;
        case (AsyncMqttClientDisconnectReason::ESP8266_NOT_ENOUGH_SPACE):
            Status.DisConReasonString  = F("ESP not enough space");
            break;
        default:
            Status.DisConReasonString  = F("Unknown");
            break;
	}
    Appl.MsgBus.sendEvent(this,MSG_MQTT_DISCONNECTED,this,(int) oReason);
    ApplLogInfoWithParms("MQTT disconnected (%d) - %s",
                          oReason,
                          Status.DisConReasonString.c_str());
}

/**
 * A message received from the message broker...
 * - remember, you are in an interrupt - so hurry up!
 */
void CMQTTController::onMqttMessage(char *topic, char *pszPayload, AsyncMqttClientMessageProperties properties, size_t nLen, size_t nIndex, size_t nTotal)
{
    DEBUG_FUNC_START_PARMS("%s,..,..,%d,%d,%d",
                            NULL_POINTER_STRING(topic),
                            nLen,nIndex,nTotal);

    // Allocate message buffer if needed
    if(m_pszMessageBuffer == nullptr) {
        m_pszMessageBuffer = (char *) malloc(nTotal +2 );
        if(m_pszMessageBuffer) {
            memset(m_pszMessageBuffer,'\0',nTotal +2 );
            m_nMessageBufferSize = nTotal;
        }
    } 
    // Move data to new allocated buffer (blockwise)
    if((nIndex + nLen) <= m_nMessageBufferSize ) {
        memcpy(&m_pszMessageBuffer[nIndex],pszPayload,nLen);
    }
    // Last Block received ? => store data in queue
    if(nIndex + nLen == nTotal) {
        DEBUG_INFOS("MQTT final message received %s",m_pszMessageBuffer);
        MQTTMessage *pMessage = new MQTTMessage(m_pszMessageBuffer);
        if(m_bRealtimeMode) Appl.MsgBus.sendEvent(this,MSG_MQTT_MSG_RECEIVED,pMessage,nTotal);
        else m_tMessageQeue.push(pMessage);
        free(m_pszMessageBuffer);
        m_pszMessageBuffer = nullptr;
        m_nMessageBufferSize = 0;
    }
}

void CMQTTController::dispatch() {
    publishHeartBeat(false);
    while(!m_tMessageQeue.empty()) {
        MQTTMessage * pMessage = m_tMessageQeue.front();
        Appl.MsgBus.sendEvent(this,MSG_MQTT_MSG_RECEIVED,pMessage,0);
        m_tMessageQeue.pop();
        delete pMessage;
    }
}

int CMQTTController::receiveEvent(const void * pSender, int nMsg, const void * pMessage, int nClass) {
    // Call the base class receiver first (calls dispatch on MSG_APPL_LOOP)
    int nResult = ApplModule::receiveEvent(pSender,nMsg,pMessage,nClass);
    switch(nMsg) {
        /*
        // Send a heart beat on loop message
        case MSG_APPL_LOOP:
            publishHeartBeat(false);
            break;
        */
        case MSG_MQTT_SEND_JSONOBJ:
            {
                String strTopic = "msg";
                String strPayload;
                // Extract parameters
                JsonObject *pMsgDoc = (JsonObject *) pMessage;
                if(pMsgDoc) {
                    // If the doc contains "payload" as Json Object and nClass == 1,
                    // send the payload only, with topic, if exist - otherwise topic is "msg"
                    if(nClass == 1 && JsonKeyExists((*pMsgDoc),"payload",JsonObject)) {
                        JsonObject oPayload = GetJsonObject((*pMsgDoc),"payload");
                        serializeJson(oPayload,strPayload);
                        LSC::setJsonValue(*pMsgDoc,"topic",strTopic);
                    } else {
                        serializeJson(*pMsgDoc,strPayload);
                    }
                }
                publishEvent(strTopic.c_str(),strPayload.c_str(),Config.useHA);
            }
            break;
        default:
            break;
    }
    return nResult;
}
/**
 * @brief set Last Will and register needed handlers
 */
void CMQTTController::setup() {
  if (Config.isEnabled) {
    String strTopicString = Config.PublishTopic + Config.useHA ? HA_MSG_AVAILABILITY : "/status";
    String strPayloadString = "offline";
    // Last will has to stay alive during lifteme of this object...
    // create an independend pointer and give it to the function...
    m_pszLastWillTopic = strdup(strTopicString.c_str());
    m_pszLastWillPayload = strdup(strPayloadString.c_str());
    setWill(m_pszLastWillTopic, 2, true, m_pszLastWillPayload);
    setServer(Config.BrokerAddress.c_str(), Config.BrokerPort);
    setCredentials(Config.UserName.c_str(), Config.UserPassword.c_str());

    onDisconnect(std::bind(&CMQTTController::onMqttDisconnect, this, std::placeholders::_1));
    
    onConnect(std::bind(&CMQTTController::onMqttConnect, this, std::placeholders::_1));

    onMessage(std::bind(&CMQTTController::onMqttMessage,this,
                        std::placeholders::_1,
                        std::placeholders::_2,
                        std::placeholders::_3,
                        std::placeholders::_4,
                        std::placeholders::_5,
                        std::placeholders::_6));
    }
}