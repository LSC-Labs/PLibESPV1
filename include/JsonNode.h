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
    enum ELEMENT_TYPE {
        OBJECT,
        VALUE,
        ARRAY
    };

    // Name of this node...
    String Name;
    std::vector<CJsonNode*> Elements;

protected:
    bool            m_bWriteValueWithQuotes = true;
    ELEMENT_TYPE    m_nObjectType = ELEMENT_TYPE::OBJECT;

    // List of Object subnodes...

    String  m_strSerializationCache;
    String  m_strValue;

    const char*  parseValue(const char* pszJsonData, String& strValueData, bool& bHasQuotes);
    const char * serializeNode(String & strResultString, int nIdentDeep = 0);
    void         writeIdentPrefixString(String & strResultString, int nIdentDeep = 0);
    void         setNodeValueType(bool bWriteWithQuotes = true);
    bool         getNameFromJsonPath(const char *pszName, String & strName);
    
    
    public:
    bool isJsonObject();
    bool isJsonObject(const char *pszName);
    bool isJsonArray();
    bool isJsonArray(const char *pszName);
    bool isJsonValue();
    bool isJsonValue(const char *pszName);
    bool isNumberValue();
    bool isNumberValue(const char *pszName);
    bool isBooleanValue();
    bool isBooleanValue(const char *pszName);
    
    CJsonNode() {}
    CJsonNode(const char* pszName, const char* pszValue);
    CJsonNode(const char* pszName, ELEMENT_TYPE eType = ELEMENT_TYPE::OBJECT);
    
    virtual ~CJsonNode() {
        clear();
    }
    virtual void        clear();
    virtual bool        exists(const char* pszName);
    ELEMENT_TYPE        getType();
    CJsonNode*          find(const char* pszName);
    CJsonNode*          createJsonPathToElement(const char *pszElement);
    CJsonNode   &       operator[](const char *pszName);
    CJsonNode   &       operator[](String & strName);


    String & getValueAsString(String & strDefault);
    String & getValueAsString(const char *pszName, String & strDefault);

    const char* getValue();
    const char* getValue(const char *pszName,const char *pszDefault = nullptr);

    const char* getValueAsCharPointer(const char* pszDefault);
    const char* getValueAsCharPointer(const char* pszName, const char* pszDefault);

    int getValueAsInt(const char *pszname, int nDefault = -1);
    int getValueAsInt(int nDefault = -1);

    unsigned int getValueAsUnsignedInt(const char *pszname, unsigned int nDefault = 0);
    unsigned int getValueAsUnsignedInt(unsigned int nDefault = 0);

    float getValueAsFloat(const char *pszname, float fDefault = -999.0);
    float getValueAsFloat(float fDefault = -999.0);

    long getValueAsLong(const char *pszname, long lDefault = -999.0);
    long getValueAsLong(long lDefault = -999.0);

    bool getValueAsBool(const char *pszname, bool bDefault = false);
    bool getValueAsBool(bool bDefault = false);

    bool storeValueIf(String & strTarget);
    bool storeValueIfNot(String & strTarget, const char *pszIfNot);
    bool storeValueIf(const char *pszName, String & strTarget);
    bool storeValueIfNot(const char *pszName, String & strTarget, const char *pszIfNot);
    
    bool storeValueIf(int * pnTarget);
    bool storeValueIf(const char *pszName, int   * pnTarget);

    bool storeValueIf(unsigned int * pnTarget);
    bool storeValueIf(const char *pszName, unsigned int   * pnTarget);

    bool storeValueIf(long * plTarget);
    bool storeValueIf(const char *pszName, long  * plTarget);

    bool storeValueIf(bool * pbTarget);
    bool storeValueIf(const char *pszName, bool  * pbTarget);
    
    bool storeValueIf(float * fTarget);
    bool storeValueIf(const char *pszName, float * pfTarget);
    

    CJsonNode*  getObject(  const char* pszName, bool bCreateIfNotExist = false);
    CJsonNode*  getArray(   const char* pszName, bool bCreateIfNotExist = false);
    CJsonNode*  getElement( const char* pszName, bool bCreateIfNotExist = false);

    const char* parse(const char* pszJsonData);

    void       remove(const char* pszName);

    CJsonNode* setValue(const char  *   pszValue);
    CJsonNode* setValue(String      &   strValue);
    CJsonNode* setValue(bool            bValue);
    CJsonNode* setValue(int             nValue);
    CJsonNode* setValue(unsigned int    unValue);
    CJsonNode* setValue(float           fValue);
    CJsonNode* setValue(long            lValue);
    CJsonNode* setValue(unsigned long   ulValue);

    CJsonNode* operator=(const char *   pszValue) { return(setValue(pszValue)); }
    CJsonNode* operator=(String &       strValue) { return(setValue(strValue)); }
    CJsonNode* operator=(bool           bValue)   { return(setValue(bValue)); }
    CJsonNode* operator=(int            nValue)   { return(setValue(nValue)); }
    CJsonNode* operator=(unsigned int   unValue)  { return(setValue(unValue)); }
    CJsonNode* operator=(float          fValue)   { return(setValue(fValue)); }
    CJsonNode* operator=(long           lValue)   { return(setValue(lValue)); }
    CJsonNode* operator=(unsigned long  ulValue)  { return(setValue(ulValue)); }

    CJsonNode* setValue(const char* pszName, const char *   pszValue);
    CJsonNode* setValue(const char* pszName, String     &   strValue);
    CJsonNode* setValue(const char* pszName, bool           bValue);
    CJsonNode* setValue(const char* pszName, int            nValue);
    CJsonNode* setValue(const char* pszName, float          fValue);
    CJsonNode* setValue(const char* pszName, long           lValue);
    CJsonNode* setValue(const char* pszName, unsigned long  ulValue);

    virtual void dump(const char *pszPrefix = "");

    JsonNode * createPayloadStructure(const char* pszCommand, const char *pszDataType, const char *pszPayload = nullptr) {
        setValue("command",pszCommand);
        setValue("data",pszDataType);
        JsonNode *pPayload = getObject("payload",true);
        if(pszPayload) {
            pPayload->setValue(pszPayload);
        }
        return(pPayload);
    }
    const char* getAsJsonText();
    const char* getAsJsonTextPretty();
};


