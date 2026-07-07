#ifndef DEBUG_LSC_MQTT
    #undef DEBUGINFOS
#endif

#include <Arduino.h>
#include <JsonNode.h>
#include <Appl.h>
#include <Network.h>
#include <MQTTController.h>
#include <LSCUtils.h>
#include <Msgs.h>
#include <SysStatus.h>

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
const char * MQTT_CONFIG_HA_TOPICS      = "hatopics";

const char * MQTT_MSG_TOPIC_STATE       = "state";
const char * MQTT_MSG_TOPIC_STATUS      = "status";
const char * MQTT_MSG_TOPIC_HEARTBEAT   = "info";

// Messages for Home Assistance
// -> https://github.com/home-assistant/core/blob/dev/homeassistant/components/mqtt/abbreviations.py
// -> https://www.home-assistant.io/integrations/mqtt/#supported-abbreviations-in-mqtt-discovery-messages

const char * MQTT_HA_MSG_ONLINE         = "online";
const char * MQTT_HA_MSG_OFFLINE        = "offline";


#define  MQTT_HA_TOPIC_AVAILABILITY  "avty"  


#pragma region Constructor, configuration, status and message receiving

/**
 * @brief Creates an MQTT controller with an expired reconnect delay.
 *
 * Marking the delay as expired allows the first connection attempt as soon as
 * MQTT is enabled and dispatch/enableConnection runs.
 */
CMQTTController::CMQTTController() {
    m_oTryConnectDelay.start(5000,false);
    m_oTryConnectDelay.setExpired();
}

/**
 * @brief Releases dynamically generated topic strings.
 */
CMQTTController::~CMQTTController() {
    if(m_pszPublishStateTopic)        free(m_pszPublishStateTopic);
    if(m_pszPublishAvailabilityTopic) free(m_pszPublishAvailabilityTopic);
    if(m_pszHomeAssistantStatusTopic) free(m_pszHomeAssistantStatusTopic);
    if(m_pszDeviceCommandTopics)      free(m_pszDeviceCommandTopics);
}
/**
 * @brief Ensures an enabled MQTT connection is running.
 *
 * If MQTT is enabled but disconnected and the retry delay has elapsed, this
 * starts a new connection attempt and notifies the application bus.
 *
 * @return true when MQTT is enabled and currently connected.
 */
bool CMQTTController::enableConnection() {
    if(Config.isEnabled && !connected()) {
        if(m_oTryConnectDelay.isDone()) {
            // Connect, restart the timer and send a event to all modules.
            connect();  
            m_oTryConnectDelay.restart();
            Appl.MsgBus.sendEvent(this,MSG_MQTT_STARTING,this,0);
        }
    }
    return(Config.isEnabled && connected());
}

/**
 * @brief Checks whether a topic belongs to this device topic prefix.
 * @param pszTopic MQTT topic to inspect.
 * @return true when the topic starts with PublishTopicPrefix followed by '/'.
 */
bool CMQTTController::isDeviceTopic(const char *pszTopic) {
    bool bIsDeviceTopic = false;
    if(pszTopic ) {
        unsigned int nPublishTopicLength = Config.PublishTopicPrefix.length();
        // Check if the topic is long enough to contain the prefix and a device topic
        if(strlen(pszTopic) > nPublishTopicLength && pszTopic[nPublishTopicLength] == '/') {
            String strTopicPrefix(pszTopic);
            bIsDeviceTopic = strTopicPrefix.substring(0,nPublishTopicLength).equalsIgnoreCase(Config.PublishTopicPrefix);
        }
    }
    return(bIsDeviceTopic);
}

/**
 * @brief Checks whether a topic is below this device's command topic path.
 * @param pszTopic MQTT topic to inspect.
 * @return true for topics under PublishTopicPrefix + "/cmd/".
 */
bool CMQTTController::isDeviceCommandTopic(const char *pszTopic) {
    bool bIsCommandTopic = false;
    if(isDeviceTopic(pszTopic)) {
        size_t nPublishTopicLength = Config.PublishTopicPrefix.length();
        String strTopic = pszTopic;
        bIsCommandTopic = strTopic.substring(nPublishTopicLength,nPublishTopicLength + 5).equalsIgnoreCase("/cmd/");
    }
    return(bIsCommandTopic);
}


