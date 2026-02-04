#ifndef DEBUG_LSC_UTILS
    #undef DEBUGINFOS
#endif
#include <stdlib.h>
#include <Appl.h>
#include <LSCUtils.h>
#include <JsonHelper.h>
#include <DevelopmentHelper.h>

namespace LSC {

    /**
     * @brief Get the current ISO Time String representation.
     * @param pszBuffer The buffer to store the resulting ISO time string.
     * @param nBufferLen The length of the buffer.
     * @return String The ISO formatted time string.
     */
    char * getCurrentISODateTime(char *pszBuffer, int nBufferLen) {
        time_t oNativeTime;
        time(&oNativeTime);
        return(getISODateTime(oNativeTime,pszBuffer,nBufferLen));
    }

    /**
     * @brief Get the ISO Time String representation of a native time_t value.
     * @param oNativeTime The native time_t value to be converted.
     * @param pszBuffer The buffer to store the resulting ISO time string.
     * @param nBufferLen The length of the buffer.
     * @return String The ISO formatted time string.
     */
    char * getISODateTime(time_t oNativeTime, char *pszBuffer, int nBufferLen) {
        struct tm* oTimeInfo = localtime(&oNativeTime);
        if(nBufferLen > 20) {
            memset(pszBuffer,'\0',sizeof(nBufferLen));
            strftime(pszBuffer,nBufferLen,"%FT%T",oTimeInfo);
        }
        return(pszBuffer);
    }

    float getFarenheitFromCelsius(float fTemp) {
        return((fTemp * 1.8) + 32);
    }
    float getCelsiusFromFarenheit(float fTemp) {
        return((fTemp - 32) * 1.8);
    }

