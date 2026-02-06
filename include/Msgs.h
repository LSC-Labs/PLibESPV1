#pragma once
/**
 * Message definition basics
 * 
 * Defines the basic messages, sent or received by the libary functions.
 * Application should send messages beginning from MSG_USER_BASE
 * 
 */

#define MSG_APPL_STARTING           100    // Application starts - prepare, if needed
#define MSG_APPL_INITIALIZED        102    // Application has been initialized... let's start.
#define MSG_APPL_STATUS_CHANGED     103    // Application notification, Status has changed
#define MSG_APPL_LOOP               110    // Application loop message, pMessage and nClass are optional data   
#define MSG_REBOOT_REQUEST          198    // Request a reboot, nClass = delay in ms
#define MSG_RESTART_REQUEST         198    // Reqeust a restart/reboot - same as MSG_REBOOT_REQUEST
#define MSG_APPL_SHUTDOWN           199    // Application is going down, modules should close connections and free resources

#define MSG_LOG_ENTRY               201    // Log Entry request message see CLog
#define MSG_LOG_ENTRY_JSON          202    // Log Entry request, msg is a json object 


#define MSG_BUTTON_CHANGED          1020
#define MSG_BUTTON_ON               1021
#define MSG_BUTTON_OFF              1022

#define MSG_RF433_SCAN              1080    // Scan for RF433 message sent

#define MSG_WIFI_STARTING           5000
#define MSG_WIFI_CONNECTED          5001
#define MSG_WIFI_SCAN               5002
#define MSG_WIFI_SCAN_RESULT        5003
#define MSG_WIFI_DISABLING          5007
#define MSG_WIFI_DISABLED           5008
#define MSG_WIFI_ERROR              5009
#define MSG_CAPTIVE_PORTAL_STARTED  5050
#define MSG_CAPTIVE_PORTAL_STOPPED  5051
#define MSG_OTA_START               5090
#define MSG_OTA_END                 5091
#define MSG_OTA_PROGRESS            5092
#define MSG_OTA_ERROR               5093

#define MSG_WEBSOCKET_SEND_JSON     5100
#define MSG_WEBSOCKET_DATA_RECEIVED 5101

#define MSG_MQTT_STARTING           5300
#define MSG_MQTT_CONNECTED          5301
#define MSG_MQTT_DISCONNECTED       5308
#define MSG_MQTT_MSG_RECEIVED       5310
#define MSG_MQTT_SEND_JSONOBJ       5320



/**
 * Generic Payload Message, object is a JsonDocument in Message Payload strcuture
 */
#define MSG_PAYLOAD                 9010   

/**
 * Base Value - User defined messages should start with this value
 */
const int MSG_USER_BASE = 10000;

#define LOG_CLASS_INFO       1   // [I]
#define LOG_CLASS_VERBOSE    2
#define LOG_CLASS_WARN       3
#define LOG_CLASS_ERROR      6
#define LOG_CLASS_TRACE      7
#define LOG_CLASS_DEBUG      8
#define LOG_CLASS_EXCEPTION  9


#define WIFI_ACCESS_POINT_MODE  0
#define WIFI_STATION_MODE       1


