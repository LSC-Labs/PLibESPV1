/**
 * @file CVarTable.cpp
 * @version 1.0
 * @date 2023-10-01
 * @copyright LSC-Labs
 * @author P. Liebl
 * @brief This file contains the implementation of the CVarTable class.
 * @details The CVarTable class is used to manage a list of variables (CVar) and provides
 *          methods to find, create, and manipulate these variables.
 * 
 * -D DEBUGINFOS                    - enable debug macros
 * -D DEBUG_LSC_VARS                - enable debug macros output for this module.
 * -D LSC_VARS_CRITICAL_ENTRY_MASK  - string to be used for critical vars (value)
 * -D LSC_VARS_CRITICAL_NAMES_KEY   - key name to be used to store critical names into a JSON object
 * 
 * @note 2025-03-31 : added support for critical vars
 *                    Support for JSON 7.x and JSON 6.x
 */
#ifndef DEBUG_LSC_VARS
    #undef DEBUGINFOS
#endif

#pragma GCC diagnostic ignored "-Wunknown-pragmas"

#include <DevelopmentHelper.h>
#include <Vars.h>
#include <memory>
#include <new>
// #include <JsonHelper.h>


#ifndef LSC_VARS_CRITICAL_ENTRY_MASK
    #define LSC_VARS_CRITICAL_ENTRY_MASK "******"
#endif
#ifndef LSC_VARS_CRITICAL_NAMES_KEY
    #define LSC_VARS_CRITICAL_NAMES_KEY "__CN"
#endif

#pragma region CVarTable Constructor / destructor and helper...
/** 
 * @brief constructor 
 * @param isCaseSensitive Set var names case sensitive == true  
 */
 CVarTable::CVarTable(bool isCaseSensitive) {
    this->isCaseSensitive = isCaseSensitive;
}

CVarTable::~CVarTable() {
    DEBUG_FUNC_START();
    for(CVar * pVar : this->tVarEntries) {
        delete(pVar);
    }
    DEBUG_FUNC_END();
}

/*
 * @brief get the used key name, depending on the case sensitivity settings
 * @param pszKeyName Key name to be prepared 
 * @return true - key can be used...
 */
 bool CVarTable::prepareKeyName(char * pszKeyName) {
    bool bResult = pszKeyName != nullptr;
    if(bResult) {
        if(!isCaseSensitive) strlwr(pszKeyName);
    }
    return(bResult);
}

/**
 * @brief checks if there are critical vars in the table
 * @return true - there are critical vars
 */
bool CVarTable::hasCriticalVars() {
    bool bResult = false;
    for(CVar * pVar : this->tVarEntries) {
        if(pVar && pVar->isCriticalVar()) {
            bResult = true;
            break;
        }
    }
    return(bResult);
}

#pragma endregion


#pragma region Find var by name... or create new var

/**
 * @brief find the var inside the table
 * @param pszName Name of the var to be searched
 * @return nullptr or the var object
 */
CVar * CVarTable::find(const char * pszName) {
    if (!pszName) return nullptr;
    // Compare directly: no fixed-size buffer and no allocation per lookup.
    for (CVar* pVar : tVarEntries) {
        if (!pVar) continue;
        const unsigned char* key = reinterpret_cast<const unsigned char*>(pVar->getName());
        const unsigned char* name = reinterpret_cast<const unsigned char*>(pszName);
        while (*key && *name) {
            unsigned char a = *key, b = *name;
            if (!isCaseSensitive) {
                if (a >= 'A' && a <= 'Z') a += 'a' - 'A';
                if (b >= 'A' && b <= 'Z') b += 'a' - 'A';
            }
            if (a != b) break;
            ++key; ++name;
        }
        if (!*key && !*name) return pVar;
    }
    return nullptr;
}

#ifndef NATIVE_RUNTIME
CVar * CVarTable::find(const __FlashStringHelper* pszName) {
    return pszName ? find(String(pszName)) : nullptr;
}
#endif

CVar * CVarTable::find(String strName) {
    return(find(strName.c_str()));
}

/**
 * @brief returns the var entry if it exists, otherwise it creates a new one.
 * @param pszName Name of the var.
 * @return pointer to the var or nullptr if it could not be created.
 *  */
