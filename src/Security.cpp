#ifndef DEBUG_LSC_SECURITY
    #undef DEBUGINFOS
#endif
  

// Using library : https://github.com/me-cooper/esp8266-aes-128-cbc
#include <LSCUtils.h>
#include <Appl.h>
#include <JsonHelper.h>
#include <DevelopmentHelper.h>
#include <Network.h>
#include "libb64/cdecode.h"
#include "libb64/cencode.h"

#define JSON_DOC_TOKEN_SIZE 512
#define TOKEN_TIME_ALIVE    (30*60*1000)       // 30 minutes...

// Ensure the APPL_SECUIRTY_TOKEN_PASS is exact 16 chars long
#ifndef APPL_SECURITY_TOKEN_PASS
    #define APPL_SECURITY_TOKEN_PASS "Kis8%$vvQ@ä+qw12"
#endif

// Unique Key - embeded into the security token
#ifndef APPL_SECURITY_TOKEN_KEY
    #define APPL_SECURITY_TOKEN_KEY "ds$woEir=ncn<jek"
#endif

const String strAppPass = APPL_SECURITY_TOKEN_PASS;  // exact 16 chars !

#pragma region Base64 encoding/decoding

/**
 * @brief Get the base64 decoded data (as string)
 * @param strBase64String 
 * @return 
 *  */
String getBase64DecodedString(String &strBase64String) {
    DEBUG_FUNC_START_PARMS("%s",strBase64String.c_str());
    char tData[strBase64String.length()];
    memset(tData,'\0',sizeof(tData));
    base64_decode_chars(strBase64String.c_str(),strBase64String.length(),tData);
    DEBUG_FUNC_END_PARMS("%s",tData);
    return(String(tData));
}

/**
 * @brief Get the base64 encoded data (as string).
 */
String getBase64EncodedString(String &strString) {
    DEBUG_FUNC_START_PARMS("%s",strString.c_str());
    char tData[strString.length() * 4];
    memset(tData,'\0',sizeof(tData));
    unsigned int nLen = base64_encode_chars(strString.c_str(),strString.length(),tData);
    if(nLen > sizeof(tData)) ApplLogErrorWithParms("Buffer too small (%d) needed == %d",sizeof(tData),nLen);
    // Remove newlines and cr's if exist...
    String strResult;
    strResult.reserve(sizeof(tData));
    for(unsigned int nIdx = 0; nIdx < sizeof(tData); nIdx++) {
        char c = tData[nIdx];
        if(c == '\0') break;
        switch(c) {
            case '\r' :
            case '\n' : break;
            default   : strResult += c;
        }
    }
    DEBUG_FUNC_END_PARMS("%s",strResult.c_str());
    return(strResult);
}

/**
 * Get a base 64 encoded string (based on an array)
 */
String getBase64EncodedString(const char *tArray, int nSizeOfArray) {
    DEBUG_FUNC_START_PARMS("..,%d",nSizeOfArray);
    char tData[nSizeOfArray * 4];
    memset(tData,'\0',sizeof(tData));
    unsigned int nLen = base64_encode_chars(tArray,nSizeOfArray,tData);
    if(nLen > sizeof(tData)) ApplLogErrorWithParms("Buffer too small (%d) needed == %d",sizeof(tData),nLen);
    DEBUG_FUNC_END_PARMS("%s",tData);
    return(String(tData));
}
#pragma endregion



void convertStringToArray(const char * psz, uint8_t * tArray, size_t nArraySize) {
    memset(tArray,'\0',nArraySize);
    for (size_t i = 0; i < nArraySize; ++i) {
        if(psz && * psz) tArray[i] = *psz++;
    }
}
/** Convert a string with hex data into an array */
void convertStringToArray(const String & str, uint8_t *tArray, size_t nArraySize) {
    convertStringToArray(str.c_str(),tArray,nArraySize);
    memset(tArray,'\0',nArraySize);
    for (size_t i = 0; i < nArraySize; ++i) {
        if(str.length() > i) tArray[i] = str[i];
    }
}

#pragma region Random numbers - as good as possible without hw support ...

bool _bRandomNumbersInitialized = false;
/**
 * Initialize the random generator on the first time usage...
 */
