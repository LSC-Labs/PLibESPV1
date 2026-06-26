#ifndef DEBUG_LSC_VARS
    #undef DEBUGINFOS
#endif
#pragma GCC diagnostic ignored "-Wunknown-pragmas"

#include <Vars.h>
#include <LSCUtils.h>
#include <DevelopmentHelper.h>

#pragma region constructors

CVar::~CVar() {
    if(pszName)  free((void *) pszName); 
    if(pszValue) free((void *) pszValue); 
    if(pszKeyName) free((void *) pszKeyName); 
}

CVar::CVar() {
    setVarName("");
}


CVar::CVar(const char *pszName) {
    setVarName(pszName);
}
 
CVar::CVar(const char *pszName, const char *pszValue) {
    setVarName(pszName);
    setValue(pszValue);
}

CVar::CVar(const char *pszName, const int nValue) {
    setVarName(pszName);
    setValue(nValue);
}

CVar::CVar(const char *pszName, const bool bValue) {
    setVarName(pszName);
    setValue(bValue);
}

#ifndef NATIVE_RUNTIME

CVar::CVar(const __FlashStringHelper *pszName, const char *pszValue) {
    setVarName((const char *) pszName);
    setValue(pszValue);
}
CVar::CVar(const __FlashStringHelper *pszName, const __FlashStringHelper *pszValue) {
    setVarName((const char *) pszName);
    setValue(pszValue);
}
CVar::CVar(const __FlashStringHelper *pszName, const int nValue) {
    setVarName((const char *) pszName);
    setValue(nValue);
}

CVar::CVar(const __FlashStringHelper *pszName, const bool bValue) {
    setVarName((const char *)pszName);
    setValue(bValue);
}
#endif


#pragma endregion

#pragma region  internal helpers

/**
 * @brief set / change the name of this var.
 * Releases the memory of the old name..
 */
void CVar::setVarName(const char *pszName) {
    // Has already a name ?
    if(this->pszName) free((void *) this->pszName);
    this->pszName = strdup(pszName ? pszName : "");

    // Set also the KeyName...
    if(pszKeyName) free(pszKeyName);
    pszKeyName = strdup(pszName ? pszName : "");
    strlwr(pszKeyName);
}

#pragma endregion

#pragma region set Values

CVar * CVar::setCriticalVar(bool bIsCritical) {
    this->isCritical = bIsCritical;
    return(this);
}

CVar * CVar::setValue(const char *pszValue){
    if(this->pszValue)  free((void *) this->pszValue);
    this->pszValue = strdup(pszValue ? pszValue : "");
    return(this);
}

#ifndef NATIVE_RUNTIME
CVar * CVar::setValue(const __FlashStringHelper *pszValue){
    const char *pszVarValue = (const char *) pszValue ? (const char *) pszValue : "";
    setValue((const char *) pszVarValue);
    return(this);
}
#endif

CVar * CVar::setValue(const int nValue) {
    char szValue[80];
    snprintf(szValue,sizeof(szValue), "%d", nValue);
    setValue(szValue);
    return(this);
}
CVar * CVar::setValue(const unsigned long ulValue) {
    char szValue[80];
    snprintf(szValue,sizeof(szValue), "%lu", ulValue);
    setValue(szValue);
    return(this);
}

CVar * CVar::setValue(const bool bValue) {
    setValue(bValue ? "1" : "0");
    return(this);
}

#pragma endregion

#pragma region getters

/**
 * Gets the key name, if set, otherwise an empty string.
 * The key name is either the name fo the var (case sensitive)
 * or the lowercase version of the name (caseinsensitive)
 */
const char * CVar::getKeyName(bool bCaseSensitive) {
    if(bCaseSensitive) {
        return(pszName ? pszName : "");
    } else {
        return(pszKeyName ? pszKeyName : "");
    }
}

bool CVar::isCriticalVar() {
    return(isCritical);
}   
const char * CVar::getName() {
    return(pszName);
}

const char * CVar::getValue() {
    return(pszValue);
}

const int CVar::getIntValue() {
    return(atoi(pszValue));
}

const bool CVar::getBoolValue() {
    return(LSC::isTrueValue(pszValue,false));
}
const unsigned long CVar::getUnsignedLongValue() {
    return(strtoul(pszValue, nullptr, 10));
}

#pragma endregion