CVar * CVarTable::getOrCreateVarEntry(const char *pszName) {
    if (!pszName) pszName = "";
    CVar * pVar = find(pszName);
    // Insert a new entriy if it does not exist
    if(pVar == nullptr) {
        std::unique_ptr<CVar> entry(new (std::nothrow) CVar(pszName));
        if (!entry || strcmp(entry->getName(), pszName) != 0) return nullptr;
        tVarEntries.push_back(entry.get());
        pVar = entry.release();
    }
    return(pVar);
}

#pragma endregion


#pragma region Get Var Values
/// @brief gets the value of the var
/// @param strName name of the var
/// @param pszDefault Default if the var could not be found.
/// @return the var value or the default, if the var was not found.
const char * CVarTable::getValue(String &strName, const char * pszDefault) {
    CVar *pVar = find(strName);
    return(pVar ? pVar->getValue() : pszDefault );
}
const char * CVarTable::getValue(String &strName, String &strDefault) {
    CVar *pVar = find(strName);
    return(pVar ? pVar->getValue() : strDefault.c_str() );
}

int CVarTable::getIntValue(String &strName, int nDefault){
    CVar *pVar = find(strName);
    return(pVar ? pVar->getIntValue() : nDefault );
}

bool CVarTable::getBoolValue(String &strName, bool bDefault) {
    CVar *pVar = find(strName);
    return(pVar ? pVar->getBoolValue() : bDefault );
}

const char * CVarTable::getValue(const char *pszName, const char *pszDefault) {
    CVar *pVar = find(pszName);
    return(pVar ? pVar->getValue() : pszDefault );
}

int CVarTable::getIntValue(const char *pszName, int nDefault){
    CVar *pVar = find(pszName);
    return(pVar ? pVar->getIntValue() : nDefault );
}

bool CVarTable::getBoolValue(const char * pszName, bool bDefault) {
    CVar *pVar = find(pszName);
    return(pVar ? pVar->getBoolValue() : bDefault );
}

#ifndef NATIVE_RUNTIME
const char * CVarTable::getValue(const __FlashStringHelper *pszName, const char *pszDefault) {
    CVar *pVar = find(pszName);
    return(pVar ? pVar->getValue() : pszDefault );
}

const char * CVarTable::getValue(const __FlashStringHelper *pszName, const __FlashStringHelper *pszDefault) {
    CVar *pVar = find(pszName);
    if (pVar) return pVar->getValue();
    if (!pszDefault) return nullptr;
    flashDefaultCache = String(pszDefault);
    return flashDefaultCache.c_str();
}

int CVarTable::getIntValue(const __FlashStringHelper *pszName, int nDefault){
    CVar *pVar = find(pszName);
    return(pVar ? pVar->getIntValue() : nDefault );
}

bool CVarTable::getBoolValue(const __FlashStringHelper * pszName, bool bDefault) {
    CVar *pVar = find(pszName);
    return(pVar ? pVar->getBoolValue() : bDefault );
}
#endif
#pragma endregion

#pragma region setVar overlays


CVar * CVarTable::set(const char * pszName, String strValue) {
    CVar *pVar = getOrCreateVarEntry(pszName);
    if (pVar) pVar->setValue(strValue.c_str());
    return(pVar);
}

CVar * CVarTable::set(const char * pszName, const char *pszValue) {
    CVar *pVar = getOrCreateVarEntry(pszName);
    if(pVar) pVar->setValue(pszValue);
    return(pVar);
}
CVar * CVarTable::set(const char * pszName, const int nValue) {
    CVar *pVar = getOrCreateVarEntry(pszName);
    if (pVar) pVar->setValue(nValue);
    return(pVar);
}
CVar * CVarTable::set(const char * pszName, const unsigned long ulValue) {
    CVar *pVar = getOrCreateVarEntry(pszName);
    if (pVar) pVar->setValue(ulValue);
    return(pVar);
}

CVar * CVarTable::set(const char * pszName, bool bValue) {
    DEBUG_FUNC_START();
    CVar *pVar = getOrCreateVarEntry(pszName);
    if (pVar) pVar->setValue(bValue);
    DEBUG_FUNC_END();
    return(pVar);
}

CVar * CVarTable::set(String strName, const char* pszValue) {
    return set(strName.c_str(), pszValue);
}

CVar * CVarTable::set(String strName, String strValue) {
    return(set(strName.c_str(),strValue));
}

CVar * CVarTable::set(String strName, const int nValue) {
    return(set(strName.c_str(),nValue));
}
CVar * CVarTable::set(String strName, const unsigned long ulValue) {
    return(set(strName.c_str(),ulValue));
}