const void initializeRandomNumbers(bool bForce) {
    if(bForce || !_bRandomNumbersInitialized) {
        randomSeed(millis());
        _bRandomNumbersInitialized = true;
    }
}

/**
 * get random numbers
 * @param tBuffer Array to store the random numbers in
 * @param nBufferLen Size of the array (tBuffer)
 * @return a string with the native numbers (base64 encoded)
 */
const String getRandomNumbers(byte tBuffer[], int nBufferLen) {
    initializeRandomNumbers(false);
    String strNumbers;
    for(int nIdx = 0; nIdx < nBufferLen; nIdx++) {
        tBuffer[nIdx] = random(1,254);
        strNumbers += tBuffer[nIdx];
        strNumbers += ",";
    }
    char tData[ nBufferLen * 2];
    base64_encode_chars((char*) tBuffer,nBufferLen,tData);
    String strEncoded = (char*) tData;
    return(strEncoded);
}
#pragma endregion
  
#pragma region PKCS7 Padding 
  
/**
 * Insert the PKCS7 Padding 
 * - to fill up data until the block end
 * Ensure, enough memory is allocated (!)
 * -> if the datalength hits exact the boundary of the block, an extra block is needed (!)
 * Best is always to allocate one block extra to ensure (!)
 * @param pData Pointer to the data
 * @param nDataLength Length of the data.
 * @param nBlockSize Size of a block (default = 16)
 */
void insertPkcs7Padding(byte* pData, size_t nDataLength, size_t nBlockSize = 16) {
    size_t nPaddingLen = nBlockSize - (nDataLength % nBlockSize);
    for (size_t i = nDataLength; i < nDataLength + nPaddingLen; i++) {
        pData[i] = nPaddingLen;
    }
}
  
void removePkcs7Padding(byte* pData, size_t& nDataLength, size_t nBlockSize = 16) {
    size_t nPaddingLen = pData[nDataLength - 1];
    if (nPaddingLen <= nBlockSize) {
        nDataLength -= nPaddingLen;
    }
}
  
#pragma endregion
  
#pragma region AES encryption / decryption

void writeArrayToSerial(byte tData[], int nLen) {
    for(int nIdx = 0; nIdx < nLen; nIdx++ ) Serial.printf("0x%02X ",tData[nIdx]);
    Serial.println(";");
}


/**
 * @brief encrypts the data 
 * As the IV is set to the last block, cover it to avoid destroying the users IV.
 * Ensure, pData has enough buffer allocated (!) (datalen/16 + 1) * 16;
 * @param pData Data to be encrypted (Buffer has to be allocated to align to 16 bytes block size)
 * @param nDataLen Length of data to be encrypted
 * @param tIV the initial vector to be used (16 bytes expected)
 * @param pKeyPhrase Pointer to the Keyphrase to be used 
 * @param nKeyLen Length of the keyphrase (has to be aligned to 16 bytes)
 */
int encryptData(void * pData, size_t nDataLen, byte tIV[], void * pKeyPhrase, size_t nKeyLen) {

    // safe the IV to be used without impacting the users one...
    byte _IV[16] = {0};
    memcpy(_IV,tIV,16);

    insertPkcs7Padding((byte *) pData,nDataLen,16);

    int nNumBlocks = (nDataLen / 16) + 1;

    br_aes_big_cbcenc_keys encCtx;
    br_aes_big_cbcenc_init(&encCtx, pKeyPhrase, nKeyLen);
    br_aes_big_cbcenc_run(&encCtx, _IV, pData, nNumBlocks * 16);
    return(nNumBlocks * 16);
}

/**
 * @brief encrypt the data by using the keyPhrase.
 */
void encryptData(void * pData, size_t nDataLen, byte tIV[], const char * pszKeyPhrase ) {
    // Prepare the KeyPhrase to be aligned to 16 bytes
    byte tKeyPhrase[16] = {0};
    convertStringToArray(pszKeyPhrase,tKeyPhrase, sizeof(tKeyPhrase));
    encryptData(pData,nDataLen, tIV, tKeyPhrase, sizeof(tKeyPhrase));
}

