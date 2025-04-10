#ifndef DEBUG_LSC_MQTT
    #undef DEBUGINFOS
#endif

#include <Arduino.h>
#include <Appl.h>
#include <Network.h>
#include <MQTTController.h>
#include <LSCUtils.h>

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
                if(bTryConnect) connect();
            }
        }
    }
    return(Config.isEnabled && connected());
}

void CMQTTController::readConfigFrom(JsonObject &oCfg) {
    DEBUG_FUNC_START();
    DEBUG_JSON_OBJ(oCfg);
    Config.isEnabled    = oCfg[MQTT_CONFIG_ENABLED];
    Config.useHA        = oCfg[MQTT_CONFIG_USEHA];
    Config.useAutoTopic = oCfg[MQTT_CONFIG_USEAUTOTOPIC];

    LSC::setValue(&(Config.BrokerPort),      oCfg[MQTT_CONFIG_BROCKERPORT]);
    LSC::setValue(Config.BrokerAddress,      oCfg[MQTT_CONFIG_BROCKERADDRESS]);
    LSC::setValue(Config.UserName,           oCfg[MQTT_CONFIG_USERNAME]);
    LSC::setValue(Config.PublishTopic,       oCfg[MQTT_CONFIG_PUBLISHTOPIC]);
    LSC::setValue(&(Config.PublishInterval), oCfg[MQTT_CONFIG_PUBLISHINTERVAL]);
    LSC::setValue(&(Config.useAutoTopic),    oCfg[MQTT_CONFIG_USEAUTOTOPIC]);
    String strPasswd = oCfg[MQTT_CONFIG_USERPASSWORD];
    if(strPasswd && strPasswd.length() > 0 && strPasswd != MQTT_HIDDEN_PASSWORD) {
        Config.UserPassword = strPasswd;
    }
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

#pragma region Publish an Event to the Message Broker
void CMQTTController::publishEvent(String strTopic, JsonDocument &oData, bool bToHA)
{
    // JsonObject oNode = oData.as<JsonObject>();
    JsonObject oNode = GetJsonDocumentAsObject(oData);
    publishEvent(strTopic, oNode,bToHA);
}

void CMQTTController::publishEvent(String strTopic, JsonObject &oData,  bool bToHA)
{
	if (isSessionActiv())
	{
		String mqttBuffer;
		serializeJson(oData, mqttBuffer);
        publishEvent(strTopic.c_str(), mqttBuffer.c_str(), bToHA);
	}
}

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

#pragma endregion


void CMQTTController::publishBoot()
{
    if(isSessionActiv()) {
        publishEvent("app",Appl.AppName.c_str());
        publishEvent("version",Appl.AppVersion.c_str());
        publishEvent("ip",WiFi.localIP().toString().c_str());
        publishEvent("status","booting");
        #if ARDUINOJSON_VERSION_MAJOR < 7
            StaticJsonDocument<512> oData;
	    #else
		    JsonDocument oData;
	    #endif
        
        oData["type"]       = "boot";
        oData["clientid"]   = this->getClientId();
        oData["prog"]       = Appl.AppName;
        oData["version"]    = Appl.AppVersion;
        oData["time"]       = Appl.StartTime;
        oData["uptime"]     = Appl.getUpTime();
        oData["ip"]         = WiFi.localIP().toString();
        publishEvent(String("info"),oData);
        // m_ulLastHeartBeat = millis();
    }
}

void CMQTTController::publishHeartBeat(bool bForceSend) {
    if(isSessionActiv() && Config.PublishInterval > 0) {
        // Immediatly, if it is the first time - otherwise use the user publish intervall...
        unsigned long ulNextPublish = 0;
        if(m_ulLastHeartBeat > 0) ulNextPublish = (Config.PublishInterval * 1000) + m_ulLastHeartBeat;
        // Time to start ?
        if(bForceSend || ulNextPublish < millis()) {
            ApplLogVerboseWithParms("MQTT sending hartbeat... %d",Config.PublishInterval);
            String strUpTime = Appl.getUpTime();
            publishEvent("uptime",strUpTime.c_str());
            publishEvent("status","online");
            #if ARDUINOJSON_VERSION_MAJOR < 7
                StaticJsonDocument<256> oHeartBeat;
            #else
                JsonDocument oHeartBeat;
            #endif
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
}

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
    ApplLogErrorWithParms("MQTT disconnected (%d) - %s",
                          oReason,
                          Status.DisConReasonString.c_str());
}

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
        Appl.MsgBus.sendEvent(this,MSG_MQTT_MSG_RECEIVED,m_pszMessageBuffer,nTotal);
        free(m_pszMessageBuffer);
        m_pszMessageBuffer = nullptr;
        m_nMessageBufferSize = 0;
    }
}

/// @brief set Last Will and register needed handlers
void CMQTTController::setup() {
  if (Config.isEnabled) {
    String strTopicString = Config.PublishTopic + Config.useHA ? HA_MSG_AVAILABILITY : "/status";
    String strPayloadString = "offline";
    char *pszTopicLWT   = strdup(strTopicString.c_str());
    char *pszPayloadLWT = strdup(strPayloadString.c_str());
    setWill(pszTopicLWT, 2, true, pszPayloadLWT);
    setServer(Config.BrokerAddress.c_str(), Config.BrokerPort);
    setCredentials(Config.UserName.c_str(), Config.UserPassword.c_str());

    onDisconnect(std::bind(&CMQTTController::onMqttDisconnect, this, std::placeholders::_1));
    /*
    mqttClient.onPublish(onMqttPublish);
    mqttClient.onSubscribe(onMqttSubscribe);
    */
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