/**
 * @brief Gets the cached base topic for device commands.
 * @return PublishTopicPrefix + "/cmd".
 */
const char * CMQTTController::getDeviceCommandBaseTopicPath(){
    if(m_pszDeviceCommandTopics == nullptr) {
        m_pszDeviceCommandTopics = strdup((Config.PublishTopicPrefix + "/cmd").c_str());
    }
    return(m_pszDeviceCommandTopics);
}

#pragma endregion

#pragma region Application interface implementation

/**
 * @brief Reads MQTT configuration from JSON.
 */
void CMQTTController::readConfigFrom(JsonNode &oCfg) {
    DEBUG_FUNC_START();
    DEBUG_JSON_OBJ(oCfg);
    oCfg.storeValueIf(MQTT_CONFIG_ENABLED,         &   Config.isEnabled);
    oCfg.storeValueIf(MQTT_CONFIG_USEHA,           &   Config.useHA);;

    oCfg.storeValueIf(MQTT_CONFIG_BROCKERPORT,     &   Config.BrokerPort);
    oCfg.storeValueIf(MQTT_CONFIG_BROCKERADDRESS,      Config.BrokerAddress);
    oCfg.storeValueIf(MQTT_CONFIG_USERNAME,            Config.UserName);
    oCfg.storeValueIf(MQTT_CONFIG_PUBLISHTOPIC,        Config.PublishTopicPrefix);
    oCfg.storeValueIf(MQTT_CONFIG_PUBLISHINTERVAL, &   Config.PublishInterval);
    oCfg.storeValueIf(MQTT_CONFIG_USEAUTOTOPIC,    &   Config.useAutoTopic);
    oCfg.storeValueIf(MQTT_CONFIG_HA_TOPICS,           Config.HADiscoveryPrefix);

    oCfg.storeValueIfNot(MQTT_CONFIG_USERPASSWORD,Config.UserPassword, MQTT_HIDDEN_PASSWORD);
    // Config.SubscribeTopic = Config.PublishTopicPrefix;
}

/**
 * @brief Writes MQTT configuration to JSON.
 */
void CMQTTController::writeConfigTo(JsonNode &oCfg, bool bHideCritical) {
    DEBUG_FUNC_START();
    oCfg[MQTT_CONFIG_ENABLED]           = Config.isEnabled;
    oCfg[MQTT_CONFIG_BROCKERPORT]       = Config.BrokerPort;
    oCfg[MQTT_CONFIG_BROCKERADDRESS]    = Config.BrokerAddress;
    oCfg[MQTT_CONFIG_USERNAME]          = Config.UserName;
    oCfg[MQTT_CONFIG_PUBLISHTOPIC]      = Config.PublishTopicPrefix;
    oCfg[MQTT_CONFIG_USEAUTOTOPIC]      = Config.useAutoTopic;
    oCfg[MQTT_CONFIG_PUBLISHINTERVAL]   = Config.PublishInterval;
    oCfg[MQTT_CONFIG_USEHA]             = Config.useHA;
    oCfg[MQTT_CONFIG_HA_TOPICS]         = Config.HADiscoveryPrefix;
    oCfg[MQTT_CONFIG_USERPASSWORD]      = (const char *) (bHideCritical ? MQTT_HIDDEN_PASSWORD : Config.UserPassword.c_str());
    DEBUG_JSON_OBJ(oCfg);
    
}

/**
 * @brief Writes MQTT runtime status to JSON.
 */
void CMQTTController::writeStatusTo(JsonNode &oStatus, int nLevel) {
    if(nLevel >= STATUS_LEVEL_INFO) {
        oStatus.setValue("isEnabled",       Config.isEnabled);
        oStatus.setValue("isConnected",     connected());
        oStatus.setValue("started",         Status.ConStart);
        oStatus.setValue("disconTS",        Status.ConEnd);
        oStatus.setValue("disconReasonRC",  (int) Status.DisConReason);
        oStatus.setValue("disconReason",    Status.DisConReasonString);
    }
}

