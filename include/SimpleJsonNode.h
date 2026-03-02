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
#include <Runtime.h>


class CSimpleJsonNode {
    /**
     * @brief Element entry...
     */
    class CElementEntry {
        public:
            String Name;
            String Value;
            CElementEntry(String strName, String strValue) {
                Name = strName;
                Value = strValue;
            }
    };

    // Name of this node...
    String Name;

    // List of Object subnodes...
    std::vector<CSimpleJsonNode*> tSubNodes;

    // List of Elemententry (key=value)
    std::vector<CElementEntry*> tKeyValues;

protected:
    

public:
    CSimpleJsonNode() {}
    CSimpleJsonNode(const char* pszName) {
        Name = pszName;
    };

    virtual ~CSimpleJsonNode() {
        clear();
    }

    virtual void clear();
    virtual void dump();
    
    const char      * getValue(const char* pszName, const char *pszDefault = "");
    CElementEntry   * getElement(const char * pszName);
    CSimpleJsonNode * getObject(const char *pszName);
    CSimpleJsonNode * createOrGetObject(const char *pszName);

    virtual bool exists(const char* pszName);
    bool hasObject(const char * pszName);
    bool hasElement(const char * pszName);
    virtual const char* parse(const char* pszJsonData);

};


