#pragma once
#include <ConfigHandler.h>
#include <vector>
#include <cstddef>

// class CVarTable;
/// @brief Represents one named configuration variable.
class CVar  {
        // friend class CVarTable;
        char * pszKeyName = nullptr;    // Key name (Real name, not case sensitive)
        char * pszName = nullptr;       // Real name, set by user
        char * pszValue = nullptr;      // Value
        bool isCritical = false;        // is a critical value, like a password

    protected:
        
   
    public:
        /// @brief Release owned name/key/value buffers.
        ~CVar();
        /// @brief Create an empty variable.
        CVar();
        CVar(const CVar&) = delete;
        CVar& operator=(const CVar&) = delete;
        /// @brief Create a variable with a name and empty value.
        CVar(const char *pszName);
        /// @brief Create a string variable.
        CVar(const char *pszName, const char *pszValue);
        CVar(const char* name, std::nullptr_t) : CVar(name, static_cast<const char*>(nullptr)) {}
        template<typename T> CVar(std::nullptr_t, T value) : CVar(static_cast<const char*>(nullptr), value) {}
        /// @brief Create an integer variable.
        CVar(const char *pszName, const int   nValue);
        /// @brief Create a boolean variable.
        CVar(const char *pszName, const bool  bValue);

        #ifndef NATIVE_RUNTIME
            /// @brief Create a string variable from flash-string name.
            CVar(const __FlashStringHelper* pszName, const char * pszValue);
            CVar(const char* pszName, const __FlashStringHelper* pszValue);
            CVar(const __FlashStringHelper* name, std::nullptr_t) : CVar(name, static_cast<const char*>(nullptr)) {}
            /// @brief Create a string variable from flash-string name/value.
            CVar(const __FlashStringHelper* pszName, const __FlashStringHelper * pszValue);
            /// @brief Create an integer variable from flash-string name.
            CVar(const __FlashStringHelper *strName, const int nValue);
            /// @brief Create a boolean variable from flash-string name.
            CVar(const __FlashStringHelper *strName, const bool bValue);
            /// @brief Set the value from a flash string.
            CVar * setValue(const __FlashStringHelper * strValue);
        #endif

        /// @brief Set or replace the variable name and derived lookup key.
        void setVarName(const char * pszName);
        /// @brief Return the lookup key; original case can be requested.
        const char * getKeyName(bool bCaseSensitive = false);
        
        /// @brief Mark whether this variable contains critical/secret data.
        CVar * setCriticalVar(bool bIsCritical);
        /// @brief Set the variable value from text.
        CVar * setValue(const char * strValue);
        CVar* setValue(std::nullptr_t) { return setValue(static_cast<const char*>(nullptr)); }
        /// @brief Set the variable value from an integer.
        CVar * setValue(const int  nValue);
        /// @brief Set the variable value from a boolean.
        CVar * setValue(const bool bValue); 
        /// @brief Set the variable value from an unsigned long.
        CVar * setValue(const unsigned long ulValue);    
        
