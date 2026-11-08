#ifndef DEBUG_LSC_VARS
    #undef DEBUGINFOS
#endif
#pragma GCC diagnostic ignored "-Wunknown-pragmas"

#include <Vars.h>
#include <LSCUtils.h>
#include <DevelopmentHelper.h>
#include <climits>

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
CVar::CVar(const char* pszName, const __FlashStringHelper* pszValue) {
    setVarName(pszName);
    setValue(pszValue);
}

CVar::CVar(const __FlashStringHelper *pszName, const char *pszValue) {
    setVarName(String(pszName).c_str());
    setValue(pszValue);
}

/**
 * @brief Creates a variable from flash-string name and flash-string value.
 */
CVar::CVar(const __FlashStringHelper *pszName, const __FlashStringHelper *pszValue) {
    setVarName(String(pszName).c_str());
    setValue(pszValue);
}

/**
 * @brief Creates a variable from flash-string name and integer value.
 */
CVar::CVar(const __FlashStringHelper *pszName, const int nValue) {
    setVarName(String(pszName).c_str());
    setValue(nValue);
}

/**
 * @brief Creates a variable from flash-string name and boolean value.
 */
CVar::CVar(const __FlashStringHelper *pszName, const bool bValue) {
    setVarName(String(pszName).c_str());
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
    // Allocate before releasing the old buffers: input may alias either buffer.
    char* name = strdup(pszName ? pszName : "");
    char* key = strdup(pszName ? pszName : "");
    if (!name || !key) { free(name); free(key); return; }
    for (char* p = key; *p; ++p) {
        if (*p >= 'A' && *p <= 'Z') *p += 'a' - 'A';
    }
    free(this->pszName);
    free(pszKeyName);
    this->pszName = name;
    pszKeyName = key;
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
    char* value = strdup(pszValue ? pszValue : "");
    if (!value) return this;
    free(this->pszValue);
    this->pszValue = value;
    return(this);
}

#ifndef NATIVE_RUNTIME
/**
 * @brief Stores a flash-string value.
 * @return this for chained calls.
 */
CVar * CVar::setValue(const __FlashStringHelper *pszValue){
    String value(pszValue);
    setValue(value.c_str());
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
    return(pszName ? pszName : "");
}

/**
 * @brief Gets the stored value as text.
 * @return Value pointer.
 */
const char * CVar::getValue() {
    return(pszValue ? pszValue : "");
}

/**
 * @brief Converts the stored value to int.
 * @return Parsed value, clamped to the int range.
 */
const int CVar::getIntValue() {
    long value = strtol(getValue(), nullptr, 10);
    if (value > INT_MAX) return INT_MAX;
    if (value < INT_MIN) return INT_MIN;
    return static_cast<int>(value);
}

/**
 * @brief Converts the stored value to bool.
 * @return Result of LSC::isTrueValue() in non-explicit mode.
 */
const bool CVar::getBoolValue() {
    return(*getValue() && LSC::isTrueValue(getValue(),false));
}

/**
 * @brief Converts the stored value to unsigned long.
 * @return strtoul() result using base 10.
 */
const unsigned long CVar::getUnsignedLongValue() {
    return(strtoul(getValue(), nullptr, 10));
}

#pragma endregion
