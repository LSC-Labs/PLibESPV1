#pragma once
#include <vector>
#include <LSCUtils.h>
#include <DevelopmentHelper.h>

template<typename TValue>
class CNamedValueEntry {

    protected:
        char * m_pszKey = nullptr;

    public:
        CNamedValueEntry(const char *pszKey, TValue value) {
            this->m_pszKey = strdup(pszKey);
            this->value = value;
        }

        ~CNamedValueEntry() {
            if(m_pszKey != nullptr) {
                free((void*)m_pszKey);
                m_pszKey = nullptr;
            }
        }

        /**
         * Get the key of this entry
         */
        const char * getKey() {
            return(m_pszKey);
        }

        /**
         * Ask if the given key matches the key of this object
         */
        bool doesKeyMatch(const char *pszKey, bool bCaseSensitive) {
            bool bResult = false;
            if(pszKey && m_pszKey) {
                bResult = bCaseSensitive ? strcmp(pszKey,m_pszKey) == 0 : LSC::stricmp(pszKey,m_pszKey) == 0;
            }
            return(bResult);
        }

        // Stored value as Property         
        TValue value;
};


template<typename TValue>
class CNamedValueTable {
   
    protected:
        TValue  m_defaultValue = 0;
        bool    m_bCaseSensitive = true;       // Default is case sensitive is off, so "wifi" and "WiFi" is not the same.
        bool    m_bOnlyUniqueValues = false;   // Ensures, ta value is only once in the table
        bool    m_bOnlyUniqueKeys = false;     // Ensures, a key is only once in the table

        // To ease iteration for keynames, the cache will be used.
        std::vector<const char *> m_tKeyNameCache;
        
        
    public:
        /**
         * List of entries to enable iteration.
         * - do not modify elements via this vector.
         */
        std::vector<CNamedValueEntry<TValue> * > Entries;

        CNamedValueTable(bool bCaseSensitiveKey = true, TValue defaultValue = 0) 
            : m_defaultValue(defaultValue), m_bCaseSensitive(bCaseSensitiveKey) {
        }

        ~CNamedValueTable() {
            // Free the allocated keys
            for(auto * kvp : Entries) {
                delete(kvp);
            }
        }

#pragma region search and find functions.

        /**
         * @brief does this table already holds an entry with this value
         */
        bool hasValueEntry(TValue oValue) {
            return(findFirstByValue(oValue) != nullptr);
        }

        /**
         * Find the first key value pair for the given value. If not found, return nullptr.
         * @returns the first entry, otherwise a nullptr.
         */
        CNamedValueEntry<TValue> * findFirstByValue(TValue oValue) {
            CNamedValueEntry<TValue> * pResult = findNextByValue(oValue,nullptr);
            return(pResult);
        }

        CNamedValueEntry<TValue> * findNextByValue(TValue oValue, CNamedValueEntry<TValue> * pLastFound = nullptr) {
            CNamedValueEntry<TValue> * pResult = nullptr;
            // If pLastFound is not null, search the entry first...
            // otherwise begin searching immediately
            bool bSearch = pLastFound == nullptr;
            for(auto pEntry : Entries) {
                if(bSearch) {
                    if(pEntry->value == oValue) { pResult = pEntry; break; }
                } else {
                    // This is the entry, start searching now...
                    if(pEntry == pLastFound) bSearch = true;
                }
            }
            return(pResult);
        }

        /**
         * @brief Does this table already contains a key with this name ?
         */
        bool hasKeyEntry(const char *pszKey) {
            return(findFirstByKey(pszKey) != nullptr);
        }

        /**
         * Find the first key value pair for the given key. If not found, return nullptr.
         * @returns the first entry, otherwise a nullptr.
         */
        CNamedValueEntry<TValue> * findFirstByKey(const char *pszKey) {
            DEBUG_FUNC_START_PARMS("%s",pszKey);
            CNamedValueEntry<TValue> * pResult = findNextByKey(pszKey,nullptr);
            DEBUG_FUNC_END_PARMS("%p",pResult);
            return(pResult);
        }