/**
 * @brief Handles application events that publish or react to MQTT data.
 *
 * Loop events trigger heartbeat publishing. Received MQTT messages can trigger
 * Home Assistant rediscovery. JSON/text send events are published to configured
 * device topics.
 */
int CMQTTController::receiveEvent(const void * pSender, int nMsg, const void * pMessage, int nClass) {
    // Call the base class receiver first (calls dispatch on MSG_APPL_LOOP)
    int nResult = ApplModule::receiveEvent(pSender,nMsg,pMessage,nClass);
    switch(nMsg) {
        // Send a heart beat on loop message
        case MSG_APPL_LOOP:
            publishHeartBeat(false);
            break;

        // If a MQTT message is received, check if it is a Home Assistant status message to update the session status of Home Assistant
        case MSG_MQTT_MSG_RECEIVED:
            if(Config.useHA && m_pszHomeAssistantStatusTopic) {
                DEBUG_INFOS("MQTT: HA status Topic Message received : %s", m_pszHomeAssistantStatusTopic);
                MQTTMessage * pMsg = (MQTTMessage *) pMessage;
                if(pMsg && pMsg->Topic && pMsg->Message ) {
                    DEBUG_INFOS("MQTT:  ==>  [%s]", pMsg->Topic);
                    if(LSC::stricmp(pMsg->Topic,m_pszHomeAssistantStatusTopic) == 0) {
                        // If Home Assistant goes online, publish the auto discovery information.
                        // see : https://www.home-assistant.io/integrations/mqtt/#birth-and-last-will-messages
                        //       https://www.home-assistant.io/integrations/mqtt/#discovery-messages-and-availability
                        if(strcmp(pMsg->Message,MQTT_STATE_AVAILABLE) == 0) {
                            publishHomeAssistantDiscovery();
                        } 
                    }
                }
            }
            break;

        case MSG_APPL_STARTED:
            publishHomeAssistantDiscovery();
            break;

            

        case MSG_MQTT_SEND_JSONSTATE: {
                JsonNode * pMsgObj = (JsonNode * ) pMessage;
                publishDeviceState(*pMsgObj);
            }
            break;

        case MSG_MQTT_SEND_TEXTSTATE: {
                const char * pMsgObj = (const char * ) pMessage;
                publishDeviceState(pMsgObj);
            }
            break;
        
        case MSG_MQTT_SEND_JSONNODE:
            {
                String strTopic = "msg";
                String strPayload;
                int nQOS = 2;
                bool bRetain = false;
                // Extract parameters

                JsonNode * pMsgObj = (JsonNode *) pMessage;
                if(pMsgObj) {
                    // If the doc contains "payload" as Json Object and nClass == 1,
                    // send the payload only, with topic, if exist - otherwise topic is "msg"
                    if(nClass == MSG_JSON_PAYLOAD) {
                        JsonNode * pPayload = pMsgObj->getObject("payload");
                        if(pPayload) {
                            strPayload = pPayload->getAsJsonText();
                            pMsgObj->storeValueIf("topic",    strTopic);
                            pMsgObj->storeValueIf("qos",    & nQOS);
                            pMsgObj->storeValueIf("retain", & bRetain);
                        }
                    } else {
                        strPayload = pMsgObj->getAsJsonText();
                    }
                }
                if(strPayload.length() > 2) {
                    publishDeviceTopic(strTopic.c_str(),strPayload.c_str(),nQOS,bRetain);
                }
            }
            break;

        default:
            break;
    }
    return nResult;
}

#pragma endregion

#pragma region Setup and dispatching for Arduino framework...
/**
 * @brief Configures MQTT server, callbacks, last will and starts connecting.
 */
