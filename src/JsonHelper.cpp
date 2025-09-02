#include <Appl.h>
#include <JsonHelper.h>
#include <DevelopmentHelper.h>


namespace LSC {

    JsonObject createPayloadStructure(const char* pszCommand, const char *pszDataType, JsonDocument &oPayloadDoc, const char *pszData) {
        DEBUG_FUNC_START_PARMS("%s,%s,...",NULL_POINTER_STRING(pszCommand),NULL_POINTER_STRING(pszDataType));

        char pszBuffer[256];
        sprintf(pszBuffer,"{\"command\":\"%s\",\"data\":\"%s\"",pszCommand,pszDataType);
        String strData = pszBuffer;
        if(pszData) {
            strData += ",\"payload\":";
            strData += pszData;
        }
        strData += "}";
        DEBUG_INFO(" --> creating payload:");
        DEBUG_INFOS("%s",strData.c_str());
        DeserializationError error = deserializeJson(oPayloadDoc,strData);
        if(error) {
            ApplLogErrorWithParms("WS: Error creating payload structure : %s",error.c_str());
        } 
        DEBUG_FUNC_END();
        return(pszData == nullptr ? CreateJsonObject(oPayloadDoc,"payload") : oPayloadDoc["payload"]);
    }

    JsonObject createPayloadStructure(const __FlashStringHelper* pszCommand, const __FlashStringHelper* pszDataType, JsonDocument &oPayloadDoc, const char *pszPayload) {
        return(createPayloadStructure((const char*) pszCommand,(const char*) pszDataType,oPayloadDoc,pszPayload));
    }
}