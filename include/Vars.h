#pragma once
#include <ConfigHandler.h>
#include <vector>

// class CVarTable;
/**
 * Represents a single Var entry
 */
class CVar  {
        // friend class CVarTable;
        char * pszKeyName = nullptr;    // Key name (Real name, not case sensitive)
        char * pszName = nullptr;       // Real name, set by user
        char * pszValue = nullptr;      // Value
        bool isCritical = false;        // is a critical value, like a password

    protected:
        
   
    public:
        ~CVar();
        CVar();
        CVar(const char *pszName);
        CVar(const char *pszName, const char *pszValue);
        CVar(const char *pszName, const int   nValue);
        CVar(const char *pszName, const bool  bValue);
        CVar(const __FlashStringHelper* pszName, const char * pszValue);
        CVar(const __FlashStringHelper* pszName, const __FlashStringHelper * pszValue);
        CVar(const __FlashStringHelper *strName, const int nValue);
        CVar(const __FlashStringHelper *strName, const bool bValue);

        void setVarName(const char * pszName);
        const char * getKeyName(bool bCaseSensitive = false);
        
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

    /*
    class CVarTableEntry {
        public:
            char * pszKeyName;
            CVar * pVar;
            // CVarTableEntry * pNextEntry;
            CVarTableEntry(CVar * pVar, bool bCaseSensitive = false);
            ~CVarTableEntry();
    };
    */

    std::vector<CVar *> tVarEntries;
    
    // CVarTableEntry *pVarEntries;
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



