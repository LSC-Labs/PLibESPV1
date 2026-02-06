#pragma once

#if defined(ESP32) || defined(LIBRETINY)
// #ifdef CONFIG_IDF_TARGET_ESP32
    #include <AsyncTCP.h>
    #include <WiFi.h>
#elif defined(ESP8266)
    #include <ESP8266WiFi.h>
    #include <ESPAsyncTCP.h>
    #include <ESP8266mDNS.h>
#elif defined(TARGET_RP2040) || defined(TARGET_RP2350) || defined(PICO_RP2040) || defined(PICO_RP2350)
    #include <RPAsyncTCP.h>
    #include <WiFi.h>
#endif

#include <ESPAsyncWebServer.h>