void CMQTTController::setup(int nPublishInterval) {
    DEBUG_FUNC_START_PARMS("%d",nPublishInterval);
    Config.PublishInterval = nPublishInterval;
    if (Config.isEnabled) {
        DEBUG_INFOS("- initializing connection : %s:%d",Config.BrokerAddress.c_str(),Config.BrokerPort);
        setServer(Config.BrokerAddress.c_str(), Config.BrokerPort);
        setCredentials(Config.UserName.c_str(), Config.UserPassword.c_str());
    
        // Set the publish state topic and the last will...
        DEBUG_INFOS("- state topic name : %s",(Config.PublishTopicPrefix + "/" + MQTT_MSG_TOPIC_STATE).c_str());
        m_pszPublishStateTopic          = strdup((Config.PublishTopicPrefix + "/" + MQTT_MSG_TOPIC_STATE).c_str());
        m_pszPublishAvailabilityTopic   = strdup((Config.PublishTopicPrefix + "/" + MQTT_MSG_TOPIC_STATUS).c_str());

        if(Config.useHA) {
            m_pszHomeAssistantStatusTopic = strdup((Config.HADiscoveryPrefix + "/" + MQTT_MSG_TOPIC_STATUS).c_str());
        }

        onDisconnect(std::bind(&CMQTTController::onMqttDisconnect, this, std::placeholders::_1));
    
        onConnect(std::bind(&CMQTTController::onMqttConnect, this, std::placeholders::_1));

        onMessage(std::bind(&CMQTTController::onMqttMessage,this,
                            std::placeholders::_1,
                            std::placeholders::_2,
                            std::placeholders::_3,
                            std::placeholders::_4,
                            std::placeholders::_5,
                            std::placeholders::_6));

        setWill(m_pszPublishAvailabilityTopic, 2, true, MQTT_STATE_UNAVAILABLE);

        connect();
    }
}

/**
 * @brief Main MQTT loop hook.
 *
 * Keeps the connection alive, publishes heartbeat data and forwards queued
 * inbound MQTT messages to the application bus.
 */
void CMQTTController::dispatch() {
    enableConnection();
    publishHeartBeat(false);
    while(!m_tMessageQeue.empty()) {
        MQTTMessage * pMessage = m_tMessageQeue.front();
        Appl.MsgBus.sendEvent(this,MSG_MQTT_MSG_RECEIVED,pMessage,0);
        m_tMessageQeue.pop();
        delete pMessage;
    }
}

#pragma endregion

#pragma region Publish functions like a JsonDocument to the Message Broker

/**
 * @brief Publishes a JsonNode on a device-relative topic.
 */
void CMQTTController::publishDeviceTopic(String strTopic, JsonNode &oData,  int nQOS, bool bRetain)
{
    DEBUG_FUNC_START();
    publishDeviceTopic(strTopic.c_str(),oData,nQOS,bRetain);
    DEBUG_FUNC_END();
}

/**
 * @brief Serializes a JsonNode and publishes it on a device-relative topic.
 */
void CMQTTController::publishDeviceTopic(const char *pszTopic, JsonNode &oData,  int nQOS, bool bRetain)
{
    DEBUG_FUNC_START();
	if (connected())
	{
		String mqttBuffer = oData.getAsJsonText();
        publishDeviceTopic(pszTopic, mqttBuffer.c_str(), nQOS,bRetain);
	}
    DEBUG_FUNC_END();
}

/**
 * @brief Publishes text data on a device-relative topic.
 * @param pszTopic Topic suffix below PublishTopicPrefix.
 * @param pszData Payload to publish.
 * @param nQOS MQTT QoS.
 * @param bRetain Retain flag.
 */
void CMQTTController::publishDeviceTopic(const char *pszTopic, const char * pszData, int nQOS, bool bRetain)
{
    DEBUG_FUNC_START_PARMS("\"%s\",\"%s\"",NULL_POINTER_STRING(pszTopic),NULL_POINTER_STRING(pszData));
	if (connected() && pszData && pszTopic)
	{
        char szTopicName[Config.PublishTopicPrefix.length() + strlen(pszTopic) + 5];
        sprintf(szTopicName,"%s/%s",Config.PublishTopicPrefix.c_str(),pszTopic);
        publish(szTopicName, nQOS, bRetain, pszData);   
	} else {
        if(!connected()) ApplLogWarn("MQTT: Message not sent - not connected.");
    }
    DEBUG_FUNC_END();
}

/**
 * @brief Publishes device state JSON to the state topic.
 */
