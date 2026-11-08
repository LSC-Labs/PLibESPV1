#pragma once
/**
 * @file JsonHelper.h
 * @brief Backend independent JSON macros and helpers.
 *
 * The library uses JsonNode (CJsonNode) as JSON implementation. Code written
 * with the macros below can be switched to ArduinoJson (v6 or v7) without
 * changes by defining
 *
 *      -D LSC_JSON_USE_ARDUINOJSON
 *
 * (and adding ArduinoJson to lib_deps). Without the define, ArduinoJson is
 * not included and not needed at all.
 *
 * Note: The module interfaces of the library (IModule::writeConfigTo(), ...)
 * always use JsonNode. The switch affects code written against these macros.
 *
 * Handles
 * -------
 *  JSON_DOCUMENT  Document type (owns the data)        JsonNode   | JsonDocument
 *  JSON_OBJECT    Handle to an object (cheap to copy)  JsonNode * | JsonObject
 *  JSON_ARRAY     Handle to an array  (cheap to copy)  JsonNode * | JsonArray
 *
 * Usage
 * -----
 *      JSON_DOC(oDoc);
 *      JSON_OBJECT oRoot = JSON_ROOT(oDoc);
 *      JSON_SET(oRoot, "name", "value");
 *      JSON_OBJECT oSub  = JSON_CREATE_OBJECT(oRoot, "sub");
 *      JSON_SET(oSub, "count", 5);
 *      JSON_ARRAY  oList = JSON_CREATE_ARRAY(oRoot, "list");
 *      JSON_ADD_VALUE(oList, 1);
 *      String strText;
 *      JSON_SERIALIZE(oDoc, strText);
 *      if(JSON_PARSE(oDoc, "{\"a\":1}")) { int a = JSON_GET_INT(JSON_ROOT(oDoc), "a", 0); }
 */

#ifdef NATIVE_RUNTIME
    #include <Network.h>
#else
    #include <IPAddress.h>
#endif

#ifdef LSC_JSON_USE_ARDUINOJSON
// ***************************************************************************
// ArduinoJson backend
// ***************************************************************************
    #include <ArduinoJson.h>

    #define LSC_JSON_BACKEND_NAME               "ArduinoJson"

    #define JSON_DOCUMENT                       JsonDocument
    #define JSON_OBJECT                         JsonObject
    #define JSON_ARRAY                          JsonArray

    #if ARDUINOJSON_VERSION_MAJOR < 7
        // Optional 2nd parameter: capacity in bytes (default 2048)
        #define JSON_DOC(n, ...)                DynamicJsonDocument n(LSC::jsonDocSize(0, ##__VA_ARGS__))
        #define JSON_CREATE_OBJECT(h,name)      (h).createNestedObject(name)
        #define JSON_CREATE_ARRAY(h,name)       (h).createNestedArray(name)
        #define JSON_ADD_OBJECT(arr)            (arr).createNestedObject()
        #define JSON_ADD_ARRAY(arr)             (arr).createNestedArray()
    #else
        #define JSON_DOC(n, ...)                JsonDocument n
        #define JSON_CREATE_OBJECT(h,name)      (h)[name].to<JsonObject>()
        #define JSON_CREATE_ARRAY(h,name)       (h)[name].to<JsonArray>()
        #define JSON_ADD_OBJECT(arr)            (arr).add<JsonObject>()
        #define JSON_ADD_ARRAY(arr)             (arr).add<JsonArray>()
    #endif

    #define JSON_ROOT(doc)                      LSC::jsonRoot(doc)
    #define JSON_IS_NULL(h)                     ((h).isNull())
    #define JSON_GET_OBJECT(h,name)             (h)[name].as<JsonObject>()
    #define JSON_GET_ARRAY(h,name)              (h)[name].as<JsonArray>()
    #define JSON_GET_OR_CREATE_OBJECT(h,name)   LSC::jsonGetOrCreateObject(h,name)
    #define JSON_KEY_EXISTS(h,key)              (!(h)[key].isNull())

    #define JSON_SET(h,key,value)               ((h)[key] = (value))
    #define JSON_ADD_VALUE(arr,value)           ((arr).add(value))

    #define JSON_GET_STRING(h,key,def)          ((h)[key] | (const char *)(def))
    #define JSON_GET_INT(h,key,def)             ((h)[key] | (int)(def))
    #define JSON_GET_ULONG(h,key,def)           ((h)[key] | (unsigned long)(def))
    #define JSON_GET_FLOAT(h,key,def)           ((h)[key] | (float)(def))
    #define JSON_GET_BOOL(h,key,def)            ((h)[key] | (bool)(def))

    #define JSON_PARSE(doc,text)                (deserializeJson((doc),(text)) == DeserializationError::Ok)
    #define JSON_SERIALIZE(doc,str)             ((str) = "", serializeJson((doc),(str)))
    #define JSON_SERIALIZE_PRETTY(doc,str)      ((str) = "", serializeJsonPretty((doc),(str)))

    #define JSON_PAYLOAD(doc,cmd,type,payload)  LSC::createPayloadStructure((cmd),(type),(doc),(payload))

