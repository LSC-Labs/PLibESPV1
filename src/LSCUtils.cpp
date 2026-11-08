#ifndef DEBUG_LSC_UTILS
    #undef DEBUGINFOS
#endif
#include <LSCUtils.h>
#include <stdlib.h>

// #include <DevelopmentHelper.h>

namespace LSC {

    /**
     * @brief Checks if a character is one of the supported whitespace chars.
     *
     * This intentionally avoids std::isspace() to prevent locale/encoding
     * surprises on embedded targets.
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

    /**
     * @brief Checks whether a string is a simple decimal number.
     *
     * A leading sign is allowed. At least one digit is required. At most one
     * decimal point is allowed.
     */
    bool isNumber(const char *pszString) {
        bool bResult = false;
        bool bHasDigit = false;
        unsigned int nDotCounter = 0;
        if (pszString && *pszString) {
            // Allow optional '+' or '-' at the start (but still false if only char.
            if (*pszString == '+' || *pszString == '-') pszString++;
            while (*pszString) {
                if ((*pszString >= '0' && *pszString <= '9') || *pszString == '.') {
                    if (*pszString == '.') nDotCounter++;
                    else bHasDigit = true;
                    bResult = true;
                }
                else { bResult = false; break; }
                pszString++;
            }

        }
        return (bResult && bHasDigit && nDotCounter < 2); // Must have at least one digit and only 0 or 1 dot inside.
    }
    /**
     * @brief Skips leading whitespace characters.
     * @param psz Pointer to a null-terminated string, or nullptr.
     * @return Pointer to the first non-whitespace character, end of string, or
     *         nullptr when psz is nullptr.
     */
    const char* skipWhite(const char * psz) {
        while (psz && isWhite(*psz)) psz++;
        return(psz);
    }

    /**
     * @brief Case-insensitive string comparison (ASCII letters).
     *
     * Characters are passed to toupper() as unsigned char - a negative char
     * (UTF-8 / umlaut bytes >= 0x80 on targets with signed char) is undefined
     * behavior for toupper(). nullptr is sorted before every string.
     *
     * @return 0 when equal, otherwise the character difference.
     * @see : https://doxygen.reactos.org/d3/d11/stricmp_8c_source.html
     */
    int stricmp(const char *psz1, const char *psz2)
    {
        if (!psz1 || !psz2) return (psz1 == psz2) ? 0 : (psz1 ? 1 : -1);
        const unsigned char *p1 = (const unsigned char *) psz1;
        const unsigned char *p2 = (const unsigned char *) psz2;
        while (toupper(*p1) == toupper(*p2))
        {
            if (*p1 == 0) return 0;
            p1++;
            p2++;
        }
        return toupper(*p1) - toupper(*p2);
    }

    /**
     * @brief Finds the first index of a character in a string.
     * @return Zero-based index, or -1 when not found or psz is nullptr.
     */
    int indexOf(const char *psz, const char cToken) {
        int nResult = -1;
        if(psz) {
            int nCurPos = -1;
            while(*psz) {
                nCurPos++;
                if(*psz == cToken) { nResult = nCurPos; break; }
                psz++;
            }
        }
        return(nResult);
    }

    /**
     * @brief Finds the last index of a character in a string.
     * @return Zero-based index, or -1 when not found or psz is nullptr.
     */
    int lastIndexOf(const char *psz, const char cToken) {
        int nResult = -1;
        if(psz) {
            int nCurPos = -1;
            while(*psz) {
                nCurPos++;
                if(*psz == cToken) nResult = nCurPos;
                psz++;
            }
        }
        return(nResult);
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
        if(!pszBuffer || nBufferLen <= 0) return(pszBuffer);
        pszBuffer[0] = '\0';                   // empty result if the buffer is too small
        // localtime_r: thread safe (localtime() uses one static buffer for all callers)
        struct tm oTimeInfo;
        if(nBufferLen > 20 && localtime_r(&oNativeTime, &oTimeInfo)) {
            memset(pszBuffer,'\0',(size_t) nBufferLen);
            strftime(pszBuffer,(size_t) nBufferLen,"%FT%T",&oTimeInfo);
        }
        return(pszBuffer);
    }

