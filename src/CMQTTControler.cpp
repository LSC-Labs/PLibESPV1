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

#include <new>

#ifdef ESP32
    #define MQTT_QUEUE_LOCK() std::lock_guard<std::mutex> oQueueGuard(m_oQueueLock)
#else
    #define MQTT_QUEUE_LOCK()
#endif

namespace {
    /// @brief Replace an owned C string by a copy of strValue.
    void replaceOwnedString(char *&pszTarget, const String &strValue) {
        if(pszTarget) free(pszTarget);
        pszTarget = strdup(strValue.c_str());
    }
    void freeOwnedString(char *&pszTarget) {
        if(pszTarget) free(pszTarget);
        pszTarget = nullptr;
    }
}

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
    // Detach the registered components, so they do not call this object later
    for(auto pEntry : m_tComponentHandlerByName.Entries) {
        if(pEntry->value) pEntry->value->m_oHARegistrations.detach(this);
    }
    freeOwnedString(m_pszPublishStateTopic);
    freeOwnedString(m_pszPublishAvailabilityTopic);
    freeOwnedString(m_pszHomeAssistantStatusTopic);
    freeOwnedString(m_pszDeviceCommandTopics);
    freeOwnedString(m_pszMessageBuffer);
    freeOwnedString(m_pszHost);
    freeOwnedString(m_pszUser);
    freeOwnedString(m_pszPassword);
    freeOwnedString(m_pszClientId);
    MQTT_QUEUE_LOCK();
    while(!m_tMessageQeue.empty()) {
        delete m_tMessageQeue.front();
        m_tMessageQeue.pop();
    }
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
    // Without setup() no server is set; without WiFi a connect attempt is useless
    if(Config.isEnabled && m_bSetupDone && !connected() && WiFi.isConnected()) {
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
 * @return true for PublishTopicPrefix + "/cmd" and topics below it.
 */
bool CMQTTController::isDeviceCommandTopic(const char *pszTopic) {
    bool bIsCommandTopic = false;
    if(isDeviceTopic(pszTopic)) {
        // "<prefix>/cmd" and "<prefix>/cmd/..." - the subscription "<prefix>/cmd/#" delivers both
        size_t nPublishTopicLength = Config.PublishTopicPrefix.length();
        const char *pszRest = &pszTopic[nPublishTopicLength];
        bIsCommandTopic = LSC::stricmp(pszRest,"/cmd") == 0 || strncasecmp(pszRest,"/cmd/",5) == 0;
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

    int nPort = Config.BrokerPort;
    if(oCfg.storeValueIf(MQTT_CONFIG_BROCKERPORT, &nPort) && nPort > 0 && nPort <= 65535) Config.BrokerPort = nPort;
    oCfg.storeValueIf(MQTT_CONFIG_BROCKERADDRESS,      Config.BrokerAddress);
    oCfg.storeValueIf(MQTT_CONFIG_USERNAME,            Config.UserName);
    oCfg.storeValueIf(MQTT_CONFIG_PUBLISHTOPIC,        Config.PublishTopicPrefix);
    int nInterval = Config.PublishInterval;
    if(oCfg.storeValueIf(MQTT_CONFIG_PUBLISHINTERVAL, &nInterval) && nInterval >= 0) Config.PublishInterval = nInterval;
    oCfg.storeValueIf(MQTT_CONFIG_USEAUTOTOPIC,    &   Config.useAutoTopic);
    oCfg.storeValueIf(MQTT_CONFIG_HA_TOPICS,           Config.HADiscoveryPrefix);

    oCfg.storeValueIfNot(MQTT_CONFIG_USERPASSWORD,Config.UserPassword, MQTT_HIDDEN_PASSWORD);
    // Topic prefix may have changed - rebuild the cached command topic on next use.
    // (Server/credentials of the client are only changed by setup().)
    freeOwnedString(m_pszDeviceCommandTopics);
    DEBUG_FUNC_END();
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
        // MSG_APPL_LOOP: handled by dispatch() (called by ApplModule::receiveEvent)

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
                        if(strcmp(pMsg->Message,Config.HAOnlineMessage.c_str()) == 0) {
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
                if(pMsgObj) publishDeviceState(*pMsgObj);
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
    if(nPublishInterval >= 0) Config.PublishInterval = nPublishInterval;
    if (Config.isEnabled) {
        DEBUG_INFOS("- initializing connection : %s:%d",Config.BrokerAddress.c_str(),Config.BrokerPort);
        // Own copies - AsyncMqttClient keeps only the pointers
        replaceOwnedString(m_pszHost,     Config.BrokerAddress);
        replaceOwnedString(m_pszUser,     Config.UserName);
        replaceOwnedString(m_pszPassword, Config.UserPassword);
        setServer(m_pszHost, (uint16_t) Config.BrokerPort);
        setCredentials(m_pszUser, m_pszPassword);
        if(Config.ClientID.length() > 0) {
            replaceOwnedString(m_pszClientId, Config.ClientID);
            setClientId(m_pszClientId);
        }
    
        // Set the publish state topic and the last will...
        replaceOwnedString(m_pszPublishStateTopic,        Config.PublishTopicPrefix + "/" + MQTT_MSG_TOPIC_STATE);
        replaceOwnedString(m_pszPublishAvailabilityTopic, Config.PublishTopicPrefix + "/" + MQTT_MSG_TOPIC_STATUS);
        freeOwnedString(m_pszDeviceCommandTopics);
        freeOwnedString(m_pszHomeAssistantStatusTopic);
        if(Config.useHA) {
            replaceOwnedString(m_pszHomeAssistantStatusTopic, Config.HADiscoveryPrefix + "/" + MQTT_MSG_TOPIC_STATUS);
        }

        if(!m_bSetupDone) {
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
        setWill(m_pszPublishAvailabilityTopic, 2, true, MQTT_STATE_UNAVAILABLE);
        m_bSetupDone = true;

        // First attempt now (if WiFi is up), further attempts by dispatch() with delay
        m_oTryConnectDelay.setExpired();
        enableConnection();
    }
    DEBUG_FUNC_END();
}

/**
 * @brief Main MQTT loop hook.
 *
 * Keeps the connection alive, publishes heartbeat data and forwards queued
 * inbound MQTT messages to the application bus.
 */
void CMQTTController::dispatch() {
    processConnectionEvents();
    enableConnection();
    publishHeartBeat(false);
    // Take each message out of the queue before it is processed (queue is filled async)
    for(int nCount = 0; nCount < MQTT_MAX_QUEUED_MESSAGES; nCount++) {
        MQTTMessage * pMessage = nullptr;
        {
            MQTT_QUEUE_LOCK();
            if(m_tMessageQeue.empty()) break;
            pMessage = m_tMessageQeue.front();
            m_tMessageQeue.pop();
        }
        Appl.MsgBus.sendEvent(this,MSG_MQTT_MSG_RECEIVED,pMessage,0);
        delete pMessage;
    }
}

/**
 * @brief Processes connect/disconnect notifications of the async callbacks.
 *
 * Runs in the loop: logging, bus events and the Home Assistant discovery are
 * not executed in the async TCP context.
 */
void CMQTTController::processConnectionEvents() {
    if(m_bEvtDisconnected) {
        m_bEvtDisconnected = false;
        AsyncMqttClientDisconnectReason eReason = (AsyncMqttClientDisconnectReason) m_nDisconnectReason;
        Status.DisConReason       = eReason;
        Status.DisConReasonString = getDisconnectReasonText(eReason);
        ApplLogInfoWithParms("MQTT disconnected (%d) - %s", (int) eReason, Status.DisConReasonString.c_str());
        Appl.MsgBus.sendEvent(this,MSG_MQTT_DISCONNECTED,this,(int) eReason);
    }
    if(m_bEvtConnected) {
        m_bEvtConnected = false;
        ApplLogInfoWithParms("MQTT session established (%d)",(int) connected());
        Appl.MsgBus.sendEvent(this,MSG_MQTT_CONNECTED,this,0);
        // Publish the discovery on every new connection - HA does not request it
        // when only the device (re)connects (only on its own birth message).
        publishHomeAssistantDiscovery();
    }
}

#pragma endregion

#pragma region Publish functions like a JsonNode to the Message Broker

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
        String strTopicName = Config.PublishTopicPrefix + "/" + pszTopic;
        publish(strTopicName.c_str(), (uint8_t) nQOS, bRetain, pszData);   
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
    if(connected() && (Config.PublishInterval > 0 || bForceSend)) {
        // Immediately after (re)connect (delay expired / not started) - otherwise the user interval
        if(bForceSend || !m_oHeartBeatDelay.isActive() || m_oHeartBeatDelay.isDone()) {
            DEBUG_INFOS("MQTT: sending heartbeat... %d",Config.PublishInterval);
            const char *pszStatus = Appl.getStatusAsText();
            publishDeviceTopic(MQTT_MSG_TOPIC_HEARTBEAT,pszStatus ? pszStatus : "{}",0,false);
            m_oHeartBeatDelay.restartWith((unsigned long) Config.PublishInterval * 1000UL);
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
    if(!pszComponentName || !pComponentHandler) return;
    if(m_tComponentHandlerByName.set(pszComponentName,pComponentHandler)) {
        pComponentHandler->m_oHARegistrations.attach(this);
    }
}

/**
 * @brief Removes a component hook (all names) from the discovery list.
 */
void CMQTTController::unregisterHomeAssistantComponent(IHomeAssistantComponent *pComponentHandler) {
    if(!pComponentHandler) return;
    m_tComponentHandlerByName.removeByValue(pComponentHandler);
    pComponentHandler->m_oHARegistrations.detach(this);
}

/**
 * @brief IRegistry - a registered component is being destroyed.
 */
void CMQTTController::forgetRegisteredObject(const void *pObject) {
    m_tComponentHandlerByName.removeIf([pObject](CNamedValueEntry<IHomeAssistantComponent *> *pEntry) {
        return((const void *) pEntry->value == pObject);
    });
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
        if(!pszDeviceName || !*pszDeviceName) pszDeviceName = "esp-device";
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

        // Insert the component discovery data for each registered component handler.
        // Copy names/handlers first - a handler may (un)register components in the call.
        std::vector<String> tNames;
        std::vector<IHomeAssistantComponent *> tHandlers;
        for(auto pEntry : m_tComponentHandlerByName.Entries) {
            tNames.push_back(String(pEntry->getKey()));
            tHandlers.push_back(pEntry->value);
        }
        for(size_t nIdx = 0; nIdx < tNames.size(); nIdx++) {
            // skip handlers removed meanwhile
            if(tHandlers[nIdx] && m_tComponentHandlerByName.hasValueEntry(tHandlers[nIdx])) {
                tHandlers[nIdx]->insertComponentDiscovery(tNames[nIdx].c_str(), *pComponents, this);
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
    }
    DEBUG_FUNC_END();
}


#pragma endregion

#pragma region MQTT onEvent handlers
/**
 * @brief Callback invoked after the broker connection was established (async context).
 *
 * Subscribes to Home Assistant status and device command topics and publishes
 * availability. Events/logging/discovery follow in the loop (processConnectionEvents()).
 */
void CMQTTController::onMqttConnect(bool sessionPresent)
{
    Status.ConStart  = millis();
    m_oHeartBeatDelay.setExpired(); // send the first heartbeat immediately

    // Listen on Home Assistant status topic to get the session status of Home Assistant
    if(Config.useHA && m_pszHomeAssistantStatusTopic) {
        subscribe(m_pszHomeAssistantStatusTopic, 0);
    }
    // Subscribe to your own command topic to receive commands.
    String strCmdTopic = String(getDeviceCommandBaseTopicPath()) + "/#";
    subscribe(strCmdTopic.c_str(), 2);
    
    // Notify in the state availabilty
    publishDeviceTopic(MQTT_MSG_TOPIC_STATUS,MQTT_STATE_AVAILABLE, 2, true);

    m_bSessionPresent = sessionPresent;
    m_bEvtConnected   = true;
}

/**
 * @brief Text of an AsyncMqttClient disconnect reason.
 */
const char * CMQTTController::getDisconnectReasonText(AsyncMqttClientDisconnectReason eReason) {
	switch (eReason)
	{
        case AsyncMqttClientDisconnectReason::TCP_DISCONNECTED:                     return("TCP Disconnected");
        case AsyncMqttClientDisconnectReason::MQTT_UNACCEPTABLE_PROTOCOL_VERSION:   return("MQTT unacceptable protocol version");
        case AsyncMqttClientDisconnectReason::MQTT_IDENTIFIER_REJECTED:             return("MQTT identifier rejected");
        case AsyncMqttClientDisconnectReason::MQTT_SERVER_UNAVAILABLE:              return("MQTT server unavailable");
        case AsyncMqttClientDisconnectReason::MQTT_MALFORMED_CREDENTIALS:           return("MQTT malformed credentials");
        case AsyncMqttClientDisconnectReason::MQTT_NOT_AUTHORIZED:                  return("MQTT not authorized");
        case AsyncMqttClientDisconnectReason::ESP8266_NOT_ENOUGH_SPACE:             return("ESP not enough space");
        default:                                                                    return("Unknown");
	}
}

/**
 * @brief Callback invoked after the broker connection was lost (async context).
 *
 * Only stores the reason - Status, log and MSG_MQTT_DISCONNECTED follow in the loop.
 */
void CMQTTController::onMqttDisconnect(AsyncMqttClientDisconnectReason oReason)
{
    Status.ConEnd       = millis();
    m_nDisconnectReason = (int) oReason;
    m_bEvtDisconnected  = true;
}

/**
 * @brief Callback for inbound MQTT payload fragments (async context).
 *
 * The callback assembles fragmented payloads in a temporary buffer and queues a
 * complete MQTTMessage for later dispatch in the main loop. Messages larger
 * than MQTT_MAX_RECV_MESSAGE_SIZE or exceeding the queue limit are dropped.
 */
void CMQTTController::onMqttMessage(char *pszTopic, char *pszPayload, AsyncMqttClientMessageProperties properties, size_t nLen, size_t nIndex, size_t nTotal)
{
    (void) properties;
    if(nTotal > MQTT_MAX_RECV_MESSAGE_SIZE) return;

    // First fragment: (re)allocate - drops the rest of an interrupted message
    if(nIndex == 0) {
        freeOwnedString(m_pszMessageBuffer);
        m_nMessageBufferSize = 0;
        m_pszMessageBuffer = (char *) malloc(nTotal + 1);
        if(m_pszMessageBuffer) {
            memset(m_pszMessageBuffer,'\0',nTotal + 1);
            m_nMessageBufferSize = nTotal;
        }
    }
    if(!m_pszMessageBuffer || nTotal != m_nMessageBufferSize) return;

    // Move data to the buffer (blockwise)
    if(pszPayload && nIndex + nLen <= m_nMessageBufferSize) {
        memcpy(&m_pszMessageBuffer[nIndex],pszPayload,nLen);
    }
    // Last Block received ? => store data in queue
    if(nIndex + nLen == nTotal) {
        MQTTMessage *pMessage = new (std::nothrow) MQTTMessage(pszTopic, m_pszMessageBuffer, this);
        freeOwnedString(m_pszMessageBuffer);
        m_nMessageBufferSize = 0;
        if(pMessage) {
            bool bQueued = false;
            {
                MQTT_QUEUE_LOCK();
                if(m_tMessageQeue.size() < MQTT_MAX_QUEUED_MESSAGES) {
                    m_tMessageQeue.push(pMessage);
                    bQueued = true;
                }
            }
            if(!bQueued) delete pMessage;     // queue full - drop
        }
    }
}

#pragma endregion
