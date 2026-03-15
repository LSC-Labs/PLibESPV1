#include <Appl.h>
#include <JsonHelper.h>
#include <DevelopmentHelper.h>
#include <LSCUtils.h>


namespace LSC {

    /**
     * @brief create payload structure, with command, data and payload fields
     * to be used in web socket and web rest api's
     * Payload document content will be replaced
     */
    JsonObject createPayloadStructure(const char* pszCommand, const char *pszDataType, JsonDocument &oPayloadDoc, const char *pszPayload) {
        char pszBuffer[256];
        sprintf(pszBuffer,"{\"command\":\"%s\",\"data\":\"%s\"",pszCommand,pszDataType);
        String strData = pszBuffer;
        if(pszPayload) {
            strData += ",\"payload\":";
            strData += pszPayload;
        }
        strData += "}";
        DeserializationError error = deserializeJson(oPayloadDoc,strData);
        if(error) {
            ApplLogErrorWithParms("WS: Error creating payload structure : %s",error.c_str());
        } 
        return(pszPayload == nullptr ? CreateJsonObject(oPayloadDoc,"payload") : oPayloadDoc["payload"]);
    }

    JsonObject createPayloadStructure(const __FlashStringHelper* pszCommand, const __FlashStringHelper* pszDataType, JsonDocument &oPayloadDoc, const char *pszPayload) {
        String strCommand = pszCommand;
        String strDataType = pszDataType;
        return(createPayloadStructure(strCommand.c_str(), strDataType.c_str(),oPayloadDoc,pszPayload));
    }


    /**
     * Special version to store a string value, if it does not match the HiddenValue.
     * Use this to set your password - if it is not the hidden password
     */
    bool ICACHE_FLASH_ATTR setJsonValueIfNot(JsonObject & oSource, const char* pszKey, String & strTarget, const char *pszHiddenValue) {
        bool bResult = false;
        if(JsonKeyExists(oSource,pszKey,String)) {
            String strData = oSource[pszKey].as<String>();
            if(!strData.equals(pszHiddenValue)) {
                strTarget = strData;
                bResult = true;
            }
        } 
        return(bResult);
    }

    /**
     * Set an ipaddress stored in the json object
     */
    bool ICACHE_FLASH_ATTR setJsonValue(JsonObject & oSource, const char* pszKey, IPAddress & oTarget) {
        bool bResult = false;
        String strData;
        if(setJsonValue(oSource,pszKey,strData)) {
            oTarget.fromString(strData);
            bResult = true;
        }
        return(bResult);
    }

    /**
     * Set a String to the value found in the json object
     */
    bool ICACHE_FLASH_ATTR setJsonValue(JsonObject & oSource, const char* pszKey, String & strTarget) {
        bool bResult = false;
        if(JsonKeyExists(oSource,pszKey,String)) {
            strTarget = oSource[pszKey].as<String>();
            bResult = true;
        } 
        return(bResult);
    }

    /**
     * Set a float value, as stored in the json object.
     * the float can be native (0.xxx)
     * or as a string ("0.xx")
     * both version are accepted
     */
    bool ICACHE_FLASH_ATTR setJsonValue(JsonObject & oSource, const char* pszKey, float * pTarget) {
        bool bResult = false;

        // Either 0.xxx or "0.xxx" first is float, second is string...
        if(JsonKeyExists(oSource,pszKey,float) || JsonKeyExists(oSource,pszKey,String)) {
            *pTarget = oSource[pszKey].as<float>();
            bResult = true;
        } 
        return(bResult);
    } 

    /**
     * Set a float value, as stored in the json object.
     * the float can be native (0.xxx)
     * or as a string ("0.xx")
     * both version are accepted
     */
    bool ICACHE_FLASH_ATTR setJsonValue(JsonObject & oSource, const char* pszKey, unsigned long * pTarget) {
        bool bResult = false;

        // Either 0.xxx or "0.xxx" first is float, second is string...
        if(JsonKeyExists(oSource,pszKey,unsigned long) || JsonKeyExists(oSource,pszKey,String)) {
            *pTarget = oSource[pszKey].as<unsigned long>();
            bResult = true;
        } 
        return(bResult);
    } 

     /**
     * Set a bool value, as stored in the json object.
     * the bool can be native (true)
     * or as a string ("true")
     * also as "0", "-", "off", "false"... will become false (case insensitive)
     * 
     * It will be checked against false values... all other values are true (!)
     */
    bool ICACHE_FLASH_ATTR setJsonValue(JsonObject & oSource, const char* pszKey, bool * pTarget) {
        bool bResult = false;
        if(JsonKeyExists(oSource,pszKey,bool)  || JsonKeyExists(oSource,pszKey,String)) {
            if(JsonKeyExists(oSource,pszKey,bool)) *pTarget = oSource[pszKey].as<bool>(); 
            else {
                String strValue = oSource[pszKey].as<String>();
                strValue.toLowerCase();
                *pTarget = !(LSC::isFalseValue(strValue.c_str()));
            }
            bResult = true;
        } 
        return(bResult);
    }

    /**
     * Set a int value, as stored in the json object
     * either as int value (nnn)
     * or as string ("nnn")
     */
    bool ICACHE_FLASH_ATTR setJsonValue(JsonObject & oSource, const char* pszKey, int * pTarget) {
        bool bResult = false;
        Serial.println(" - set int...");
        if(JsonKeyExists(oSource,pszKey,int) || JsonKeyExists(oSource,pszKey,String)) {
            *pTarget = oSource[pszKey].as<int>();
            bResult = true;
        } 
        return(bResult);
    }

    
    bool ICACHE_FLASH_ATTR setJsonValue(JsonObject & oSource,const __FlashStringHelper* pszKey,  String    & strTarget) {
        String strKey(pszKey);
        return(setJsonValue(oSource,strKey.c_str(),strTarget));
    }
    
   
    bool ICACHE_FLASH_ATTR setJsonValue(JsonObject & oSource,const __FlashStringHelper* pszKey,  float     * pTarget){
        String strKey(pszKey);
        return(setJsonValue(oSource,strKey.c_str(),pTarget));
    }
   
    bool ICACHE_FLASH_ATTR setJsonValue(JsonObject & oSource,const __FlashStringHelper* pszKey,  int       * pTarget){
        String strKey(pszKey);
        return(setJsonValue(oSource,strKey.c_str(),pTarget));
    }
    bool ICACHE_FLASH_ATTR setJsonValue(JsonObject & oSource,const __FlashStringHelper* pszKey,  bool      * pTarget){
        String strKey(pszKey);
        return(setJsonValue(oSource,strKey.c_str(),pTarget));
    }
    bool ICACHE_FLASH_ATTR setJsonValue(JsonObject & oSource,const __FlashStringHelper* pszKey,  IPAddress & pTarget){
        String strKey(pszKey);
        return(setJsonValue(oSource,strKey.c_str(),pTarget));
    }
    
}