/**
 * encrypt the data to a base 64 string.
 * Using a cbc algo with 16 byte of block size.
 * @param strData String with the data to be encoded
 * @param tIV[] the initial vector (16 bytes)
 */
String encryptDataToBase64(String strData, byte tIV[]) {
    // Prepare the numbers.
    // - Data len
    // - needed blocks (i.W. 15/16 == 0 => allocate always one for padding)
    int nBlocks = (strData.length() / 16) + 1;
    byte tData[nBlocks * 16];
    memcpy(tData, strData.c_str(), strData.length());

    encryptData(tData,strData.length(),tIV,strAppPass.c_str());
    String strResult = getBase64EncodedString((const char*) tData,sizeof(tData));
    return strResult;
}


/**
 * @brief decrypts the data 
 * As the IV is set to the last block, it will be covered to avoid destroying the users IV.
 * Ensure, pData has enough buffer allocated (!) (datalen/16 + 1) * 16;
 * @param pData Data to be encrypted (Buffer has to be allocated to align to 16 bytes block size)
 * @param nDataLen Length of encrypted data (is aligned to 16 byte block size)
 * @param tIV the initial vector to be used (16 bytes expected)
 * @param pKeyPhrase Pointer to the Keyphrase to be used 
 * @param nKeyLen Length of the keyphrase (has to be aligned to 16 bytes)
 */
void decryptData(void * pData, size_t & nDataLen, const byte tIV[], void * pKeyPhrase, size_t nKeyLen) {
   
    // copy the IV to protect the users one...
    byte _IV[16] = {0};
    memcpy(_IV,tIV,16);
    br_aes_big_cbcdec_keys decCtx;
    br_aes_big_cbcdec_init(&decCtx, pKeyPhrase, nKeyLen);
    br_aes_big_cbcdec_run(&decCtx, _IV, pData, nDataLen);

    // Remove the padding and set the datalen to the correct one.
    removePkcs7Padding((byte *) pData,nDataLen,16);
}

/**
 * @brief decrypt the data by using the keyPhrase as string pointer.
 * @param pData Data to be encrypted (Buffer has to be allocated to align to 16 bytes block size)
 * @param nDataLen Length of encrypted data (is aligned to 16 byte block size) and will be set
 *                 to the decrypted data size after decryption.
 * @param tIV the initial vector to be used (16 bytes expected) - will stay untouched
 * @param pKeyPhrase Pointer to the Keyphrase to be used 
 * @param nKeyLen Length of the keyphrase (has to be aligned to 16 bytes)
 */
void decryptData(void * pData, size_t & nDataLen, const byte tIV[], const char * pszKeyPhrase ) {

    // Prepare the KeyPhrase to be aligned to 16 bytes
    // and decrpyt the data
    byte tKeyPhrase[16] = {0};
    convertStringToArray(pszKeyPhrase,tKeyPhrase, sizeof(tKeyPhrase));
    decryptData(pData,nDataLen, tIV, tKeyPhrase, sizeof(tKeyPhrase));
}

/**
 * @brief Decrypt the base 64 string, is using the application key to decrypt
 * @param strData the base64 encoded string
 * @param the IV - will stay untouched
 */
String decryptDataFromBase64(String strData, const byte tIV[]) {
    int nInputLen = strData.length();
    char *pszEncodedData = const_cast<char *>(strData.c_str());
    uint8_t tData[nInputLen];   // Is larger then needed...
    size_t nRealDataLen = base64_decode_chars(pszEncodedData,nInputLen,(char *)tData);
    size_t nDataLen = nRealDataLen;
    decryptData(tData,nDataLen,tIV,strAppPass.c_str());
    return String((char*)tData).substring(0, nDataLen);
}

#pragma endregion


String getTokenKey() {
    return(String(APPL_SECURITY_TOKEN_KEY));
}

/**
 * Create a new Auth Token
 * {
 *      "IV":                   // contains the initial vector - for the encrypted Data"
 *      "Data": {               // Data is encrypted (base64) and contains this structure
 *          "TS":               // Timestamp (avoids salt and is for token timeout)    
 *          "IP":               // The IP of the client
 *          "K" :               // An application identifier as string              
 *       }
 * }
 * Millis first - so no salt is needed...
 * Millis are used to detect tokens no longer valid...
 * Insert the client IP - to identify the client,
 * Insert an application key
 */
