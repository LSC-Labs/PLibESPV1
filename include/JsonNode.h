#pragma once
/**
 * @brief CSimpleJsonNode Helper class to read Json Data.
 * Having problems reading json data with existing libraries,
 * this implementation comes in place with a small footprint for
 * smart devices like esp32.
 * @copyright LSC-Labs - use without warranty..
 *
 * 2026-02-28 : parse data and read values and objects...
 * 2026-03-02 : using Runtime.h to enable testing and debugging in the host system.
 */

 // If compiled with MS - supress warnings...
#define _CRT_SECURE_NO_WARNINGS
#define JsonNode CJsonNode

// Using the Runtime to enable native debugging and testing
#include "Runtime.h"
// #include "Network.h"
#include <vector>

#define SIMPLE_JSON_TYPE_VALUE  0
#define SIMPLE_JSON_TYPE_OBJECT 1
#define SIMPLE_JSON_TYPE_ARRAY  2

class CJsonNode {
public:
    /// @brief JSON node kind used by the lightweight tree implementation.
    enum ELEMENT_TYPE {
        /// @brief Node contains named child nodes.
        OBJECT,
        /// @brief Node contains one scalar value.
        VALUE,
        /// @brief Node contains ordered child nodes.
        ARRAY
    };

    /// @brief Optional name/key of this node inside its parent.
    String Name;
    /// @brief Owned child nodes for objects and arrays.
    std::vector<CJsonNode*> Elements;

protected:
    JsonNode      * m_pParentNode = nullptr;
    bool            m_bWriteValueWithQuotes = true;
    ELEMENT_TYPE    m_nObjectType = ELEMENT_TYPE::OBJECT;

    // List of Object subnodes...

    String  m_strSerializationCache;
    String  m_strValue;

    /// @brief Store the parent pointer used for tree navigation.
    void         setParentNode(JsonNode * pParentNode);
    /// @brief Parse one scalar JSON token from the input string.
    const char*  parseValue(const char* pszJsonData, String& strValueData, bool& bHasQuotes);
    /// @brief Recursively serialize this node into the provided string buffer.
    const char * serializeNode(String & strResultString, int nIdentDeep = 0);
    /// @brief Append indentation spaces used by pretty JSON serialization.
    void         writeIdentPrefixString(String & strResultString, int nIdentDeep = 0);
    /// @brief Convert this node into a scalar value and remember quote handling.
    void         setNodeValueType(bool bWriteWithQuotes = true);
    /// @brief Split a dotted JSON path and return the final element name.
    bool         getNameFromJsonPath(const char *pszName, String & strName);
    
    
public:

    /// @brief Create an unnamed object node.
    CJsonNode() {}
    /// @brief Create a named JSON value node initialized with text data.
    CJsonNode(const char* pszName, const char* pszValue);
    /// @brief Create a JSON node with an optional name and explicit node type.
    CJsonNode(const char* pszName, ELEMENT_TYPE eType = ELEMENT_TYPE::OBJECT);

    /// @brief Delete all child nodes on destruction.
    virtual ~CJsonNode() {
        clear();
    }
    /// @brief Delete all child nodes and reset this node to an empty container.
    virtual void        clear();
    /// @brief Return true if a direct or dotted-path child exists.
    virtual bool        exists(const char* pszName);
    /// @brief Return the stored node type.
    ELEMENT_TYPE        getType();
    /// @brief Return the parent node, or nullptr for a root node.
    JsonNode *          getParentNode();