void CMQTTController::publishDeviceState(JsonNode & oStateData) {
    DEBUG_FUNC_START();
    publishDeviceTopic(MQTT_MSG_TOPIC_STATE,oStateData);
    DEBUG_FUNC_END();
}

/**
 * @brief Publishes device state text to the state topic.
 */
void CMQTTController::publishDeviceState(const char * pszStateData) {
    DEBUG_FUNC_START();
    publishDeviceTopic(MQTT_MSG_TOPIC_STATE,pszStateData);
    DEBUG_FUNC_END();
}


/**
 * @brief Publishes a periodic heartbeat/status message.
 * @param bForceSend true to send regardless of the configured interval.
 */
void CMQTTController::publishHeartBeat(bool bForceSend) {
    if(connected() && Config.PublishInterval > 0) {
        // Immediatly, if it is the first time - otherwise use the user publish intervall...
        unsigned long ulNextPublish = (Config.PublishInterval * 1000) + m_ulLastHeartBeat;
        // Time to start ?
        if(bForceSend || ulNextPublish < millis()) {
            DEBUG_INFOS("MQTT: sending heartbeat... %d",Config.PublishInterval);
            publishDeviceTopic(MQTT_MSG_TOPIC_HEARTBEAT,Appl.getStatusAsText(),0,false);
            m_ulLastHeartBeat = millis();
        }
    }
}

#pragma endregion

#pragma region Home Assistant

// Home Assistant web links further information:
// - https://www.home-assistant.io/integrations/mqtt/#mqtt-discovery
// - https://www.home-assistant.io/integrations/mqtt/#discovery-messages-and-availability

/**
 * @brief Registers a component hook for Home Assistant discovery.
 * @param pszComponentName Component name inside the discovery document.
 * @param pComponentHandler Handler that writes component discovery data.
 */
void CMQTTController::registerHomeAssistantComponent(const char *pszComponentName, IHomeAssistantComponent *pComponentHandler) {
    m_tComponentHandlerByName.set(pszComponentName,pComponentHandler);
}

/**
 * @brief Publishes Home Assistant device discovery information.
 *
 * Discovery data is assembled from device metadata, application bus hooks and
 * registered component handlers.
 *
 * @see https://www.home-assistant.io/integrations/sensor.mqtt/
 */
void CMQTTController::publishHomeAssistantDiscovery() {
    DEBUG_FUNC_START();
    if(connected() && Config.useHA) {
        JsonNode oDiscovery;
        JsonNode *pDevice = oDiscovery.getObject("dev",true);
        const char * pszDeviceName = WiFi.getHostname();
        String strCfgUrl = "http://";
        strCfgUrl += pszDeviceName;

        (*pDevice)["ids"]   = pszDeviceName;
        // (*pDevice)["id"]  ="ea334450945aff";

        (*pDevice)["name"] = pszDeviceName;
        (*pDevice)["mf"]   = "LSC";
        (*pDevice)["mdl"]  = Appl.AppName;
        (*pDevice)["sw"]   = Appl.AppVersion;
        // (*pDevice)["sn"]   =  "ea334450945aff";
        (*pDevice)["configuration_url"] = strCfgUrl.c_str(); 


        JsonNode *pOrigin  = oDiscovery.getObject("o",true);
        (*pOrigin)["sw"]   = Appl.AppVersion;
        (*pOrigin)["name"] = Appl.AppName;
        (*pOrigin)["url"]  = "https://github.com/LSC-Labs";

        JsonNode * pComponents  = oDiscovery.getObject("cmps",true);
        // Ask the components to insert their discovery data into the discovery document
        Appl.MsgBus.sendEvent(this, MSG_HA_FILL_DISCOVERY,      &oDiscovery,  0);
        Appl.MsgBus.sendEvent(this, MSG_HA_FILL_DISCOVERY_CMPS, pComponents, 0);

         // Insert the component discovery data for each registered component handler
        for(const char *pszComponentName : m_tComponentHandlerByName.getKeys()) {
            IHomeAssistantComponent *pComponentHandler = m_tComponentHandlerByName.get(pszComponentName);
            if(pComponentHandler) {
                pComponentHandler->insertComponentDiscovery(pszComponentName, *pComponents, this);
            }
        }

        oDiscovery["qos"]         = 2;

        // oDiscovery["state_topic"]= "/home/office/Sensor01/state",
        oDiscovery["state_topic"]           = this->m_pszPublishStateTopic;
        oDiscovery["availability_topic"]    = this->m_pszPublishAvailabilityTopic;

        DEBUG_INFOS("Sending discovery info to %s",(Config.HADiscoveryPrefix + "/device/" + pszDeviceName + "/config").c_str());
        DEBUG_JSON_OBJ(oDiscovery);
    
        const char * pszMsg = oDiscovery.getAsJsonText();
        // Now publish the discover information on the message broker with the topic: homeassistant/device/{device_name}/config
        publish(
                (Config.HADiscoveryPrefix + "/device/" + pszDeviceName + "/config").c_str(), 
                0,                              // QoS 0
                false,                           // As retain message. 
                pszMsg   // Publish the document as string
            );       
        DEBUG_INFOS("HA Discovery published: %s",pszMsg);
    
        DEBUG_FUNC_END();
    }
}


