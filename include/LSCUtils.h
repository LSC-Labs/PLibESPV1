#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <IPAddress.h>

namespace LSC {

    char * getCurrentISODateTime(char *pszBuffer, int nBufferLen);
    char * getISODateTime(time_t oNativeTime, char *pszBuffer, int nBufferLen);
    
    float getFarenheitFromCelsius(float fTemp);
    float getCelsiusFromFarenheit(float fTemp);

    int stricmp(const char *psz1, const char *psz2);

    void ICACHE_FLASH_ATTR copyTo(char *pszTarget, const char *pszSource, int nMaxLen);
    bool ICACHE_FLASH_ATTR isTrueValue(const char * pszData, bool bExplicit = false);
    bool ICACHE_FLASH_ATTR isFalseValue(const char * pszData);

    // String getAddressAsString(IPAddress ip);
    // String getAddressAsString(ip4_addr ip);

    int ICACHE_FLASH_ATTR parseBytesToArray(uint8_t *pBytes, const char * pszData, char cSep, int nMaxBytes, int nBase);

}