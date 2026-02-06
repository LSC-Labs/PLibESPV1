#pragma once
#include <ArduinoJson.h>
#include <IPAddress.h>

#define GetJsonDocumentAsObject(oDoc) oDoc.as<JsonObject>()

#if ARDUINOJSON_VERSION_MAJOR < 7
    #define JSON_DOC(n,s) n = DynamicJsonDocument(s)
    #define JSON_DOC_STATIC(n,s) StaticJsonDocument<s> n
    #define CreateJsonArray(doc,name) doc.createNestedArray(name)
    #define CreateEmptyJsonArray(doc,name) doc.createNestedArray()
    #define CreateJsonObject(doc,name) doc.createNestedObject(name)
    #define CreateEmptyJsonObject(doc) doc.createNestedObject()
    #define GetJsonObject(doc,name)  doc[name]
    #define GetOrCreateJsonObject(doc,name) (doc[name] ? doc[name] : doc.createNestedObject(name))
    #define JsonKeyExists(obj,key,type) obj.containsKey(ke)
#else
    #define JSON_DOC(n,s)        JsonDocument n
    #define JSON_DOC_STATIC(n,s) JsonDocument n
    #define JsonKeyExists(obj,key,type) obj[key].is<type>()
    #define CreateEmptyJsonArray(doc)    doc.add<JsonArray>()
    #define CreateJsonArray(doc,name)    doc[name].to<JsonArray>()
    #define CreateEmptyJsonObject(doc)   doc.add<JsonObject>()
    #define CreateJsonObject(doc,name)   doc[name].to<JsonObject>()
    #define GetJsonObject(doc,name)      doc[name].as<JsonObject>()
    #define GetOrCreateJsonObject(doc,name) (JsonKeyExists(oNode,oEntry.pszName,JsonObject) ? GetJsonObject(oNode,oEntry.pszName) : CreateJsonObject(oNode,oEntry.pszName))
    
    // ;doc[name].is<JsonObject> ? doc[name].as<JsonObject>() : doc[name].to<JsonObject>()
    
#endif

namespace LSC {
    JsonObject createPayloadStructure(const __FlashStringHelper* pszCommand, const __FlashStringHelper* pszDataType, JsonDocument &oPayloadDoc, const char *pszData = nullptr);
    JsonObject createPayloadStructure(const char* pszCommand, const char *pszDataType, JsonDocument &oPayloadDoc,const char *pszData = nullptr);


    bool ICACHE_FLASH_ATTR setJsonValueIfNot(JsonObject & oSource,const char* pszKey,  String & strTarget, const char  *pszHiddenValue);

    bool ICACHE_FLASH_ATTR setJsonValue(JsonObject & oSource,const char* pszKey,  String    & strTarget);
    bool ICACHE_FLASH_ATTR setJsonValue(JsonObject & oSource,const char* pszKey,  float     * pTarget);
    bool ICACHE_FLASH_ATTR setJsonValue(JsonObject & oSource,const char* pszKey,  int       * pTarget);
    bool ICACHE_FLASH_ATTR setJsonValue(JsonObject & oSource,const char* pszKey,  bool      * pTarget);
    bool ICACHE_FLASH_ATTR setJsonValue(JsonObject & oSource,const char* pszKey,  IPAddress & pTarget);

    /*
    bool ICACHE_FLASH_ATTR setJsonValue(JsonObject & oSource,const __FlashStringHelper* pszKey,  String    & strTarget);
    bool ICACHE_FLASH_ATTR setJsonValue(JsonObject & oSource,const __FlashStringHelper* pszKey,  float     * pTarget);
    bool ICACHE_FLASH_ATTR setJsonValue(JsonObject & oSource,const __FlashStringHelper* pszKey,  int       * pTarget);
    bool ICACHE_FLASH_ATTR setJsonValue(JsonObject & oSource,const __FlashStringHelper* pszKey,  bool      * pTarget);
    bool ICACHE_FLASH_ATTR setJsonValue(JsonObject & oSource,const __FlashStringHelper* pszKey,  IPAddress & pTarget);
    */
}