#pragma endregion

#pragma region MQTT onEvent handlers
/**
 * @brief Callback invoked after the broker connection was established.
 *
 * Subscribes to Home Assistant status and device command topics, publishes
 * availability, and notifies the application bus.
 */
void CMQTTController::onMqttConnect(bool sessionPresent)
{
    DEBUG_FUNC_START_PARMS("%d",sessionPresent);
    size_t nBufferSize = Config.HADiscoveryPrefix.length() > Config.PublishTopicPrefix.length() ?
                         Config.HADiscoveryPrefix.length() :
                         Config.PublishTopicPrefix.length();
    char szBuffer[nBufferSize + 20]; // Buffer to create the subscribe topic strings

    Status.ConStart = millis();
    
    // Listen on Home Assistant status topic to get the session status of Home Assistant
    if(Config.useHA && Config.HADiscoveryPrefix.length() > 0) {
        sprintf(szBuffer,"%s/%s",Config.HADiscoveryPrefix.c_str(),"status");
        DEBUG_INFOS("MQTT: subscribing to HA status message : %s",szBuffer);
        subscribe(szBuffer, 0);
    }
    
    // Subscribe to your own command topic to receive commands.
    sprintf(szBuffer,"%s/#",getDeviceCommandBaseTopicPath());
    subscribe(szBuffer, 2);
    
    // Notify in the state availabilty
    publishDeviceTopic(MQTT_MSG_TOPIC_STATUS,MQTT_STATE_AVAILABLE, 2, true);

    ApplLogInfoWithParms("MQTT session established (%d/%d)",sessionPresent,connected());
    Appl.MsgBus.sendEvent(this,MSG_MQTT_CONNECTED,this,0);
    DEBUG_FUNC_END();
}

/**
 * @brief Callback invoked after the broker connection was lost.
 *
 * Stores the disconnect reason in Status and emits MSG_MQTT_DISCONNECTED.
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
 * @brief Callback for inbound MQTT payload fragments.
 *
 * The callback assembles fragmented payloads in a temporary buffer and queues a
 * complete MQTTMessage for later dispatch in the main loop.
 */
void CMQTTController::onMqttMessage(char *pszTopic, char *pszPayload, AsyncMqttClientMessageProperties properties, size_t nLen, size_t nIndex, size_t nTotal)
{
    DEBUG_FUNC_START_PARMS("%s,..,..,%d,%d,%d",
                            NULL_POINTER_STRING(pszTopic),
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
        DEBUG_INFOS("MQTT status message received \"%s\"",m_pszMessageBuffer);
        MQTTMessage *pMessage = new MQTTMessage( pszTopic, m_pszMessageBuffer, this);
        m_tMessageQeue.push(pMessage);
        free(m_pszMessageBuffer);
        m_pszMessageBuffer = nullptr;
        m_nMessageBufferSize = 0;
    }
    DEBUG_FUNC_END();
}

#pragma endregion