    String getAddressAsString(IPAddress ip) {
        return String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3]);
    }
        /*
        String getAddressAsString(ip4_addr ip) {
            DEBUG_FUNC_START();
            char tBuffer[20];
            sprintf(tBuffer,IPSTR,IP2STR(&(ip)));
            return(String(tBuffer));
        }
        */
    void ICACHE_FLASH_ATTR copyTo(char *pszTarget, const char *pszSource, int nMaxLen) {
        if(pszTarget && pszSource) {
            if(nMaxLen > 0) memset(pszTarget,0,nMaxLen);
            strncpy(pszTarget,pszSource,nMaxLen);
        }
    }

    void ICACHE_FLASH_ATTR copyTo(char &szTarget, const char &szSource, int nMaxLen) {
        if(szTarget && szSource) {
            if(nMaxLen > 0) memset(&szTarget,0,nMaxLen);
            strncpy(&szTarget,&szSource,nMaxLen);
        }
    }

    /// @brief parse a string with delimited values into a byte array.
    /// @param pBytes    Pointer to the byte Array to be filled in min length of nMaxBytes
    /// @param pszInput  The string to be parsed like "xx:xx:xx:xx"
    /// @param cSep      The separator like ':'
    /// @param nMaxBytes Max bytes available in pBytes Array
    /// @param nBase     Base to convert, like 10 - decimal
    /// @return 
    int ICACHE_FLASH_ATTR parseBytesToArray(byte *pBytes, const char * pszInput, char cSep, int nMaxBytes, int nBase)
    {
        // set target to zero...
        memset(pBytes,'\0',nMaxBytes);

        // copy source into buffer
        char tBuffer[strlen(pszInput) +2 ];
        memset(tBuffer,'\0',sizeof(tBuffer));
        strcpy(tBuffer,pszInput);

        // prepare the token
        char tToken[2];
        tToken[0] = cSep;
        tToken[1] = '\0';

        // start tokenization
        char * psz = strtok(tBuffer,tToken);
        int nIdx = 0;
        while(psz != NULL && nMaxBytes > nIdx) {
            pBytes[nIdx++] = strtol(psz,NULL,nBase);
            psz = strtok(NULL, " ");
        }
        return(nIdx);
    }

    /// @brief check if it is a "false" value string
    /// @param pszData either nullptr, "0", "false", "off" or "-" will result in a false result
    /// @return 
    bool ICACHE_FLASH_ATTR isFalseValue(const char* pszData) {
        bool bResult = false;
        if(!pszData) bResult = true;
        else if (0 == strcmp(pszData,"0")) bResult = true;
        else if( 0 == strcmp(pszData,"false")) bResult = true;
        else if( 0 == strcmp(pszData,"-")) bResult = true;
        else if( 0 == strcmp(pszData,"off")) bResult = true;
        return(bResult);
    }

    /// @brief Check if the value is a true value.
    ///        checks if a string is NOT false, by using the isFalseValue() function.
    ///        To check explicit if the values represents a true state, use bExplicit=true
    /// @param strData either "1", "true" or "+" will result in a true value (if bExplicit == true)
    /// @param bExplicit false == will use !isFalseValue(), otherwise a check of specific values
    /// @return 
    bool ICACHE_FLASH_ATTR isTrueValue(const char* pszData, bool bExplicit) {
        bool bResult = false;
        if(!bExplicit) bResult = !isFalseValue(pszData);
        else {
            if(pszData) {
                if(0 == strcmp(pszData,"1")) bResult = true;
                else if( 0 == strcmp(pszData,"true")) bResult = true;
                else if( 0 == strcmp(pszData,"+")) bResult = true;
                else if( 0 == strcmp(pszData,"on")) bResult = true;
            }
        }
        return(bResult);
    }

    /// @brief Store a int value to the target (by pointer)
    /// @param pTarget pointer to store the value
    /// @param strValue the value to be stored. if null or empty, try to set the default
    /// @param pDefault pointer to a default, if strValue can not be used.
    /// @return  true, if value or default could be set
    bool ICACHE_FLASH_ATTR setValue(float* pTarget, String strValue, const float *pDefault) {
        DEBUG_FUNC_START();
        bool bResult = true;
        if(pTarget) {
            if(strValue && strValue.length() > 0) { 
                
                *pTarget = strtod(strValue.c_str(),NULL); // strValue.toInt();
                // DEBUG_INFOS(" storeValue(int) -> %s (%d)", strValue.c_str(),*pTarget);
            } else if(pDefault && pTarget != pDefault) {
                *pTarget = *pDefault;
            } else {
                bResult = false;
            }
        } else bResult = false;
        return(bResult);
    }


    /// @brief Store a int value to the target (by pointer)
    /// @param pTarget pointer to store the value
    /// @param strValue the value to be stored. if null or empty, try to set the default
    /// @param pDefault pointer to a default, if strValue can not be used.
    /// @return  true, if value or default could be set
    bool ICACHE_FLASH_ATTR setValue(int* pTarget, String strValue, const int *pDefault) {
        DEBUG_FUNC_START();
        bool bResult = true;
        if(pTarget) {
            if(strValue && strValue.length() > 0) { 
                
                *pTarget = strValue.toInt();
                // DEBUG_INFOS(" storeValue(int) -> %s (%d)", strValue.c_str(),*pTarget);
            } else if(pDefault && pTarget != pDefault) {
                *pTarget = *pDefault;
            } else {
                bResult = false;
            }
        } else bResult = false;
        return(bResult);
    }

    /// @brief Store a unsigned long value to the target (by pointer)
    /// @param pTarget pointer to store the value
    /// @param strValue the value to be stored. if null or empty, try to set the default
    /// @param pDefault pointer to a default, if strValue can not be used.
    /// @return  true, if value or default could be set
    bool ICACHE_FLASH_ATTR setValue(unsigned long* pTarget, String strValue, const unsigned long *pDefault) {
        DEBUG_FUNC_START();
        bool bResult = true;
        if(pTarget) {
            if(strValue && strValue.length() > 0) { 
                *pTarget = strValue.toInt();
            } else if(pDefault && pTarget != pDefault) {
                *pTarget = *pDefault;
            } else {
                bResult = false;
            }
        } else bResult = false;
        return(bResult);
    }
       


    /// @brief Store a bool value to the target (by pointer)
    /// @param pTarget pointer to store the value
    /// @param strValue the value to be stored. if null or empty, try to set the default
    /// @param bDefault The default, if strValue can not be used. 
    /// @return  true, if value or default could be set
    bool ICACHE_FLASH_ATTR setValue(bool *pTarget, String strValue, const bool *pDefault) {
        bool bResult = true;
        if(pTarget) {
            if(strValue && strValue.length() > 0) { 
                *pTarget = isTrueValue(strValue.c_str(),false);
            } else if(pDefault && pTarget != pDefault) {
                *pTarget = *pDefault;
            } else {
                bResult = false;
            }
        } else bResult = false;
        return(bResult);
    }

    /// @brief Store a psz value to the target 
    /// @param strTarget Target string object
    /// @param pszValue the value to be stored. if null or empty, try to set the default
    /// @param pszDefault The default, if pszValue can not be used.
    /// @return  true, if value or default could be set
    bool ICACHE_FLASH_ATTR setValue(String &strTarget, const char* pszValue, const char* pszDefault) {
        DEBUG_FUNC_START();
        bool bResult = true;
        if(strTarget) {
            if(pszValue && *pszValue && *pszValue != '\0') {
                strTarget = pszValue;
            } else if(pszDefault && *pszDefault != '\0') {
                strTarget = pszValue;
            } else {
                bResult = false;
            }
        } else bResult = false;
        return(bResult);
    }


    bool ICACHE_FLASH_ATTR setValue(IPAddress &oAddress, const char *pszAddress, const char *pszDefault) {
        DEBUG_FUNC_START();
        bool bResult = true;
        if(pszAddress) {
            oAddress.fromString(pszAddress);
        } else if(pszDefault) {
            oAddress.fromString(pszDefault);
        } else {
            bResult = false;
        }
        return(bResult);
    }

}