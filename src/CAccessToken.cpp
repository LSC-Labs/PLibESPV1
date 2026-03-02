#include <AccessToken.h>
#include <DevelopmentHelper.h>
#include <SimpleJsonNode.h>
#include <Base64Data.h>
#ifndef DEBUG_LSC_SECURITY
    #undef DEBUGINFOS
#endif
    
CAccessToken::CAccessToken() {}

/**
 * @brief constructor to create an access token structure
 * - Load the IV
 * - copy the token key,
 * - copy the ip Address
 * - create a timestamp in milliseconds
 * @param pszIPAddress the ip address of the client
 * @param pszTokenKey the token key of this application
 * @param pszIV a initial vector for de/encryption (optional)
 */
CAccessToken::CAccessToken(const char * pszIPAddress, const char * pszTokenKey, const char * pszIV) {
    if(pszIV) m_oAESCryptor.IV.loadFromString(pszIV);
    m_ulTimeStamp = millis();
    strncpy(m_szIPAddress,pszIPAddress,sizeof(m_szIPAddress));
    strncpy(m_szTokenKey,pszTokenKey,sizeof(m_szTokenKey));
    
    m_szIPAddress[sizeof(m_szIPAddress) -1] = '\0';
    m_szTokenKey[sizeof(m_szTokenKey) -1] = '\0';
    m_bStructureIsValid = true;
}

/**
 * @brief constructor with a base64 encoded string, containing the token structure
 * to be used to validate the token...
 */
CAccessToken::CAccessToken(const char * pszBase64Data) {
    m_bStructureIsValid = false;
    if(pszBase64Data) {
        // Decode the Base 64 string to IV/Data elements
        char szBuffer[strlen(pszBase64Data) * 2];
        CBase64Data::base64DecodeData(pszBase64Data,strlen(pszBase64Data),szBuffer,sizeof(szBuffer));
        // base64_decode_chars(pszBase64Data,strlen(pszBase64Data),szBuffer);
        CSimpleJsonNode oToken;
        oToken.parse(szBuffer);
        if(oToken.exists("IV") && oToken.exists("Data")) { 
            m_oAESCryptor.IV.loadFromBase64(oToken.getValue("IV"));
            loadBase64DataElement(oToken.getValue("Data"));
        } else {
            DEBUG_INFO("## Error missing json elements..");
        }
    }
    DEBUG_FUNC_END();
}

/**
 * @brief read the data element (content)
 * Data will be decoded from base64 and decrypted.
 * The elements will be filled. If there is a structure error, or some elements are missing, the result is false
 * @param pszBase64Data The base64 encoded data with the encrypted content
 * @returns true, if data is valid, the format is correct and all elements needed are inside.
 */
bool CAccessToken::loadBase64DataElement(const char * pszBase64Data) {
    bool bSuccess = false;
    if(pszBase64Data) {
        // Decode the base64 string... and decrypt the block
        char szBuffer[sizeof(m_szDataElement)] = {0};
        size_t nDataLen = CBase64Data::base64DecodeData(pszBase64Data,strlen(pszBase64Data),szBuffer,sizeof(szBuffer));
        m_oAESCryptor.decrypt(szBuffer,nDataLen,APPL_SECURITY_TOKEN_PASS);
        // Store the clear text strings into my object members...
        strncpy(m_szDataElementAsBase64,pszBase64Data,sizeof(m_szDataElementAsBase64));
        strncpy(m_szDataElement,szBuffer,sizeof(m_szDataElement));
        DEBUG_INFOS("--> parsing %s",m_szDataElement);
        CSimpleJsonNode oToken;
        oToken.parse(m_szDataElement);
        if( oToken.exists("TS") && oToken.exists("IP") && oToken.exists("T") ) {
            m_ulTimeStamp = strtoul(oToken.getValue("TS"),0,10);
            strncpy(m_szIPAddress,oToken.getValue("IP"),sizeof(m_szIPAddress));
            strncpy(m_szTokenKey,oToken.getValue("T"),sizeof(m_szTokenKey));
            m_bStructureIsValid = true;
            bSuccess = true;
        } else {
            DEBUG_INFO("## Error reading json, missing elements..");
        }
    }
    return(bSuccess);       
}

/**
 * @brief get the token data encrypted and in base64 format to be used as data element
 *        
 * @param bRefreshTimeStamp if true, a new timestamp will be generated for the token.
 */
const char * CAccessToken::getBase64DataElement(bool bRefreshTimeStamp) {
    // Build the json structure by string...   
    if(bRefreshTimeStamp) m_ulTimeStamp = millis();
    sprintf(m_szDataElement,
                "{\"TS\":%lu,"
                "\"IP\":\"%s\","
                "\"T\":\"%s\"}",
                m_ulTimeStamp,
                m_szIPAddress,
                m_szTokenKey);
    // make a copy of the data, to encrypt this block...
    // buffer is at least 3 Blocks larger...
    char szDataBlockBuffer[strlen(m_szDataElement) + (16 * 3)] = {0};
    strncpy(szDataBlockBuffer,m_szDataElement,sizeof(szDataBlockBuffer));
    int nFinalDataLen = m_oAESCryptor.encrypt(szDataBlockBuffer,strlen(szDataBlockBuffer),APPL_SECURITY_TOKEN_PASS);
    // Convert the data block to Base64
    CBase64Data::base64EncodeData(szDataBlockBuffer,nFinalDataLen,m_szDataElementAsBase64,sizeof(m_szDataElementAsBase64),true);
    return(m_szDataElementAsBase64);
}

