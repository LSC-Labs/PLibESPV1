/**
 * @file JsonHelper.cpp
 * @brief Backend independent JSON helpers (JsonNode or ArduinoJson).
 * @see JsonHelper.h
 */
#include <JsonHelper.h>
#include <string.h>
#include <stdlib.h>

namespace LSC {

#ifdef LSC_JSON_USE_ARDUINOJSON
// ***************************************************************************
// ArduinoJson backend
// ***************************************************************************

    JsonObject createPayloadStructure(const char* pszCommand, const char *pszDataType, JsonDocument &oPayloadDoc, const char *pszPayload) {
        oPayloadDoc.clear();
        JsonObject oRoot = jsonRoot(oPayloadDoc);
        oRoot["command"] = pszCommand  ? pszCommand  : "";
        oRoot["data"]    = pszDataType ? pszDataType : "";
        if(pszPayload) {
            #if ARDUINOJSON_VERSION_MAJOR < 7
                DynamicJsonDocument oPayload(strlen(pszPayload) * 2 + 64);
            #else
                JsonDocument oPayload;
            #endif
            if(deserializeJson(oPayload, pszPayload) == DeserializationError::Ok) {
                oRoot["payload"] = oPayload.as<JsonVariant>();
                JsonObject oResult = oRoot["payload"].as<JsonObject>();
                if(!oResult.isNull()) return(oResult);
            } else {
                oRoot["payload"] = pszPayload;   // no JSON - store as string
            }
            return(JsonObject());
        }
        return(JSON_CREATE_OBJECT(oRoot,"payload"));
    }

    bool setJsonValue(JsonObject oSource, const char* pszKey, String & strTarget) {
        if(oSource.isNull() || !pszKey) return(false);
        JsonVariant oValue = oSource[pszKey];
        if(oValue.isNull()) return(false);
        strTarget = oValue.as<String>();
        return(true);
    }

    bool setJsonValue(JsonObject oSource, const char* pszKey, float * pTarget) {
        if(oSource.isNull() || !pszKey || !pTarget) return(false);
        JsonVariant oValue = oSource[pszKey];
        if(oValue.is<float>())       { *pTarget = oValue.as<float>(); return(true); }
        if(oValue.is<const char*>()) { *pTarget = (float) atof(oValue.as<const char*>()); return(true); }
        return(false);
    }

    bool setJsonValue(JsonObject oSource, const char* pszKey, int * pTarget) {
        if(oSource.isNull() || !pszKey || !pTarget) return(false);
        JsonVariant oValue = oSource[pszKey];
        if(oValue.is<int>())         { *pTarget = oValue.as<int>(); return(true); }
        if(oValue.is<const char*>()) { *pTarget = atoi(oValue.as<const char*>()); return(true); }
        return(false);
    }

    bool setJsonValue(JsonObject oSource, const char* pszKey, unsigned long * pTarget) {
        if(oSource.isNull() || !pszKey || !pTarget) return(false);
        JsonVariant oValue = oSource[pszKey];
        if(oValue.is<unsigned long>()) { *pTarget = oValue.as<unsigned long>(); return(true); }
        if(oValue.is<const char*>())   { *pTarget = strtoul(oValue.as<const char*>(),nullptr,10); return(true); }
        return(false);
    }

    bool setJsonValue(JsonObject oSource, const char* pszKey, bool * pTarget) {
        if(oSource.isNull() || !pszKey || !pTarget) return(false);
        JsonVariant oValue = oSource[pszKey];
        if(oValue.is<bool>()) { *pTarget = oValue.as<bool>(); return(true); }
        if(oValue.is<const char*>()) {
            const char *psz = oValue.as<const char*>();
            *pTarget = strcasecmp(psz,"true") == 0 || strcasecmp(psz,"on") == 0 || strcmp(psz,"1") == 0;
            return(true);
        }
        if(oValue.is<int>()) { *pTarget = oValue.as<int>() != 0; return(true); }
        return(false);
    }

#else
// ***************************************************************************
// JsonNode backend
// ***************************************************************************