    /**
     * @brief Converts Celsius to Fahrenheit.
     */
    float getFarenheitFromCelsius(float fTemp) {
        return((fTemp * 1.8f) + 32.0f);      // float literals - no (slow, software) double math
    }

    /**
     * @brief Converts Fahrenheit to Celsius.
     */
    float getCelsiusFromFarenheit(float fTemp) {
        return((fTemp - 32.0f) / 1.8f);
    }
    /**
     * @brief Parses delimited numeric tokens into a byte array.
     *
     * Works without heap and without strtok() (not reentrant, and it skips empty
     * fields - "aa::bb" gave 2 instead of 3 bytes and shifted the following ones).
     * An empty field is 0. Parsing stops at the first invalid token
     * (not a number in nBase, or > 255) - the bytes parsed so far are kept.
     *
     * @param pBytes Target byte array (set to 0 first).
     * @param pszInput Input string such as "127.0.0.1" or "AA:BB".
     * @param cSep Token separator.
     * @param nMaxBytes Maximum target byte count.
     * @param nBase Numeric base for strtol().
     * @return Number of bytes written.
     */
    int ICACHE_FLASH_ATTR parseBytesToArray(uint8_t *pBytes, const char * pszInput, char cSep, int nMaxBytes, int nBase)
    {
        if(!pBytes || nMaxBytes <= 0) return(0);
        memset(pBytes,'\0',(size_t) nMaxBytes);
        if(!pszInput) return(0);

        int nIdx = 0;
        const char *psz = pszInput;
        while(*psz && nIdx < nMaxBytes) {
            // Token: up to the separator / end of string
            const char *pszEnd = psz;
            while(*pszEnd && *pszEnd != cSep) pszEnd++;
            long lValue = 0;
            if(pszEnd > psz) {
                char szToken[16];
                size_t nLen = (size_t) (pszEnd - psz);
                if(nLen >= sizeof(szToken)) break;          // too long for a byte value
                memcpy(szToken, psz, nLen);
                szToken[nLen] = '\0';
                char *pszParsedEnd = nullptr;
                lValue = strtol(szToken, &pszParsedEnd, nBase);
                if(*pszParsedEnd != '\0' || lValue < 0 || lValue > 255) break;   // invalid token
            }
            pBytes[nIdx++] = (uint8_t) lValue;
            if(*pszEnd == '\0') break;
            psz = pszEnd + 1;
            // trailing separator ("1.2.") - nothing follows
        }
        return(nIdx);
    }

    /**
     * @brief Checks whether a string explicitly represents false.
     * @param pszData Data to check. nullptr and "" are treated as false.
     * @return true for nullptr, "", "0", "false", "-", "off", "n" or "no".
     */
    bool ICACHE_FLASH_ATTR isFalseValue(const char* pszData) {
        bool bResult = false;
        if(!pszData || !*pszData) bResult = true;   // no value is false (not "true" in non-explicit mode)
        else if (0 == stricmp(pszData,"0"))     bResult = true;
        else if( 0 == stricmp(pszData,"false")) bResult = true;
        else if( 0 == strcmp( pszData,"-"))     bResult = true;
        else if( 0 == stricmp(pszData,"off"))   bResult = true;
        else if( 0 == stricmp(pszData,"n"))     bResult = true;
        else if( 0 == stricmp(pszData,"no"))    bResult = true;
        return(bResult);
    }
    /** 
     * @brief Checks whether a string represents true.
     *
     * Explicit mode accepts only "1", "true", "+", "on", "y" and "yes".
     * Non-explicit mode returns true for every value that is not an explicit
     * false value.
     *
     * @param pszData Value to check.
     * @param bExplicit true for strict true matching, false for !isFalseValue().
     * @return true when the value should be interpreted as true.
     */
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

}
