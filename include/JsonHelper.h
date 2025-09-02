#pragma once
#include <ArduinoJson.h>

#define GetJsonDocumentAsObject(oDoc) oDoc.as<JsonObject>()



#if ARDUINOJSON_VERSION_MAJOR < 7
    #define JSON_DOC(n,s) n = DynamicJsonDocument(s)
    #define JSON_DOC_STATIC(n,s) StaticJsonDocument<s> n
    #define CreateJsonArray(doc,name) doc.createNestedArray(name)
    #define CreateEmptyJsonArray(doc,name) doc.createNestedArray()
    #define CreateJsonObject(doc,name) doc.createNestedObject(name)
    #define CreateEmptyJsonObject(doc) doc.createNestedObject()
#else
    #define JSON_DOC(n,s)        JsonDocument n
    #define JSON_DOC_STATIC(n,s) JsonDocument n
    #define CreatemptyJsonArray(doc)    doc.add<JsonArray>()
    #define CreateJsonArray(doc,name)   doc[name].to<JsonArray>()
    #define CreateEmptyJsonObject(doc)  doc.add<JsonObject>()
    #define CreateJsonObject(doc,name)  doc[name].to<JsonObject>()
#endif

namespace LSC {
    JsonObject createPayloadStructure(const __FlashStringHelper* pszCommand, const __FlashStringHelper* pszDataType, JsonDocument &oPayloadDoc, const char *pszData = nullptr);
    JsonObject createPayloadStructure(const char* pszCommand, const char *pszDataType, JsonDocument &oPayloadDoc,const char *pszData = nullptr);
}