#else
// ***************************************************************************
// JsonNode backend (default)
// ***************************************************************************
    #include <JsonNode.h>

    #define LSC_JSON_BACKEND_NAME               "JsonNode"

    #define JSON_DOCUMENT                       JsonNode
    #define JSON_OBJECT                         JsonNode *
    #define JSON_ARRAY                          JsonNode *

    // Size parameter (ArduinoJson v6) is accepted and ignored
    #define JSON_DOC(n, ...)                    JsonNode n

    #define JSON_ROOT(doc)                      (&(doc))
    #define JSON_IS_NULL(h)                     ((h) == nullptr)
    #define JSON_CREATE_OBJECT(h,name)          ((h)->createObject(name))
    #define JSON_CREATE_ARRAY(h,name)           ((h)->createArray(name))
    #define JSON_ADD_OBJECT(arr)                ((arr)->createObject())
    #define JSON_ADD_ARRAY(arr)                 ((arr)->createArray())
    #define JSON_GET_OBJECT(h,name)             ((h)->getObject(name))
    #define JSON_GET_ARRAY(h,name)              ((h)->getArray(name))
    #define JSON_GET_OR_CREATE_OBJECT(h,name)   ((h)->getObject(name,true))
    #define JSON_KEY_EXISTS(h,key)              ((h)->find(key,false) != nullptr)

    #define JSON_SET(h,key,value)               LSC::jsonSet((h),(key),(value))
    #define JSON_ADD_VALUE(arr,value)           LSC::jsonSetValue((arr)->createElement(),(value))

    #define JSON_GET_STRING(h,key,def)          ((h)->getValue((key),(def)))
    #define JSON_GET_INT(h,key,def)             ((h)->getValueAsInt((key),(def)))
    #define JSON_GET_ULONG(h,key,def)           ((h)->getValueAsUnsignedLong((key),(def)))
    #define JSON_GET_FLOAT(h,key,def)           ((h)->getValueAsFloat((key),(def)))
    #define JSON_GET_BOOL(h,key,def)            ((h)->getValueAsBool((key),(def)))

    #define JSON_PARSE(doc,text)                ((doc).parse(text) != nullptr)
    #define JSON_SERIALIZE(doc,str)             ((str) = (doc).getAsJsonText())
    #define JSON_SERIALIZE_PRETTY(doc,str)      ((str) = (doc).getAsJsonTextPretty())

    #define JSON_PAYLOAD(doc,cmd,type,payload)  LSC::createPayloadStructure((cmd),(type),(doc),(payload))

#endif

// ***************************************************************************
// Legacy names (previous ArduinoJson based macros) - mapped to the new API.
// Please use the JSON_xxx macros in new code.
// ***************************************************************************
#define JSON_DOC_STATIC(n,s)                JSON_DOC(n,s)
#define GetJsonDocumentAsObject(oDoc)       JSON_ROOT(oDoc)
#define CreateJsonObject(h,name)            JSON_CREATE_OBJECT(h,name)
#define CreateJsonArray(h,name)             JSON_CREATE_ARRAY(h,name)
#define CreateEmptyJsonObject(arr)          JSON_ADD_OBJECT(arr)
#define CreateEmptyJsonArray(arr)           JSON_ADD_ARRAY(arr)
#define GetJsonObject(h,name)               JSON_GET_OBJECT(h,name)
#define GetOrCreateJsonObject(h,name)       JSON_GET_OR_CREATE_OBJECT(h,name)
#define JsonKeyExists(h,key,type)           JSON_KEY_EXISTS(h,key)

namespace LSC {