String getNewAuthToken(String &strClientIPAddress) {
    DEBUG_FUNC_START();
    JSON_DOC_STATIC(oToken,JSON_DOC_TOKEN_SIZE);
    // StaticJsonDocument<JSON_DOC_TOKEN_SIZE> oToken;
    oToken["TS"] = millis();
    oToken["IP"] = strClientIPAddress;
    oToken["K"] = getTokenKey();
    // Store the token into a String a build a base 64 encoded string...
    String strData;
    serializeJson(oToken,strData);
    String strData64 = getBase64EncodedString(strData);
    
    // Build the final token now...
    uint8_t tIV[16];
    oToken.clear();
    oToken["IV"]   = getRandomNumbers(tIV,sizeof(tIV));
    oToken["Data"] = encryptDataToBase64(strData64,tIV);

    // Get the final (encrypted) Token
    strData.clear();
    serializeJson(oToken,strData);    
    DEBUG_INFOS("- Access token content        : %s", strData.c_str());
    return(getBase64EncodedString(strData));
}

void setNewAuthHeader(AsyncWebServerRequest *pRequest, AsyncWebServerResponse *pResponse){
    String strIPAddress;
    if(pRequest) strIPAddress = pRequest->client()->remoteIP().toString();
    if(pResponse) pResponse->addHeader("AUTHTOKEN",getNewAuthToken(strIPAddress));
}

/**
 * @brief Write the decrypted token into a json doc.
 *        
 * @param strEncBase64Token encrypted token as received from the client... in Base64
 * @param oTokenDoc Document to store the token - data inside will be lost...
 * @return true if successfully decoded and written.
 *  */
bool writeDecryptedTokenToJsonDoc(String &strEncBase64Token, JsonDocument &oTokenDoc) {
    DEBUG_FUNC_START_PARMS("%s",strEncBase64Token.c_str());
    bool bIsValid = false;
    String strNativeToken = getBase64DecodedString(strEncBase64Token);
    deserializeJson(oTokenDoc,strNativeToken);
    String strIV = oTokenDoc["IV"]; 
    if(strIV.length() > 0 && strIV.length() < 64) {
        byte tIV[64];  // to avoid buffer overflow...
        int nIVSize = base64_decode_chars(strIV.c_str(),strIV.length(),(char*)tIV);
        if(nIVSize == 16) {
            String strSourceData = oTokenDoc["Data"];
            String strTokenData = decryptDataFromBase64(strSourceData,tIV);
            String strDocData = getBase64DecodedString(strTokenData);
            deserializeJson(oTokenDoc,strDocData);
            bIsValid = true;
        }
        else {
            DEBUG_INFOS(" -- length of IV is invalid (%d) - expected 16 bytes...",nIVSize);
        }
    } 
    return(bIsValid);
}

/**
 * @brief Check if the auth token is valid
 * {
 *  - Token Key has to match
 *  - IP address has to match
 *  - token live time may not be expired
 * }
 * @param strToken Base64 encrypted token
 * @param strClientIPAddress 
 * @return 
 * */
bool isAuthTokenValid(String &strEncBase64Token, String &strClientIPAddress) {
    DEBUG_FUNC_START_PARMS("%s,%s",strEncBase64Token.c_str(),strClientIPAddress.c_str());
    bool bIsValid = false;
    if(strEncBase64Token && strEncBase64Token.length() > 2) {
        JSON_DOC_STATIC(oTokenDoc,JSON_DOC_TOKEN_SIZE);
        if(writeDecryptedTokenToJsonDoc(strEncBase64Token,oTokenDoc)) {
            if (  // My token
                oTokenDoc["K"]  == getTokenKey()   &&
                oTokenDoc["IP"] == strClientIPAddress ) {
                // Check the timestamp of the token...
                unsigned long ulMillis = oTokenDoc["TS"];
                if((ulMillis + TOKEN_TIME_ALIVE) > millis() ) bIsValid = true;
            }
        }
    }
    DEBUG_FUNC_END_PARMS("%d",bIsValid);
    return(bIsValid);

}
 