    /// @brief Return true when this node is a JSON object.
    bool isJsonObject();
    /// @brief Return true when the named child exists and is an object.
    bool isJsonObject(const char *pszName);
    /// @brief Return true when this node is a JSON array.
    bool isJsonArray();
    /// @brief Return true when the named child exists and is an array.
    bool isJsonArray(const char *pszName);
    /// @brief Return true when this node is a scalar JSON value.
    bool isJsonValue();
    /// @brief Return true when the named child exists and is a scalar value.
    bool isJsonValue(const char *pszName);
    /// @brief Return true if this node contains a simple integer/float literal.
    bool isNumberValue();
    /// @brief Return true if a named child contains a numeric literal.
    bool isNumberValue(const char *pszName);
    /// @brief Return true if this node contains a recognized boolean literal.
    bool isBooleanValue();
    /// @brief Return true if a named child contains a recognized boolean literal.
    bool isBooleanValue(const char *pszName);
    /// @brief Find a child by name or dotted path.
    CJsonNode*          find(const char* pszName);
    /// @brief Ensure that all object nodes before the final path element exist.
    CJsonNode*          createJsonPathToElement(const char *pszElement);
    /// @brief Create or replace a child value node.
    CJsonNode*          createElement(const char *pszName = nullptr);
    /// @brief Create or replace a child object node.
    CJsonNode*          createObject(const char *pszName = nullptr);
    /// @brief Create or replace a child array node.
    CJsonNode*          createArray(const char *pszName = nullptr);
    /// @brief Return an existing value element or create it on demand.
    CJsonNode   &       operator[](const char *pszName);
    /// @brief String overload for value element access.
    CJsonNode   &       operator[](String & strName);


    /// @brief Return this node's value as String, or the supplied default.
    String & getValueAsString(String & strDefault);
    /// @brief Return a named child value as String, or the supplied default.
    String & getValueAsString(const char *pszName, String & strDefault);

    /// @brief Return this node's raw value text.
    const char* getValue();
    /// @brief Return a named child value or, for value nodes, this node's value.
    const char* getValue(const char *pszName,const char *pszDefault = nullptr);

    /// @brief Return this node's value as C string, or the supplied default.
    const char* getValueAsCharPointer(const char* pszDefault);
    /// @brief Return a named child value as C string, or the supplied default.
    const char* getValueAsCharPointer(const char* pszName, const char* pszDefault);

    /// @brief Parse a named child value as int.
    int getValueAsInt(const char *pszname, int nDefault = -1);
    /// @brief Parse this node's numeric value as int.
    int getValueAsInt(int nDefault = -1);

    /// @brief Parse a named child value as unsigned int.
    unsigned int getValueAsUnsignedInt(const char *pszname, unsigned int nDefault = 0);
    /// @brief Parse this node's numeric value as unsigned int.
    unsigned int getValueAsUnsignedInt(unsigned int nDefault = 0);

    /// @brief Parse a named child value as float.
    float getValueAsFloat(const char *pszname, float fDefault = -999.0);
    /// @brief Parse this node's numeric value as float.
    float getValueAsFloat(float fDefault = -999.0);

    /// @brief Parse a named child value as long.
    long getValueAsLong(const char *pszname, long lDefault = 0);
    /// @brief Parse this node's numeric value as long.
    long getValueAsLong(long lDefault = 0);

    /// @brief Parse a named child value as unsigned long.
    unsigned long getValueAsUnsignedLong(const char *pszname, unsigned long ulDefault = 0);
    /// @brief Parse this node's numeric value as unsigned long.
    unsigned long getValueAsUnsignedLong(unsigned long ulDefault = 0);

    /// @brief Parse a named child value as bool.
    bool getValueAsBool(const char *pszname, bool bDefault = false);
    /// @brief Parse this node's boolean value.
    bool getValueAsBool(bool bDefault = false);

    /// @brief Copy this node's text into the target string when present.
    bool storeValueIf(String & strTarget);
    /// @brief Copy this node's text unless it matches the excluded marker.
    bool storeValueIfNot(String & strTarget, const char *pszIfNot);
    /// @brief Copy a named child text value into the target string when present.
    bool storeValueIf(const char *pszName, String & strTarget);
    /// @brief Copy a named child text value unless it matches the excluded marker.
    bool storeValueIfNot(const char *pszName, String & strTarget, const char *pszIfNot);
    
    /// @brief Store this value in the target if it is numeric.
    bool storeValueIf(int * pnTarget);
    /// @brief Store a named child in the target if it is numeric.
    bool storeValueIf(const char *pszName, int   * pnTarget);

    /// @brief Store this value in the target if it is numeric.
    bool storeValueIf(unsigned int * pnTarget);
    /// @brief Store a named child in the target if it is numeric.
    bool storeValueIf(const char *pszName, unsigned int   * pnTarget);

    /// @brief Store this value in the target if it is numeric.
    bool storeValueIf(long * plTarget);
    /// @brief Store a named child in the target if it is numeric.
    bool storeValueIf(const char *pszName, long  * plTarget);