CVar * CVarTable::set(String strName, bool bValue) {
    return(set(strName.c_str(),bValue));
}
#ifndef NATIVE_RUNTIME
CVar* CVarTable::set(const __FlashStringHelper* name, const char* value) {
    return set(String(name).c_str(), value);
}
CVar* CVarTable::set(const __FlashStringHelper* name, const __FlashStringHelper* value) {
    return set(String(name).c_str(), String(value));
}
CVar* CVarTable::set(const char* name, const __FlashStringHelper* value) {
    return set(name, String(value));
}
CVar* CVarTable::set(String name, const __FlashStringHelper* value) {
    return set(name.c_str(), String(value));
}

CVar * CVarTable::set(const __FlashStringHelper* strName, String strValue) {
    return set(String(strName).c_str(), strValue);
}

CVar * CVarTable::set(const __FlashStringHelper* strName, const int nValue) {
    return set(String(strName).c_str(), nValue);
}
CVar * CVarTable::set(const __FlashStringHelper* strName, const unsigned long ulValue) {
    return set(String(strName).c_str(), ulValue);
}

CVar * CVarTable::set(const __FlashStringHelper* strName, bool bValue) {
    return set(String(strName).c_str(), bValue);
}
#endif
#pragma endregion

#pragma region IConfigHandler Interface

/**
 * @brief write the config to a JSON object
 * @param oCfgObj JSON object to write the config to
 * @param bHideCritical true - hide critical vars values
 * Critical var names will be written in the JSON object with name defined in LSC_VARS_CRITICAL_NAMES_KEY.
 */
void CVarTable::writeConfigTo(JsonNode &oCfgObj, bool bHideCritical) {
    if (!oCfgObj.isJsonObject()) return;
    // Remove old metadata before rebuilding it (also when no secrets remain).
    oCfgObj.remove(LSC_VARS_CRITICAL_NAMES_KEY);
    JsonNode* criticalNames = nullptr;
    int count = 0;
    for (CVar* var : tVarEntries) {
        if (!var || strcmp(var->getName(), LSC_VARS_CRITICAL_NAMES_KEY) == 0) continue;
        JsonNode* value = oCfgObj.find(var->getName(), false);
        if (!value) value = oCfgObj.createElement(var->getName());
        value->setValue(bHideCritical && var->isCriticalVar() ?
                        LSC_VARS_CRITICAL_ENTRY_MASK : var->getValue());
        if (var->isCriticalVar()) {
            if (!criticalNames) criticalNames = oCfgObj.createObject(LSC_VARS_CRITICAL_NAMES_KEY);
            char key[32];
            snprintf(key, sizeof(key), "n.%d", ++count);
            criticalNames->setValue(key, var->getName());
        }
    }
    if (criticalNames) criticalNames->setValue("n.0", count);
}

/** @brief Import scalar values and bounded critical-name metadata. */
void CVarTable::readConfigFrom(JsonNode &oCfgObj) {
    if (!oCfgObj.isJsonObject()) return;
    for (JsonNode* element : oCfgObj.Elements) {
        if (!element || !element->isJsonValue() || element->Name == LSC_VARS_CRITICAL_NAMES_KEY) continue;
        const char* value = element->getValue();
        if (value && strcmp(value, LSC_VARS_CRITICAL_ENTRY_MASK) != 0) {
            set(element->Name.c_str(), value);
        }
    }
    JsonNode* names = oCfgObj.getObject(LSC_VARS_CRITICAL_NAMES_KEY);
    JsonNode* entries = names ? names->getObject("n") : nullptr;
    if (!entries) return;
    // Never trust n.0 as a loop bound: a malicious count could stall the device.
    for (JsonNode* entry : entries->Elements) {
        if (!entry || !entry->isJsonValue() || entry->Name == "0" || entry->Name.length() == 0) continue;
        bool numeric = true;
        for (size_t i = 0; i < entry->Name.length(); ++i) {
            if (entry->Name[i] < '0' || entry->Name[i] > '9') { numeric = false; break; }
        }
        if (!numeric) continue;
        CVar* var = find(entry->getValue());
        if (var) var->setCriticalVar(true);
    }
}

/**
 * Does the table contains vars or is it empty ?
 * @returns true, if there are vars inside.
 */
bool CVarTable::hasConfigValues() {
    return(!this->tVarEntries.empty());
}

#pragma endregion