    #ifndef LSC_JSON_USE_ARDUINOJSON
        // Value setters for JsonNode - explicit overloads, so temporaries (String)
        // and double values do not end up in an unexpected operator= overload.
        template<typename T>
        inline JsonNode * jsonSetValue(JsonNode *pNode, T value)        { return(pNode ? (*pNode = value) : nullptr); }
        inline JsonNode * jsonSetValue(JsonNode *pNode, double value)   { return(pNode ? pNode->setValue((float) value) : nullptr); }
        inline JsonNode * jsonSetValue(JsonNode *pNode, const String &value) { return(pNode ? pNode->setValue(value.c_str()) : nullptr); }
        inline JsonNode * jsonSetValue(JsonNode *pNode, char *value)    { return(pNode ? pNode->setValue((const char *) value) : nullptr); }

        template<typename T>
        inline JsonNode * jsonSet(JsonNode *pNode, const char *pszKey, T value) {
            return(pNode && pszKey ? jsonSetValue(&(*pNode)[pszKey], value) : nullptr);
        }
    #endif

    #ifdef LSC_JSON_USE_ARDUINOJSON
        /// @brief Default/explicit document size for ArduinoJson v6 documents.
        inline size_t jsonDocSize(size_t nDefault, size_t nSize = 0) {
            (void) nDefault;
            return(nSize > 0 ? nSize : 2048);
        }
        /// @brief Root object of a document, converts an empty document into an object.
        inline JsonObject jsonRoot(JsonDocument &oDoc) {
            return(oDoc.is<JsonObject>() ? oDoc.as<JsonObject>() : oDoc.to<JsonObject>());
        }
        /// @brief Get a sub object, create it if it does not exist (or has the wrong type).
        inline JsonObject jsonGetOrCreateObject(JsonObject oNode, const char *pszName) {
            JsonObject oResult = oNode[pszName].as<JsonObject>();
            return(oResult.isNull() ? JSON_CREATE_OBJECT(oNode,pszName) : oResult);
        }
    #endif

    /**
     * @brief Create the standard {"command":..,"data":..,"payload":..} structure.
     * The document is replaced. pszPayload is parsed as JSON; if it is no valid
     * JSON it is stored as string value.
     * @return Handle to the payload object (empty object if pszPayload is nullptr).
     */
    JSON_OBJECT createPayloadStructure(const char* pszCommand, const char *pszDataType, JSON_DOCUMENT &oPayloadDoc, const char *pszPayload = nullptr);
    #ifndef NATIVE_RUNTIME
        JSON_OBJECT createPayloadStructure(const __FlashStringHelper* pszCommand, const __FlashStringHelper* pszDataType, JSON_DOCUMENT &oPayloadDoc, const char *pszPayload = nullptr);
    #endif

    /// @brief Store a string value unless it equals the hidden-value mask (i.e. "*****" for passwords).
    bool setJsonValueIfNot(JSON_OBJECT oSource, const char* pszKey, String & strTarget, const char *pszHiddenValue);

    /// @brief Store the value of pszKey into the target, if the key exists and is convertible.
    /// @return true if the target was updated.
    bool setJsonValue(JSON_OBJECT oSource, const char* pszKey, String        & strTarget);
    bool setJsonValue(JSON_OBJECT oSource, const char* pszKey, float         * pTarget);
    bool setJsonValue(JSON_OBJECT oSource, const char* pszKey, int           * pTarget);
    bool setJsonValue(JSON_OBJECT oSource, const char* pszKey, unsigned long * pTarget);
    bool setJsonValue(JSON_OBJECT oSource, const char* pszKey, bool          * pTarget);
    bool setJsonValue(JSON_OBJECT oSource, const char* pszKey, IPAddress     & oTarget);

    #ifndef NATIVE_RUNTIME
        bool setJsonValue(JSON_OBJECT oSource, const __FlashStringHelper* pszKey, String    & strTarget);
        bool setJsonValue(JSON_OBJECT oSource, const __FlashStringHelper* pszKey, float     * pTarget);
        bool setJsonValue(JSON_OBJECT oSource, const __FlashStringHelper* pszKey, int       * pTarget);
        bool setJsonValue(JSON_OBJECT oSource, const __FlashStringHelper* pszKey, bool      * pTarget);
        bool setJsonValue(JSON_OBJECT oSource, const __FlashStringHelper* pszKey, IPAddress & oTarget);
    #endif
}

/// @brief Store the value of a key into a target (String& or pointer to int/float/bool/unsigned long, IPAddress&).
#define JSON_STORE_IF(h,key,target)         LSC::setJsonValue((h),(key),(target))
/// @brief Like JSON_STORE_IF, but ignores the value if it equals the hidden mask.
#define JSON_STORE_IF_NOT(h,key,str,hidden) LSC::setJsonValueIfNot((h),(key),(str),(hidden))