    JsonNode * createPayloadStructure(const char* pszCommand, const char *pszDataType, JsonNode &oPayloadDoc, const char *pszPayload) {
        oPayloadDoc.clear();
        // JSON text is parsed, other text stored as string (see JsonNode::createPayloadStructure)
        return(oPayloadDoc.createPayloadStructure(pszCommand ? pszCommand : "", pszDataType ? pszDataType : "", pszPayload));
    }

    bool setJsonValue(JsonNode * pSource, const char* pszKey, String & strTarget) {
        return(pSource && pszKey ? pSource->storeValueIf(pszKey,strTarget) : false);
    }

    bool setJsonValue(JsonNode * pSource, const char* pszKey, float * pTarget) {
        return(pSource && pszKey && pTarget ? pSource->storeValueIf(pszKey,pTarget) : false);
    }

    bool setJsonValue(JsonNode * pSource, const char* pszKey, int * pTarget) {
        return(pSource && pszKey && pTarget ? pSource->storeValueIf(pszKey,pTarget) : false);
    }

    bool setJsonValue(JsonNode * pSource, const char* pszKey, unsigned long * pTarget) {
        if(!pSource || !pszKey || !pTarget) return(false);
        JsonNode *pNode = pSource->find(pszKey);
        if(!pNode || !pNode->isNumberValue()) return(false);
        *pTarget = pNode->getValueAsUnsignedLong(*pTarget);
        return(true);
    }

    bool setJsonValue(JsonNode * pSource, const char* pszKey, bool * pTarget) {
        return(pSource && pszKey && pTarget ? pSource->storeValueIf(pszKey,pTarget) : false);
    }

#endif

// ***************************************************************************
// Backend independent helpers
// ***************************************************************************

    bool setJsonValueIfNot(JSON_OBJECT oSource, const char* pszKey, String & strTarget, const char *pszHiddenValue) {
        String strData;
        if(!setJsonValue(oSource,pszKey,strData)) return(false);
        if(pszHiddenValue && strcmp(strData.c_str(),pszHiddenValue) == 0) return(false);
        strTarget = strData;
        return(true);
    }

    bool setJsonValue(JSON_OBJECT oSource, const char* pszKey, IPAddress & oTarget) {
        String strData;
        if(!setJsonValue(oSource,pszKey,strData)) return(false);
        oTarget.fromString(strData.c_str());
        return(true);
    }

#ifndef NATIVE_RUNTIME
    JSON_OBJECT createPayloadStructure(const __FlashStringHelper* pszCommand, const __FlashStringHelper* pszDataType, JSON_DOCUMENT &oPayloadDoc, const char *pszPayload) {
        String strCommand  = pszCommand;
        String strDataType = pszDataType;
        return(createPayloadStructure(strCommand.c_str(), strDataType.c_str(), oPayloadDoc, pszPayload));
    }

    bool setJsonValue(JSON_OBJECT oSource, const __FlashStringHelper* pszKey, String & strTarget) {
        String strKey(pszKey);
        return(setJsonValue(oSource,strKey.c_str(),strTarget));
    }

    bool setJsonValue(JSON_OBJECT oSource, const __FlashStringHelper* pszKey, float * pTarget) {
        String strKey(pszKey);
        return(setJsonValue(oSource,strKey.c_str(),pTarget));
    }

    bool setJsonValue(JSON_OBJECT oSource, const __FlashStringHelper* pszKey, int * pTarget) {
        String strKey(pszKey);
        return(setJsonValue(oSource,strKey.c_str(),pTarget));
    }

    bool setJsonValue(JSON_OBJECT oSource, const __FlashStringHelper* pszKey, bool * pTarget) {
        String strKey(pszKey);
        return(setJsonValue(oSource,strKey.c_str(),pTarget));
    }

    bool setJsonValue(JSON_OBJECT oSource, const __FlashStringHelper* pszKey, IPAddress & oTarget) {
        String strKey(pszKey);
        return(setJsonValue(oSource,strKey.c_str(),oTarget));
    }
#endif

}