        /// @brief Return true if this variable should be masked in public config.
        bool isCriticalVar();
        /// @brief Return the original variable name.
        const char *            getName();
        /// @brief Return the value as text.
        const char *            getValue();
        /// @brief Return the value converted to int.
        const int               getIntValue();
        /// @brief Return the value converted to unsigned long.
        const unsigned long     getUnsignedLongValue();
        /// @brief Return the value converted to bool.
        const bool              getBoolValue();
    };
    
    /// @brief Collection of named variables exposed through IConfigHandler.
    class CVarTable : public IConfigHandler {
            friend class CVar;
            std::vector<CVar *> tVarEntries;
            bool isCaseSensitive = false;
            #ifndef NATIVE_RUNTIME
                String flashDefaultCache;
            #endif
            
        protected:
            bool prepareKeyName(char * szKeyName);
            CVar * getOrCreateVarEntry(const char *strName);
           
        public:
            /// @brief Create a variable table with optional case-sensitive lookup.
            CVarTable(bool bCaseSensitive = false);
            /// @brief Delete all owned variables.
            virtual ~CVarTable();
            // Owns its entries; shallow copies would double-delete them.
            CVarTable(const CVarTable&) = delete;
            CVarTable& operator=(const CVarTable&) = delete;
            /// @brief Return true when at least one variable is marked critical.
            bool hasCriticalVars();
            
            /// @brief Find a variable by name.
            CVar * find(const char *);
            CVar* find(std::nullptr_t) { return nullptr; }
            /// @brief Find a variable by String name.
            CVar * find(String); 
            #ifndef NATIVE_RUNTIME
                /// @brief Find a variable by flash-string name.
                CVar * find(const __FlashStringHelper*);
            #endif
            
            /// @brief Set or create a String variable.
            CVar * set(String strName, String strValue);
            CVar * set(String strName, const char* pszValue);
            CVar* set(String name, std::nullptr_t) { return set(name.c_str(), static_cast<const char*>(nullptr)); }
            CVar* set(const char* name, std::nullptr_t) { return set(name, static_cast<const char*>(nullptr)); }
            /// @brief A null name is normalized to the empty name.
            template<typename T> CVar* set(std::nullptr_t, T value) {
                return set(static_cast<const char*>(nullptr), value);
            }
            /// @brief Set or create an integer variable.
            CVar * set(String strName, const int nValue);
            /// @brief Set or create an unsigned long variable.
            CVar * set(String strName, const unsigned long ulValue);
            /// @brief Set or create a boolean variable.
            CVar * set(String strName, bool   bValue);
            /// @brief Set or create a variable from C-string name and String value.
            CVar * set(const char * pszName, String strValue);
            
            /// @brief Set or create a string variable.
            CVar * set(const char * pszName, const char * pszValue);        
            /// @brief Set or create an integer variable.
            CVar * set(const char * pszName, const int    nValue);
            /// @brief Set or create an unsigned long variable.
            CVar * set(const char * pszName, const unsigned long ulValue);
            /// @brief Set or create a boolean variable.
            CVar * set(const char * pszName, bool   bValue);
            
            
            /// @brief Return a String-named value or the default.
            const char * getValue(    String &strName, const char * pszDefault);
            /// @brief Return a String-named value or the default String value.
            const char * getValue(    String &strName, String &strDefault);
            /// @brief Return a String-named value converted to int.
            int          getIntValue( String &strName, int nDefault);
            /// @brief Return a String-named value converted to bool.
            bool         getBoolValue(String &strName, bool bDefault);
            
            /// @brief Return a value by C-string name or the default.
            const char * getValue(    const char *pszName, const char * pszDefault);
            /// @brief Return a C-string named value converted to int.
            int          getIntValue( const char *pszName, int nDefault);
            /// @brief Return a C-string named value converted to bool.
            bool         getBoolValue(const char *pszName, bool bDefault);
            
            #ifndef NATIVE_RUNTIME
                CVar * set(const __FlashStringHelper*, String strValue );
                CVar * set(const __FlashStringHelper*, const char* pszValue);
                CVar* set(const __FlashStringHelper* name, std::nullptr_t) { return set(name, static_cast<const char*>(nullptr)); }
                CVar * set(const __FlashStringHelper*, const __FlashStringHelper* pszValue);
                CVar * set(const char* pszName, const __FlashStringHelper* pszValue);
                CVar * set(String strName, const __FlashStringHelper* pszValue);
                CVar * set(const __FlashStringHelper*, const int nValue);
                CVar * set(const __FlashStringHelper*, const unsigned long ulValue );
                CVar * set(const __FlashStringHelper*, bool   bValue );
                const char * getValue(const __FlashStringHelper *pszName, const char * pszDefault);
                const char* getValue(const __FlashStringHelper* name, std::nullptr_t) {
                    return getValue(name, static_cast<const char*>(nullptr));
                }
                /// @brief Flash fallback is copied to RAM; valid until the next flash fallback call.
                const char * getValue(const __FlashStringHelper *pszName, const __FlashStringHelper * pszDefault);
                int          getIntValue( const __FlashStringHelper *pszName, int nDefault);
                bool         getBoolValue(const __FlashStringHelper *pszName, bool bDefault);
            #endif


            // Implement the IConfigHandler interface
            /// @brief Write table values into an object; names are literal keys.
            /// @details __CN is reserved for critical-name metadata.
            virtual void writeConfigTo( JsonNode &oCfgObj, bool bHideCritical) override;
            /// @brief Read table values from a JSON config node.
            virtual void readConfigFrom(JsonNode &oCfgObj) override;
            /// @brief Return true when the table contains at least one variable.
            virtual bool hasConfigValues() override;
};


