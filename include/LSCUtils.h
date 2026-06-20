#pragma once
#include "Runtime.h"

namespace LSC {

    char * getCurrentISODateTime(char *pszBuffer, int nBufferLen);
    char * getISODateTime(time_t oNativeTime, char *pszBuffer, int nBufferLen);
    
    float getFarenheitFromCelsius(float fTemp);
    float getCelsiusFromFarenheit(float fTemp);

    int stricmp(const char *psz1, const char *psz2);

    bool ICACHE_FLASH_ATTR isTrueValue(const char * pszData, bool bExplicit = true);
    bool ICACHE_FLASH_ATTR isFalseValue(const char * pszData);
    bool ICACHE_FLASH_ATTR isWhite(const char c);
    bool ICACHE_FLASH_ATTR isNumber(const char *psz);
    const char * ICACHE_FLASH_ATTR skipWhite(const char * psz);

    int ICACHE_FLASH_ATTR parseBytesToArray(uint8_t *pBytes, const char * pszData, char cSep, int nMaxBytes, int nBase);

}