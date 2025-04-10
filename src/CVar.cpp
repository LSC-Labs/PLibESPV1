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
}
CVar::CVar() {
    pszName = strdup("");
    pszValue = strdup("");
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
#pragma endregion

#pragma region  internal helpers

void CVar::setVarName(const char *pszName) {
    const char *pszVarName = pszName ? pszName : "";
    if(this->pszName != nullptr) {
        if(strcmp(this->pszName, pszVarName) == 0) return;
        free((void *) this->pszName);
    }
    this->pszName = strdup(pszVarName);
}

#pragma endregion

#pragma region set Values

CVar * CVar::setCriticalVar(bool bIsCritical) {
    this->isCritical = bIsCritical;
    return(this);
}

CVar * CVar::setValue(const char *pszValue){
    const char *pszVarValue = pszValue ? pszValue : "";
    if(this->pszValue != nullptr) {
        if(strcmp(this->pszValue, pszVarValue) == 0) return(this);
        free((void *) this->pszValue);
    }
    this->pszValue = strdup(pszVarValue);
    return(this);
}
CVar * CVar::setValue(const __FlashStringHelper *pszValue){
    const char *pszVarValue = (const char *) pszValue ? (const char *) pszValue : "";
    setValue((const char *) pszVarValue);
    return(this);
}

CVar * CVar::setValue(const int nValue) {
    char szValue[80];
    sprintf(szValue, "%d", nValue);
    setValue(szValue);
    return(this);
}
CVar * CVar::setValue(const unsigned long ulValue) {
    char szValue[80];
    sprintf(szValue, "%lu", ulValue);
    setValue(szValue);
    return(this);
}

CVar * CVar::setValue(const bool bValue) {
    setValue(bValue ? "1" : "0");
    return(this);
}

#pragma endregion

#pragma region getters

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