    /// @brief Store this value in the target if it is boolean.
    bool storeValueIf(bool * pbTarget);
    /// @brief Store a named child in the target if it is boolean.
    bool storeValueIf(const char *pszName, bool  * pbTarget);
    
    /// @brief Store this value in the target if it is numeric.
    bool storeValueIf(float * fTarget);
    /// @brief Store a named child in the target if it is numeric.
    bool storeValueIf(const char *pszName, float * pfTarget);
    

    /// @brief Return a named object node, optionally creating/converting it.
    CJsonNode*  getObject(  const char* pszName, bool bCreateIfNotExist = false);
    /// @brief Return a named array node, optionally creating/converting it.
    CJsonNode*  getArray(   const char* pszName, bool bCreateIfNotExist = false);
    /// @brief Return a named scalar value node, optionally creating/converting it.
    CJsonNode*  getElement( const char* pszName, bool bCreateIfNotExist = false);

    /// @brief Parse JSON text into this node tree.
    const char* parse(const char* pszJsonData);

    /// @brief Remove and delete the direct child with the given name.
    void       remove(const char* pszName);

    /// @brief Store a quoted string value in this node.
    CJsonNode* setValue(const char  *   pszValue);
    /// @brief Store a quoted String value in this node.
    CJsonNode* setValue(String      &   strValue);
    /// @brief Store an unquoted boolean value in this node.
    CJsonNode* setValue(bool            bValue);
    /// @brief Store an unquoted signed integer value in this node.
    CJsonNode* setValue(int             nValue);
    /// @brief Store an unquoted unsigned integer value in this node.
    CJsonNode* setValue(unsigned int    unValue);
    /// @brief Store an unquoted floating point value in this node.
    CJsonNode* setValue(float           fValue);
    /// @brief Store an unquoted signed long value in this node.
    CJsonNode* setValue(long            lValue);
    /// @brief Store an unquoted unsigned long value in this node.
    CJsonNode* setValue(unsigned long   ulValue);

    CJsonNode* operator=(const char *   pszValue) { return(setValue(pszValue)); }
    CJsonNode* operator=(String &       strValue) { return(setValue(strValue)); }
    CJsonNode* operator=(bool           bValue)   { return(setValue(bValue)); }
    CJsonNode* operator=(int            nValue)   { return(setValue(nValue)); }
    CJsonNode* operator=(unsigned int   unValue)  { return(setValue(unValue)); }
    CJsonNode* operator=(float          fValue)   { return(setValue(fValue)); }
    CJsonNode* operator=(long           lValue)   { return(setValue(lValue)); }
    CJsonNode* operator=(unsigned long  ulValue)  { return(setValue(ulValue)); }

    /// @brief Set or create a named string value.
    CJsonNode* setValue(const char* pszName, const char *   pszValue);
    /// @brief Set or create a named String value.
    CJsonNode* setValue(const char* pszName, String     &   strValue);
    /// @brief Set or create a named boolean value.
    CJsonNode* setValue(const char* pszName, bool           bValue);
    /// @brief Set or create a named signed integer value.
    CJsonNode* setValue(const char* pszName, int            nValue);
    /// @brief Set or create a named floating point value.
    CJsonNode* setValue(const char* pszName, float          fValue);
    /// @brief Set or create a named signed long value.
    CJsonNode* setValue(const char* pszName, long           lValue);
    /// @brief Set or create a named unsigned long value.
    CJsonNode* setValue(const char* pszName, unsigned long  ulValue);

    /// @brief Print this node tree to Serial for diagnostics.
    virtual void dump(const char *pszPrefix = "");

    /**
     * @brief Create the common command/data/payload message structure.
     * @return The payload object node so callers can fill it directly.
     */
    JsonNode * createPayloadStructure(const char* pszCommand, const char *pszDataType, const char *pszPayload = nullptr) {
        setValue("command",pszCommand);
        setValue("data",pszDataType);
        JsonNode *pPayload = getObject("payload",true);
        if(pszPayload) {
            pPayload->setValue(pszPayload);
        }
        return(pPayload);
    }
    /// @brief Serialize this node tree as compact JSON.
    const char* getAsJsonText();
    /// @brief Serialize this node tree as pretty-printed JSON.
    const char* getAsJsonTextPretty();
};

