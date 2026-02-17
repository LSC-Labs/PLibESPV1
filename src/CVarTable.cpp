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
#include <JsonHelper.h>


#ifndef LSC_VARS_CRITICAL_ENTRY_MASK
    #define LSC_VARS_CRITICAL_ENTRY_MASK "******"
#endif
#ifndef LSC_VARS_CRITICAL_NAMES_KEY
    #define LSC_VARS_CRITICAL_NAMES_KEY "__CN"
#endif

#pragma region CVarTableEntry implementation

/**
 * @brief create a new VarTable Entry object...
 *        The member pszKeyName, contains an empty string, or 
 *        the name of the variable in lowercase, if isCaseSensitive is false,
 *        or the name of the variable as is, if isCaseSensitive = true
 * @param pszVarName name of the var
 * @param isCaseSensitive is the context case - sensitive ?
 *
 *  */
/*
 CVarTable::CVarTableEntry::CVarTableEntry(CVar * pVar, bool isCaseSensitive) {
    this->pVar = pVar;
    this->pszKeyName = strdup(pVar->getName());
    if(!isCaseSensitive) strlwr(this->pszKeyName);
    // this->pNextEntry = nullptr;
}
*/

/**
 * @brief free the allocated memory for the key name and call the destructor of the var object
 * @details This destructor is called when the CVarTableEntry object is deleted.
 */
/*
 CVarTable::CVarTableEntry::~CVarTableEntry() {
    if(pszKeyName)  free(this->pszKeyName);
    if(pVar)        delete(pVar);
}
    */
#pragma endregion

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
    CVar *pResult = nullptr;
    if(pszName) {
        char tKeyName[strlen(pszName) + 2];
        strcpy(tKeyName,pszName);
        if(!isCaseSensitive) strlwr(tKeyName);
        for(CVar * pVar : this->tVarEntries) {
            if(pVar) {
                if(strcmp(pVar->getKeyName(isCaseSensitive),tKeyName) == 0) {
                    pResult = pVar;
                    break;
                }
            }
        }
    }
    return(pResult);
}

CVar * CVarTable::find(const __FlashStringHelper* pszName) {
    return(find((const char*) pszName));
}

CVar * CVarTable::find(String strName) {
    return(find(strName.c_str()));
}

/**
 * @brief returns the var entry if it exists, otherwise it creates a new one.
 * @param pszName Name of the var.
 * @return pointer to the var or nullptr if it could not be created.
 *  */
