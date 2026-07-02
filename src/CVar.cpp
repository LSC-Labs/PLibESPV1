#ifndef DEBUG_LSC_VARS
    #undef DEBUGINFOS
#endif
#pragma GCC diagnostic ignored "-Wunknown-pragmas"

#include <Vars.h>
#include <LSCUtils.h>
#include <DevelopmentHelper.h>

#pragma region constructors

/**
 * @brief Releases all owned strings.
 */
CVar::~CVar() {
    if(pszName)  free((void *) pszName); 
    if(pszValue) free((void *) pszValue); 
    if(pszKeyName) free((void *) pszKeyName); 
}

/**
 * @brief Creates an empty variable.
 */
CVar::CVar() {
    setVarName("");
}


/**
 * @brief Creates a variable with a name and empty value.
 * @param pszName Variable name.
 */
CVar::CVar(const char *pszName) {
    setVarName(pszName);
}
 
/**
 * @brief Creates a variable with a string value.
 * @param pszName Variable name.
 * @param pszValue Initial string value.
 */
CVar::CVar(const char *pszName, const char *pszValue) {
    setVarName(pszName);
    setValue(pszValue);
}

/**
 * @brief Creates a variable with an integer value.
 * @param pszName Variable name.
 * @param nValue Initial integer value.
 */
CVar::CVar(const char *pszName, const int nValue) {
    setVarName(pszName);
    setValue(nValue);
}

/**
 * @brief Creates a variable with a boolean value.
 * @param pszName Variable name.
 * @param bValue Initial boolean value.
 */
CVar::CVar(const char *pszName, const bool bValue) {
    setVarName(pszName);
    setValue(bValue);
}

#ifndef NATIVE_RUNTIME

/**
 * @brief Creates a variable from flash-string name and RAM value.
 */
CVar::CVar(const __FlashStringHelper *pszName, const char *pszValue) {
    setVarName((const char *) pszName);
    setValue(pszValue);
}

/**
 * @brief Creates a variable from flash-string name and flash-string value.
 */
CVar::CVar(const __FlashStringHelper *pszName, const __FlashStringHelper *pszValue) {
    setVarName((const char *) pszName);
    setValue(pszValue);
}

/**
 * @brief Creates a variable from flash-string name and integer value.
 */
CVar::CVar(const __FlashStringHelper *pszName, const int nValue) {
    setVarName((const char *) pszName);
    setValue(nValue);
}

/**
 * @brief Creates a variable from flash-string name and boolean value.
 */
CVar::CVar(const __FlashStringHelper *pszName, const bool bValue) {
    setVarName((const char *)pszName);
    setValue(bValue);
}
#endif


#pragma endregion

#pragma region  internal helpers

/**
 * @brief Sets or changes the variable name.
 *
 * The original name is stored as pszName. pszKeyName stores the lowercase form
 * and is used for case-insensitive table lookups.
 *
 * @param pszName New variable name. nullptr becomes an empty string.
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

/**
 * @brief Marks this variable as critical/sensitive.
 * @param bIsCritical true when the value should be masked in exported config.
 * @return this for chained calls.
 */
CVar * CVar::setCriticalVar(bool bIsCritical) {
    this->isCritical = bIsCritical;
    return(this);
}

/**
 * @brief Stores a string value.
 * @param pszValue New value. nullptr becomes an empty string.
 * @return this for chained calls.
 */
CVar * CVar::setValue(const char *pszValue){
    if(this->pszValue)  free((void *) this->pszValue);
    this->pszValue = strdup(pszValue ? pszValue : "");
    return(this);
}

#ifndef NATIVE_RUNTIME
/**
 * @brief Stores a flash-string value.
 * @return this for chained calls.
 */
CVar * CVar::setValue(const __FlashStringHelper *pszValue){
    const char *pszVarValue = (const char *) pszValue ? (const char *) pszValue : "";
    setValue((const char *) pszVarValue);
    return(this);
}
#endif

/**
 * @brief Stores an integer value as text.
 * @return this for chained calls.
 */
CVar * CVar::setValue(const int nValue) {
    char szValue[80];
    snprintf(szValue,sizeof(szValue), "%d", nValue);
    setValue(szValue);
    return(this);
}

/**
 * @brief Stores an unsigned long value as text.
 * @return this for chained calls.
 */
CVar * CVar::setValue(const unsigned long ulValue) {
    char szValue[80];
    snprintf(szValue,sizeof(szValue), "%lu", ulValue);
    setValue(szValue);
    return(this);
}

/**
 * @brief Stores a boolean value as "1" or "0".
 * @return this for chained calls.
 */
CVar * CVar::setValue(const bool bValue) {
    setValue(bValue ? "1" : "0");
    return(this);
}

#pragma endregion

#pragma region getters

/**
 * @brief Gets the lookup key for this variable.
 *
 * The returned key is either the original name or the lowercase key, depending
 * on the requested comparison mode.
 *
 * @param bCaseSensitive true to return the original name.
 * @return Lookup key, never nullptr.
 */
const char * CVar::getKeyName(bool bCaseSensitive) {
    if(bCaseSensitive) {
        return(pszName ? pszName : "");
    } else {
        return(pszKeyName ? pszKeyName : "");
    }
}

/**
 * @brief Checks whether this variable is marked critical.
 * @return true when config export should mask the value.
 */
bool CVar::isCriticalVar() {
    return(isCritical);
}   

/**
 * @brief Gets the original variable name.
 * @return Name pointer.
 */
const char * CVar::getName() {
    return(pszName);
}

/**
 * @brief Gets the stored value as text.
 * @return Value pointer.
 */
const char * CVar::getValue() {
    return(pszValue);
}

/**
 * @brief Converts the stored value to int.
 * @return atoi() result.
 */
const int CVar::getIntValue() {
    return(atoi(pszValue));
}

/**
 * @brief Converts the stored value to bool.
 * @return Result of LSC::isTrueValue() in non-explicit mode.
 */
const bool CVar::getBoolValue() {
    return(LSC::isTrueValue(pszValue,false));
}

/**
 * @brief Converts the stored value to unsigned long.
 * @return strtoul() result using base 10.
 */
const unsigned long CVar::getUnsignedLongValue() {
    return(strtoul(pszValue, nullptr, 10));
}

#pragma endregion
