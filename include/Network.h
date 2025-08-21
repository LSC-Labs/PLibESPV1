#pragma once

#ifdef CONFIG_IDF_TARGET_ESP32
    #include <WiFi.h>
#else
    #include <ESP8266WiFi.h>
    #include <ESP8266mDNS.h>
#endif

#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
