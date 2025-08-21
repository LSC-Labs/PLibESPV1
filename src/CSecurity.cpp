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

const String strAppPass = "Kis8%$vvQ@ä+qw12";  // 16 chars !

#pragma region Base64 encoding/decoding

/// @brief Get a the base64 decoded data (as string)
/// @param strBase64String 
/// @return 
String getBase64DecodedString(String &strBase64String) {
    DEBUG_FUNC_START_PARMS("%s",strBase64String.c_str());
    char tData[strBase64String.length()];
    memset(tData,'\0',sizeof(tData));
    base64_decode_chars(strBase64String.c_str(),strBase64String.length(),tData);
    DEBUG_FUNC_END_PARMS("%s",tData);
    return(String(tData));
}

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

void convertStringToHex(const String &str, uint8_t *hexArray, size_t arraySize) {
    memset(hexArray,'\0',arraySize);
    for (size_t i = 0; i < arraySize; ++i) {
        if(str.length() > i) hexArray[i] = str[i];
    }
  }

#pragma region Random numbers - as good as possible without hw support ...
  bool _bRandomNumbersInitialized = false;
  const void initializeRandomNumbers(bool bForce) {
      if(bForce || !_bRandomNumbersInitialized) {
          randomSeed(millis());
          _bRandomNumbersInitialized = true;
      }
  }
  
  const String getRandomNumbers(byte tBuffer[], int nBufferLen) {
      initializeRandomNumbers(false);
      String strNumbers;
      for(int nIdx = 0; nIdx < nBufferLen; nIdx++) {
          tBuffer[nIdx] = random(1,254);
          strNumbers += tBuffer[nIdx];
          strNumbers += ",";
      }
      char tData[ nBufferLen * 2];
      base64_encode_chars((char*)tBuffer,nBufferLen,tData);
      String strEncoded = (char*)tData;
      return(strEncoded);
  }
  #pragma endregion
  
#pragma region PKCS7 Padding 
  
  void insertPkcs7Padding(byte* pData, size_t nDataLength, size_t nBlockSize) {
      size_t nPaddingLen = nBlockSize - (nDataLength % nBlockSize);
      for (size_t i = nDataLength; i < nDataLength + nPaddingLen; i++) {
        pData[i] = nPaddingLen;
      }
    }
  
  void removePkcs7Padding(byte* pData, size_t& nDataLength, size_t nBlockSize) {
      size_t nPaddingLen = pData[nDataLength - 1];
      if (nPaddingLen <= nBlockSize) {
          nDataLength -= nPaddingLen;
      }
  }
  
  #pragma endregion
  
#pragma region AES encryption / decryption
const String encryptDataToBase64(String strData, byte tIV[]) {
    // Load into tData and fill with paddings.
    int nLen = strData.length();
    int nBlocks = nLen / 16 + 1;
    // uint8_t nPadding = nBlocks * 16 - nLen;
    uint8_t tData[nBlocks * 16];
    memcpy(tData, strData.c_str(), nLen);
    insertPkcs7Padding(tData, nLen, 16);

    uint8_t aes_key[16];
    convertStringToHex(strAppPass,aes_key,sizeof(aes_key));

    br_aes_big_cbcenc_keys encCtx;
    br_aes_big_cbcenc_init(&encCtx, aes_key, 16);
    br_aes_big_cbcenc_run(&encCtx, tIV, tData, nBlocks * 16);

    String strResult = getBase64EncodedString((const char*) tData,sizeof(tData));
    return strResult;

}

const String decryptDataFromBase64(String strData, byte tIV[]) {
    DEBUG_FUNC_START_PARMS("%s",strData.c_str());
    int nInputLen = strData.length();
    char *pszEncodedData = const_cast<char *>(strData.c_str());
    uint8_t tData[nInputLen];   // Larger then needed...
    int nRealDataLen = base64_decode_chars(pszEncodedData,nInputLen,(char *)tData);
    uint8_t aes_key[16];
    convertStringToHex(strAppPass,aes_key,sizeof(aes_key));
    int nBlocks = nRealDataLen / 16;
    br_aes_big_cbcdec_keys decCtx;
    br_aes_big_cbcdec_init(&decCtx, aes_key, 16);
    br_aes_big_cbcdec_run(&decCtx, tIV, tData, nBlocks * 16);

    size_t nDataLen = nRealDataLen;
    removePkcs7Padding(tData, nDataLen, 16);
    return String((char*)tData).substring(0, nDataLen);
}

#pragma endregion


const String getTokenKey() {
    return(String("ds$woEir=ncn<jek"));
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
const String getNewAuthToken(String &strClientIPAddress) {
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

/// @brief Write the decrypted token into a json doc.
//        
/// @param strEncBase64Token encrypted token as received from the client... in Base64
/// @param oTokenDoc Document to store the token - data inside will be lost...
/// @return true if successfully decoded and written.
const bool writeDecryptedTokenToJsonDoc(String &strEncBase64Token, JsonDocument &oTokenDoc) {
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

/// @brief Check if the Auth Token is valid
/// @param strToken Basae64 encrypted token
/// @param strClientIPAddress 
/// @return 
const bool isAuthTokenValid(String &strEncBase64Token, String &strClientIPAddress) {
    DEBUG_FUNC_START_PARMS("%s,%s",strEncBase64Token.c_str(),strClientIPAddress.c_str());
    bool bIsValid = false;
    if(strEncBase64Token && strEncBase64Token.length() > 2) {
        JSON_DOC_STATIC(oTokenDoc,JSON_DOC_TOKEN_SIZE);
        // StaticJsonDocument<JSON_DOC_TOKEN_SIZE> oTokenDoc;
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
 
