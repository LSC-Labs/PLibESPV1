#include <Appl.h>
#include <JsonHelper.h>
#include <DevelopmentHelper.h>
#include <LSCUtils.h>


namespace LSC {

    /**
     * @brief Creates a standard command/data/payload JSON structure.
     *
     * The target document is replaced. If pszPayload is nullptr, an empty
     * payload object is created and returned so callers can fill it.
     *
     * @param pszCommand Command name.
     * @param pszDataType Logical data type of the payload.
     * @param oPayloadDoc Target ArduinoJson document.
     * @param pszPayload Optional serialized JSON payload.
     * @return Payload object inside oPayloadDoc.
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

    /**
     * @brief Flash-string overload for createPayloadStructure().
     */
    JsonObject createPayloadStructure(const __FlashStringHelper* pszCommand, const __FlashStringHelper* pszDataType, JsonDocument &oPayloadDoc, const char *pszPayload) {
        String strCommand = pszCommand;
        String strDataType = pszDataType;
        return(createPayloadStructure(strCommand.c_str(), strDataType.c_str(),oPayloadDoc,pszPayload));
    }


    /**
     * @brief Stores a string value unless it equals the hidden-value mask.
     *
     * Use this for passwords or tokens in configuration forms: masked values do
     * not overwrite the existing stored secret.
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
     * @brief Reads an IPAddress value from a JSON string field.
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
     * @brief Reads a String value from a JSON field.
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
     * @brief Reads a float from a JSON numeric or string field.
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
     * @brief Reads an unsigned long from a JSON numeric or string field.
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
     * @brief Reads a bool from a JSON bool or string field.
     *
     * String values are interpreted by checking known false values. Unknown
     * strings become true.
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
     * @brief Reads an int from a JSON numeric or string field.
     */
    bool ICACHE_FLASH_ATTR setJsonValue(JsonObject & oSource, const char* pszKey, int * pTarget) {
        bool bResult = false;
        if(JsonKeyExists(oSource,pszKey,int) || JsonKeyExists(oSource,pszKey,String)) {
            *pTarget = oSource[pszKey].as<int>();
            bResult = true;
        } 
        return(bResult);
    }

    /**
     * @brief Flash-string key overload for reading a String.
     */
    bool ICACHE_FLASH_ATTR setJsonValue(JsonObject & oSource,const __FlashStringHelper* pszKey,  String    & strTarget) {
        String strKey(pszKey);
        return(setJsonValue(oSource,strKey.c_str(),strTarget));
    }
    
    /**
     * @brief Flash-string key overload for reading a float.
     */
    bool ICACHE_FLASH_ATTR setJsonValue(JsonObject & oSource,const __FlashStringHelper* pszKey,  float     * pTarget){
        String strKey(pszKey);
        return(setJsonValue(oSource,strKey.c_str(),pTarget));
    }
   
    /**
     * @brief Flash-string key overload for reading an int.
     */
    bool ICACHE_FLASH_ATTR setJsonValue(JsonObject & oSource,const __FlashStringHelper* pszKey,  int       * pTarget){
        String strKey(pszKey);
        return(setJsonValue(oSource,strKey.c_str(),pTarget));
    }

    /**
     * @brief Flash-string key overload for reading a bool.
     */
    bool ICACHE_FLASH_ATTR setJsonValue(JsonObject & oSource,const __FlashStringHelper* pszKey,  bool      * pTarget){
        String strKey(pszKey);
        return(setJsonValue(oSource,strKey.c_str(),pTarget));
    }

    /**
     * @brief Flash-string key overload for reading an IPAddress.
     */
    bool ICACHE_FLASH_ATTR setJsonValue(JsonObject & oSource,const __FlashStringHelper* pszKey,  IPAddress & pTarget){
        String strKey(pszKey);
        return(setJsonValue(oSource,strKey.c_str(),pTarget));
    }
    
}
