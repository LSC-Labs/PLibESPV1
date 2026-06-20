#ifndef DEBUG_LSC_UTILS
    #undef DEBUGINFOS
#endif
#include <LSCUtils.h>
#include <stdlib.h>

// #include <DevelopmentHelper.h>

namespace LSC {

    /**
     * @brief check if the chararcter is a "white" character
     * Same as std::isspace() but does not throw an exception on "Umlaute" like "ä"
     */
    bool isWhite(const char c) {
        return(
            c == ' '  ||  // space
            c == '\t' ||  // horizontal tab
            c == '\n' ||  // newline
            c == '\v' ||  // vertical tab
            c == '\f' ||  // form feed
            c == '\r'   // carriage return
            );
    }

    bool isNumber(const char *pszString) {
        bool bResult = false;
        unsigned int nDotCounter = 0;
        if (pszString && *pszString) {
            // Allow optional '+' or '-' at the start (but still false if only char.
            if (*pszString == '+' || *pszString == '-') pszString++;
            while (*pszString) {
                if ((*pszString >= '0' && *pszString <= '9') || *pszString == '.') {
                    if (*pszString == '.') nDotCounter++;
                    bResult = true;
                }
                else { bResult = false; break; }
                pszString++;
            }

        }
        return (bResult && nDotCounter < 2); // Must have at least one digit and only 0 or 1 dot inside.
    }
    /**
     * @brief skipWhite skips the white characters.
     *As ther could be also special chars like "Umlaute" in german,
    * isspace() cannot handle this chars, so check step by step...
    * </summary>
    * @param psz pointer to the data to be skipped...
    * @returns the new pointer to the first non white character (or end of string)
    * */
    const char* skipWhite(const char * psz) {
        while (psz && isWhite(*psz)) psz++;
        return(psz);
    }

    /**
     * @brief a case insensitive string compare.
     * As it is not part of the std c library - this implementation can help.
     * Ensure, the pointers are valid
     * @see : https://doxygen.reactos.org/d3/d11/stricmp_8c_source.html
     */
    int stricmp(const char *psz1, const char *psz2)
    {
        while (toupper(*psz1) == toupper(*psz2))
        {
            if (*psz1 == 0)
            return 0;
            psz1++;
            psz2++;
        }
        return toupper(*(unsigned const char *)psz1) - toupper(*(unsigned const char *)(psz2));
    }

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
/*
    String getAddressAsString(IPAddress ip) {
        return String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3]);
    }
*/
    /*
        
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
*/
    /**
     * @brief parse a string with delimited values into a byte array.
     * @param pBytes    Pointer to the byte Array to be filled in min length of nMaxBytes
     * @param pszInput  The string to be parsed like "xx:xx:xx:xx"
     * @param cSep      The separator like ':'
     * @param nMaxBytes Max bytes available in pBytes Array
     * @param nBase     Base to convert, like 10 - decimal
     * @return 
     *  */
    int ICACHE_FLASH_ATTR parseBytesToArray(uint8_t *pBytes, const char * pszInput, char cSep, int nMaxBytes, int nBase)
    {
        // set target to zero...
        memset(pBytes,'\0',nMaxBytes);

        // copy source into buffer
        size_t nSizeOfBuffer = strlen(pszInput) + 5;
        char *pszBuffer = (char *) malloc(nSizeOfBuffer);
        memset(pszBuffer,'\0',nSizeOfBuffer);
        strcpy(pszBuffer,pszInput);

        // prepare the token
        char szToken[2];
        szToken[0] = cSep;
        szToken[1] = '\0';

        // start tokenization
        char * psz = strtok(pszBuffer,szToken);
        int nIdx = 0;
        while(psz != NULL && nMaxBytes > nIdx) {
            pBytes[nIdx++] = strtol(psz,NULL,nBase);
            psz = strtok(NULL, szToken);
        }
        free(pszBuffer);
        return(nIdx);
    }

    /**
     * @brief check if it is a "false" value string
     * - "0", "false", "-", "off", "n", "no"
     * @param pszData data to be checked
     * @returns true, if one of the valid false values are matiching the data (case insensitive) 
     * */
    bool ICACHE_FLASH_ATTR isFalseValue(const char* pszData) {
        bool bResult = false;
        if(!pszData) bResult = true;
        else if (0 == stricmp(pszData,"0"))     bResult = true;
        else if( 0 == stricmp(pszData,"false")) bResult = true;
        else if( 0 == strcmp( pszData,"-"))     bResult = true;
        else if( 0 == stricmp(pszData,"off"))   bResult = true;
        else if( 0 == stricmp(pszData,"n"))     bResult = true;
        else if( 0 == stricmp(pszData,"no"))    bResult = true;
        return(bResult);
    }
    /** 
     * @brief Check if the value contains a true expression
     *  - "1", "true, "+", "on", "y" or "yes"
     * In default the data will be checked explicit. If explicit is false,
     * The data will be checked against isFalseValue().
     * 
     * --> Explicit mode    : Values must match
     * --> Not Explict mode : Returns true, if it is NOT false ==> "otto" is not false ==> result is true,
     *
     * @param strData "1", "true", "+", "on", "y" or "yes" will result in a true value (if bExplicit == true)
     * @param bExplicit false == will use !isFalseValue(), all values, not false will become true (!)
     * @return true when the check results in a true value. If pszData is a nullptr, the result is false.
     * */
    bool ICACHE_FLASH_ATTR isTrueValue(const char* pszData, bool bExplicit) {
        bool bResult = false;
        if(!bExplicit) bResult = !isFalseValue(pszData);
        else {
            if(pszData) {
                if(0 == strcmp(pszData,"1")) bResult = true;
                else if( 0 == stricmp(pszData,  "true"))    bResult = true;
                else if( 0 == strcmp( pszData,  "+"))       bResult = true;
                else if( 0 == stricmp(pszData,  "on"))      bResult = true;
                else if( 0 == stricmp(pszData,  "y"))       bResult = true;
                else if( 0 == stricmp(pszData,  "yes"))     bResult = true;
            }
        }
        return(bResult);
    }

/** 

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
*/
}