#pragma once

#define MSG_LOG_ENTRY          1001
#define MSG_LOG_ENTRY_JSON     1002

#define MSG_BUTTON_CHANGED     1020
#define MSG_BUTTON_ON          1021
#define MSG_BUTTON_OFF         1022

#define MSG_SCAN_RF433         2020

#define MSG_WIFI_STARTING      5000
#define MSG_WIFI_CONNECTED     5001
#define MSG_WIFI_SCAN          5002
#define MSG_WIFI_SCAN_RESULT   5003
#define MSG_WIFI_DISABLING     5007
#define MSG_WIFI_DISABLED      5008
#define MSG_WIFI_ERROR         5009

#define MSG_SOCKET_SEND_JSON        5100

#define MSG_MQTT_MSG_RECEIVED       7001

#define MSG_REBOOT_REQUEST          8000    // Request a reboot, nClass = delay in ms
#define MSG_APPL_INITIALIZED        9000    // Application has been initialized... let's start.
#define MSG_APPL_SHUTDOWN           9001    // Application is going down, modules should close connections and free resources
#define MSG_APPL_STATUS_CHANGED     9002    // Application notification, Status has changed

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


