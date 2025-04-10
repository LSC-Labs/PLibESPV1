#pragma once
#include <ConfigHandler.h>

class CVarTable;
/**
 * Represents a single Var entry
 */
class CVar  {
        friend class CVarTable;
        const char * pszName;
        const char * pszValue;
        bool isCritical = false;

    protected:
        void setVarName(const char * pszName);
   
    public:
        CVar();
        ~CVar();
        CVar(const char *strName) : pszName(strName), pszValue("") {}
        CVar(const char *strName, const char *strValue);
        CVar(const char *strName, const int nValue);
        CVar(const char *strName, const bool bValue);
        CVar(const __FlashStringHelper* pszName, const char * pszValue);
        CVar(const __FlashStringHelper* pszName, const __FlashStringHelper * pszValue);
        CVar(const __FlashStringHelper *strName, const int nValue);
        CVar(const __FlashStringHelper *strName, const bool bValue);

        
        CVar * setCriticalVar(bool bIsCritical);
        CVar * setValue(const char * strValue);
        CVar * setValue(const __FlashStringHelper * strValue);
        CVar * setValue(const int  nValue);
        CVar * setValue(const bool bValue); 
        CVar * setValue(const unsigned long ulValue);    
        
        bool isCriticalVar();
        const char *            getName();
        const char *            getValue();
        const int               getIntValue();
        const unsigned long     getUnsignedLongValue();
        const bool              getBoolValue();
};

/**
 * Represents a collection of vars in a table
 */
class CVarTable : public IConfigHandler {
    friend class CVar;
    struct CVarTableEntry {
        char *pszKeyName;
        CVar oVar;
        CVarTableEntry * pNextEntry;
        CVarTableEntry(const char *pszVarName, bool bCaseSensitive = false);
        ~CVarTableEntry();
    };
    
    CVarTableEntry *pVarEntries;
    bool isCaseSensitive = false;

    protected:
        bool prepareKeyName(char * szKeyName);
        CVar * getOrCreateVarEntry(const char *strName);

    public:
        CVarTable(bool bCaseSensitive = false);
        ~CVarTable();
        bool hasCriticalVars();

        CVar * find(const char *);
        CVar * find(String); 
        CVar * find(const __FlashStringHelper*);

        CVar * set(String strName, String strValue);
        CVar * set(String strName, const int nValue);
        CVar * set(String strName, const unsigned long ulValue);
        CVar * set(String strName, bool   bValue);
        CVar * set(const char * pszName, String strValue);

        CVar * set(const char * pszName, const char * pszValue);        
        CVar * set(const char * pszName, const int    nValue);
        CVar * set(const char * pszName, const unsigned long ulValue);
        CVar * set(const char * pszName, bool   bValue);
        CVar * set(const __FlashStringHelper*, String strValue );
        CVar * set(const __FlashStringHelper*, const int nValue);
        CVar * set(const __FlashStringHelper*, const unsigned long ulValue );
        CVar * set(const __FlashStringHelper*, bool   bValue );

        
        const char * getValue(    String &strName, const char * pszDefault);
        const char * getValue(    String &strName, String &strDefault);
        int          getIntValue( String &strName, int nDefault);
        bool         getBoolValue(String &strName, bool bDefault);

        const char * getValue(    const char *pszName, const char * pszDefault);
        int          getIntValue( const char *pszName, int nDefault);
        bool         getBoolValue(const char *pszName, bool bDefault);

        const char * getValue(const __FlashStringHelper *pszName, const char * pszDefault);
        const char * getValue(const __FlashStringHelper *pszName, const __FlashStringHelper * pszDefault);
        int          getIntValue( const __FlashStringHelper *pszName, int nDefault);
        bool         getBoolValue(const __FlashStringHelper *pszName, bool bDefault);


        // Implement the IConfigHandler interface
        virtual void writeConfigTo(JsonObject &oCfgObj, bool bHideCritical) override;
        virtual void readConfigFrom(JsonObject &oCfgObj) override;
};