/**
 * @brief get the access token, ready as base64 string
 * @param bRandomIV if true, a new IV will be created for this token.
 * @returns a string pointer with the base64 data, ready to be delivered
 */
const char * CAccessToken::getTokenAsBase64(bool bRandomIV) {
    DEBUG_FUNC_START_PARMS("%d",bRandomIV);
    if(bRandomIV) m_oAESCryptor.IV.createRandom();
    const char * szData = getBase64DataElement();
    const char * szIV = m_oAESCryptor.IV.getAsBase64();
    char szTokenBuffer[strlen(szIV) + strlen(szData) + 40];
    DEBUG_INFOS("-- szTokenBuffer  size == %d",sizeof(szTokenBuffer));
    DEBUG_INFOS("-- IV             size == %d",strlen(szIV));
    DEBUG_INFOS("-- Data           size == %d",strlen(szData));
    sprintf(szTokenBuffer,
            "{\"IV\":\"%s\",\"Data\":\"%s\"}",
            szIV,
            szData);
    DEBUG_INFOS("-- szTokenBuffer final == %d",strlen(szTokenBuffer));
    DEBUG_INFOS("-- szToken       size  == %d",sizeof(m_szTokenAsBase64));
    CBase64Data::base64EncodeData(szTokenBuffer,strlen(szTokenBuffer),m_szTokenAsBase64,sizeof(m_szTokenAsBase64),true);
    DEBUG_INFOS("-- szToken.      final == %d",strlen(m_szTokenAsBase64));
    DEBUG_FUNC_END_PARMS("%s",m_szTokenAsBase64);
    return(m_szTokenAsBase64);
}

/*
void CAccessToken::dump() {
    Serial.println("");
    Serial.println("Access Token:");
    Serial.println("=============");
    Serial.printf("Timestamp             >>%lu<<\n",m_ulTimeStamp);
    Serial.printf("IP Address     (%d/%d)>>%s<<\n",sizeof(m_szIPAddress),strlen(m_szIPAddress), m_szIPAddress);
    Serial.printf("Token key      (%d/%d)>>%s<<\n",sizeof(m_szTokenKey), strlen(m_szTokenKey),m_szTokenKey);
    Serial.printf("Data element   (%d/%d)>>%s<<\n",sizeof(m_szDataElement), strlen(m_szDataElement),m_szDataElement);
    Serial.printf("Data element64 (%d/%d)>>%s<<\n",sizeof(m_szDataElementAsBase64), strlen(m_szDataElementAsBase64),m_szDataElementAsBase64);
    Serial.printf("IV                    >>%s<<\n",m_oAESCryptor.IV.getAsString());
    Serial.printf("IV (Base64)           >>%s<<\n",m_oAESCryptor.IV.getAsBase64());
    Serial.printf("Token (Base64) (%d/%d)>>%s<<\n",sizeof(m_szTokenAsBase64),strlen(m_szTokenAsBase64),m_szTokenAsBase64);
    Serial.println("---------------------------------------------------------------");
}
*/

/**
 * @brief check if the current token is valid
 * - IP Address matches
 * - Appl Token matches
 * - Token is still valid and Timestamp is not expired
 * @param pszIPAddress Client IP Address to be validated with the token
 * @param pszTokenKey the application token key, has to match
 * @return true, if the structure of the token and the values are correct.
 */
bool CAccessToken::isAuthValid(const char * pszIPAddress, const char * pszTokenKey) {
    DEBUG_FUNC_START_PARMS("\"%s\",\"%s\"",NULL_POINTER_STRING(pszIPAddress),NULL_POINTER_STRING(pszTokenKey));
    bool bIsValid = false;
    DEBUG_INFOS("Structure is valid : %d\n",m_bStructureIsValid);
    if(m_bStructureIsValid && pszIPAddress && pszTokenKey ) {
        DEBUG_INFOS("TS %lu\t\t >  %lu",m_ulTimeStamp + TOKEN_TIME_ALIVE, millis());
        DEBUG_INFOS("IP %s\t == %s",pszIPAddress, m_szIPAddress);
        DEBUG_INFOS("K  %s\t == %s",pszTokenKey, m_szTokenKey);
        if( 
            ((m_ulTimeStamp + TOKEN_TIME_ALIVE) > millis() ) &&
            (strcmp(pszIPAddress,m_szIPAddress) == 0)        &&
            (strcmp(pszTokenKey, m_szTokenKey ) == 0)
        ) {
            bIsValid = true;
        } 
    }
    DEBUG_FUNC_END_PARMS("%d",bIsValid);
    return(bIsValid);
}