        CNamedValueEntry<TValue> * findNextByKey(const char *pszKey, CNamedValueEntry<TValue> * pLastFound = nullptr) {
            DEBUG_FUNC_START_PARMS("\"%s\",%p",pszKey,pLastFound);
            CNamedValueEntry<TValue> * pResult = nullptr;
            // If pLastFound is not null, search the entry first...
            // otherwise begin searching immediately
            bool bSearch = pLastFound == nullptr;
            for(auto pEntry : Entries) {
                if(bSearch) {
                    if(pEntry->doesKeyMatch(pszKey,m_bCaseSensitive)) { pResult = pEntry; break; }
                } else {
                    // This is the entry, start searching now...
                    if(pEntry == pLastFound) bSearch = true;
                }
            }
            DEBUG_FUNC_END_PARMS("%p",pResult);
            return(pResult);
        }

        CNamedValueEntry<TValue> * findEntryByAddress(CNamedValueEntry<TValue> * pNamedEntry ) {
            CNamedValueEntry<TValue> * pResult = nullptr;
            if(pNamedEntry) {
                for(auto * pEntry : Entries) {
                    if(pEntry == pNamedEntry) { pResult = pEntry; break; }
                }
            }
            return(pResult);
        }

#pragma endregion


        /**
         * @brief Set the value for the given key. If the key does not exist, it will be created.
         * @returns nullptr, if nothing is set or the named value entry address.
         */
        CNamedValueEntry<TValue> *  set(const char *pszKey, const TValue value) {
            CNamedValueEntry<TValue> * pEntry = nullptr;
            DEBUG_FUNC_START_PARMS("%s,%p",pszKey,value);
            bool bHasKeyEntry   = hasKeyEntry(pszKey);
            bool bHasValueEntry = hasValueEntry(value);

            bool doInsert = true;
            if(m_bOnlyUniqueKeys)   doInsert = !bHasKeyEntry;
            if(m_bOnlyUniqueValues) doInsert = !bHasValueEntry;

            if(doInsert) {
                if(m_bOnlyUniqueKeys)   pEntry = findFirstByKey(pszKey);
                if(m_bOnlyUniqueValues) pEntry = findFirstByValue(value);
                if(pEntry) {
                    pEntry->value = value;
                } else {
                    pEntry = new CNamedValueEntry<TValue>(pszKey, value);
                    Entries.push_back(pEntry);
                    m_tKeyNameCache.push_back(pEntry->getKey());
                }
            }
            DEBUG_FUNC_END();
            return(pEntry);
        }

        /**
         * @brief Get the number of key-value pairs in the map.
         */
        int size() {
            return(Entries.size());
        }

        /**
         * @brief Get the keys of the map as a vector of const char*.
         */
        std::vector<const char *> getKeys() {
            return(m_tKeyNameCache);
        }

        /**
         * Get the value for the given key. If the key does not exist, return the default value.
         */
        TValue get(const char *pszKey) {
            TValue oResultValue = m_defaultValue;
            for(CNamedValueEntry<TValue> * pEntry : Entries) {
                if(pEntry->doesKeyMatch(pszKey,m_bCaseSensitive)) {
                    oResultValue = pEntry->value;
                    break;
                }
            }
            return(oResultValue);
        }
};

/**
 * @brief Table with unique values
 * - A value may only be one in the table.
 * - Multiple keys are allowed, as long as they reference to different values.
 * 
 * This table ensures, that a receiver (like on a message bus) is only registered once.
 */
template<typename TValue>
class CMultiNameUniqueValueTable : public CNamedValueTable<TValue> {

    public:
        CMultiNameUniqueValueTable(bool bCaseSensitiveKey = true, TValue oDefaultValue = 0) : CNamedValueTable<TValue>(bCaseSensitiveKey, oDefaultValue){
            this->m_bOnlyUniqueKeys = false;
            this->m_bOnlyUniqueValues = true;
        }
};

/**
 * @brief Table with unique names and multi values
 * - A single key is allowed (default = NOT case sensitive)
 * - A value can be stored, as long as the key is different.
 * 
 * This table ensures, that a key is unique,
 * A value can be addressed by different keys.
 */
template<typename TValue>
class CUniqueNameMultiValueTable : public CNamedValueTable<TValue> {

    public:
        CUniqueNameMultiValueTable(bool bCaseSensitiveKey = true, TValue oDefaultValue = 0) : CNamedValueTable<TValue>(bCaseSensitiveKey, oDefaultValue){
            this->m_bOnlyUniqueKeys = true;
            this->m_bOnlyUniqueValues = false;
        }
};

