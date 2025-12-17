#pragma once
#include <Arduino.h>
#include <IPAddress.h>

namespace LSC {

    char * getCurrentISODateTime(char *pszBuffer, int nBufferLen);
    char * getISODateTime(time_t oNativeTime, char *pszBuffer, int nBufferLen);
    
    float getFarenheitFromCelsius(float fTemp);
    float getCelsiusFromFarenheit(float fTemp);

    void ICACHE_FLASH_ATTR copyTo(char *pszTarget, const char *pszSource, int nMaxLen);
    bool ICACHE_FLASH_ATTR isTrueValue(const char * pszData, bool bExplicit = false);
    bool ICACHE_FLASH_ATTR isFalseValue(const char * pszData);

    // String getAddressAsString(IPAddress ip);
    // String getAddressAsString(ip4_addr ip);

    int ICACHE_FLASH_ATTR parseBytesToArray(byte *pBytes, const char * pszData, char cSep, int nMaxBytes, int nBase);

    bool ICACHE_FLASH_ATTR setValue(float         *pTarget, String strValue, const float         *pDefault   = nullptr);
    bool ICACHE_FLASH_ATTR setValue(int           *pTarget, String strValue, const int           *pDefault   = nullptr);
    bool ICACHE_FLASH_ATTR setValue(unsigned long *pTarget, String strValue, const unsigned long *pDefault   = nullptr);
    bool ICACHE_FLASH_ATTR setValue(bool          *pTarget, String strValue, const bool          *pDefault   = nullptr);

    bool ICACHE_FLASH_ATTR setValue(String & strTarget, const char* pszValue, const char          *pszDefault = nullptr);

    /*
    bool ICACHE_FLASH_ATTR storeValueIF(char  pTarget[], size_t nTargetSize, const char* pszValue,const char* pszDefault = nullptr);
    bool ICACHE_FLASH_ATTR storeValue(  char  pTarget[], size_t nTargetSize, const char* pszValue,const char* pszDefault = nullptr);
    */

    bool ICACHE_FLASH_ATTR setValue(IPAddress &oAddress, const char *pszAddress, const char *pszDefault = nullptr);

}