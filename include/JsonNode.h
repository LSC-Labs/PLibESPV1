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

// Using the Runtime to enable native debugging and testing
#include "Runtime.h"
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
    ELEMENT_TYPE    m_nObjectType = ELEMENT_TYPE::OBJECT;
    bool            m_bWriteValueWithQuotes = true;

    // List of Object subnodes...

    String  m_strDataCache;

    const   char* parseValue(const char* pszJsonData, String& strValueData, bool& bHasQuotes);

    bool    isWhite(const char c);
    const   char* skipWhite(const char* psz);
    String  m_strValue;
    




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


    String & getValueAsString();
    String & getValueAsString(const char *pszName, String & strDefault);

    const char* getValue();
    const char* getValue(const char *pszName);
    const char* getValueOrDefault(const char* pszDefault);
    const char* getValueOrDefault(const char* pszName, const char* pszDefault);


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

    CJsonNode* setValue(const char* pszValue);
    CJsonNode* setValue(bool        bValue);
    CJsonNode* setValue(int         nValue);
    CJsonNode* setValue(float       fValue);
    CJsonNode* setValue(long        lValue);

    CJsonNode* setValue(const char* pszName, const char *pszValue);
    CJsonNode* setValue(const char* pszName, bool        bValue);
    CJsonNode* setValue(const char* pszName, int         nValue);
    CJsonNode* setValue(const char* pszName, float       fValue);
    CJsonNode* setValue(const char* pszName, long        lValue);

    virtual void dump(const char *pszPrefix = "");

    const char* getAsJsonText();

};