CVar * CVarTable::getOrCreateVarEntry(const char *pszName) {
    CVar * pVar = find(pszName);
    // Insert a new entriy if it does not exist
    if(pVar == nullptr) {
        pVar = new CVar(pszName);
        tVarEntries.push_back(pVar);
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


const char * CVarTable::getValue(const __FlashStringHelper *pszName, const char *pszDefault) {
    CVar *pVar = find(pszName);
    return(pVar ? pVar->getValue() : pszDefault );
}

const char * CVarTable::getValue(const __FlashStringHelper *pszName, const __FlashStringHelper *pszDefault) {
    CVar *pVar = find(pszName);
    return(pVar ? pVar->getValue() : (const char *)pszDefault );
}

int CVarTable::getIntValue(const __FlashStringHelper *pszName, int nDefault){
    CVar *pVar = find(pszName);
    return(pVar ? pVar->getIntValue() : nDefault );
}

bool CVarTable::getBoolValue(const __FlashStringHelper * pszName, bool bDefault) {
    CVar *pVar = find(pszName);
    return(pVar ? pVar->getBoolValue() : bDefault );
}

#pragma endregion

#pragma region setVar overlays


CVar * CVarTable::set(const char * pszName, String strValue) {
    CVar *pVar = getOrCreateVarEntry(pszName);
    pVar->setValue(strValue.c_str());
    return(pVar);
}

CVar * CVarTable::set(const char * pszName, const char *pszValue) {
    CVar *pVar = getOrCreateVarEntry(pszName);
    if(pVar) pVar->setValue(pszValue);
    return(pVar);
}
CVar * CVarTable::set(const char * pszName, const int nValue) {
    CVar *pVar = getOrCreateVarEntry(pszName);
    pVar->setValue(nValue);
    return(pVar);
}
CVar * CVarTable::set(const char * pszName, const unsigned long ulValue) {
    CVar *pVar = getOrCreateVarEntry(pszName);
    pVar->setValue(ulValue);
    return(pVar);
}

CVar * CVarTable::set(const char * pszName, bool bValue) {
    DEBUG_FUNC_START();
    CVar *pVar = getOrCreateVarEntry(pszName);
    pVar->setValue(bValue);
    DEBUG_FUNC_END();
    return(pVar);
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

CVar * CVarTable::set(const __FlashStringHelper* strName, String strValue) {
    return(set((const char*) strName,strValue));
}

CVar * CVarTable::set(const __FlashStringHelper* strName, const int nValue) {
    return(set((const char*) strName,nValue));
}
CVar * CVarTable::set(const __FlashStringHelper* strName, const unsigned long ulValue) {
    return(set((const char*) strName,ulValue));
}

CVar * CVarTable::set(const __FlashStringHelper* strName, bool bValue) {
    return(set((const char* ) strName,bValue));
}

#pragma endregion

#pragma region IConfigHandler Interface

/**
 * @brief write the config to a JSON object
 * @param oCfgObj JSON object to write the config to
 * @param bHideCritical true - hide critical vars values
 * Critical var names will be written in the JSON object with name defined in LSC_VARS_CRITICAL_NAMES_KEY.
 */
void CVarTable::writeConfigTo(JsonObject &oCfgObj, bool bHideCritical) {
    DEBUG_FUNC_START();
    for(CVar * pVar : this->tVarEntries) {
        bool showValue = true;
        if(pVar) {
            if(bHideCritical  && pVar->isCriticalVar()) showValue = false;

            DEBUG_INFOS("Writing var '%s' value='%s' (critical=%d) (showValue=%d)",
                        pVar->getName(),
                        showValue ? pVar->getValue() : LSC_VARS_CRITICAL_ENTRY_MASK,
                        pVar->isCriticalVar() ? 1 : 0,
                        showValue ? 1 : 0);

            oCfgObj[pVar->getName()] =  showValue ? 
                                        pVar->getValue() :
                                        LSC_VARS_CRITICAL_ENTRY_MASK;
            }
    }
    if(hasCriticalVars()) {
        JsonObject oCriticalNames = CreateJsonObject(oCfgObj,LSC_VARS_CRITICAL_NAMES_KEY);
        int nCount = 0;
        char tCountBuffer[80];
        for(CVar * pVar : tVarEntries) {
            if(pVar) {
                if(pVar->isCriticalVar()) {
                    nCount++;
                    sprintf(tCountBuffer,"n.%d",nCount);
                    oCriticalNames[tCountBuffer] = pVar->getName();
                }
            }
        }
        oCriticalNames["n.0"] = nCount;
    }
    DEBUG_FUNC_END();
}

/**
 * @brief read the config from a JSON object
 * @param oCfgObj JSON object to read the config from
 */
void CVarTable::readConfigFrom(JsonObject &oCfgObj) {
    DEBUG_FUNC_START();
    for(JsonPair oElement : oCfgObj) {
        // If it is the special key name for critical name, skip it.
        if(oElement.key() != LSC_VARS_CRITICAL_NAMES_KEY) {
            String strValue = oElement.value().as<String>();;
            // Set only if it is NOT a CRITICAL VALUE that is HIDDEN, to avoid loosing the original data.
            // You have to use setVar() explicit for this value...
            if(!strValue.equals(LSC_VARS_CRITICAL_ENTRY_MASK)) {
                set(oElement.key().c_str(),strValue.c_str());
            }
        }
    }   
    // Now correct the critical vars setting...
    if(JsonKeyExists(oCfgObj,LSC_VARS_CRITICAL_NAMES_KEY,JsonObject)) {
        JsonObject oCriticalNames = GetJsonObject(oCfgObj,LSC_VARS_CRITICAL_NAMES_KEY);
        int nCount = oCriticalNames["n.0"];
        for(int i = 1; i <= nCount; i++) {
            char tCountBuffer[80];
            sprintf(tCountBuffer,"n.%d",i);
            const char *pszName = oCriticalNames[tCountBuffer];
            CVar *pVar = find(pszName);
            if(pVar != nullptr) {
                pVar->setCriticalVar(true);
            }
        }
    }
    DEBUG_FUNC_END();
}

/**
 * Does the table contains vars or is it empty ?
 * @returns true, if there are vars inside.
 */
bool CVarTable::hasConfigValues() {
    return(!this->tVarEntries.empty());
}

#pragma endregion