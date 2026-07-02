#pragma once
#include <vector>
#include <LSCUtils.h>
#include <DevelopmentHelper.h>

/**
 * @brief Single key/value entry used by CNamedValueTable.
 *
 * The entry owns a copy of the key string. The value is stored directly and is
 * intentionally public so callers can update it after a lookup without another
 * wrapper function.
 *
 * @tparam TValue Type of the stored value. The value type must be copyable.
 */
template<typename TValue>
class CNamedValueEntry {

    protected:
        /**
         * @brief Owned copy of the key string.
         *
         * The pointer is allocated with strdup() in the constructor and released
         * in the destructor. A nullptr key is allowed, but it will not match any
         * lookup key.
         */
        char * m_pszKey = nullptr;

    public:
        /**
         * @brief Creates a new entry and copies the key.
         * @param pszKey Key name for this entry. May be nullptr.
         * @param value Initial value to store in the entry.
         */
        CNamedValueEntry(const char *pszKey, TValue value) {
            this->m_pszKey = pszKey ? strdup(pszKey) : nullptr;
            this->value = value;
        }

        /**
         * @brief Releases the owned key string.
         */
        ~CNamedValueEntry() {
            if(m_pszKey != nullptr) {
                free((void*)m_pszKey);
                m_pszKey = nullptr;
            }
        }

        /**
         * @brief Gets the stored key.
         * @return Pointer to the owned key string, or nullptr if the entry was
         *         created without a key.
         */
        const char * getKey() {
            return(m_pszKey);
        }

        /**
         * @brief Checks if a lookup key matches this entry key.
         * @param pszKey Lookup key to compare with this entry key.
         * @param bCaseSensitive true for strcmp(), false for LSC::stricmp().
         * @return true when both keys exist and match according to the selected
         *         comparison mode.
         */
        bool doesKeyMatch(const char *pszKey, bool bCaseSensitive) {
            bool bResult = false;
            if(pszKey && m_pszKey) {
                bResult = bCaseSensitive ? strcmp(pszKey,m_pszKey) == 0 : LSC::stricmp(pszKey,m_pszKey) == 0;
            }
            return(bResult);
        }

        /**
         * @brief Stored value.
         *
         * This is public by design: find functions return entries so callers can
         * read or update the value directly.
         */
        TValue value;
};

/**
 * @brief Lightweight ordered table for named values.
 *
 * The table stores entries in insertion order and offers lookup by key, lookup
 * by value, and optional uniqueness rules. It is intentionally simpler than
 * std::map: duplicate keys and duplicate values are allowed by default, while
 * derived classes can enable uniqueness constraints.
 *
 * Key comparison can be case-sensitive or case-insensitive. When no matching
 * key exists, get() returns the configured default value.
 *
 * @tparam TValue Type of the stored values. The value type must be copyable and
 *         comparable with operator== for value lookups and unique-value tables.
 */
template<typename TValue>
class CNamedValueTable {
   
    protected:
        /**
         * @brief Value returned by get() when no matching key exists.
         */
        TValue  m_defaultValue = 0;

        /**
         * @brief Controls key comparison mode.
         *
         * true means "wifi" and "WiFi" are different keys. false means they are
         * treated as the same key.
         */
        bool    m_bCaseSensitive = true;

        /**
         * @brief When true, a value may exist only once in the table.
         *
         * Used by CMultiNameUniqueValueTable. Duplicate values are rejected by
         * set() unless the existing entry is updated through an allowed key rule.
         */
        bool    m_bOnlyUniqueValues = false;

        /**
         * @brief When true, a key may exist only once in the table.
         *
         * Used by CUniqueNameMultiValueTable. Reusing an existing key updates
         * the existing entry instead of adding another one.
         */
        bool    m_bOnlyUniqueKeys = false;

        /**
         * @brief Cached list of key pointers in insertion order.
         *
         * The pointers refer to keys owned by the entries in Entries. They stay
         * valid for the lifetime of the table because entries are not removed.
         */
        std::vector<const char *> m_tKeyNameCache;
        
        
    public:
        /**
         * @brief Public entry list for direct iteration.
         *
         * The table owns all pointers in this vector and deletes them in the
         * destructor. Callers may iterate over the entries, but should not delete
         * pointers or insert foreign pointers into this vector.
         */
        std::vector<CNamedValueEntry<TValue> * > Entries;

        /**
         * @brief Creates an empty named-value table.
         * @param bCaseSensitiveKey true for case-sensitive key lookup, false for
         *        case-insensitive key lookup.
         * @param defaultValue Value returned by get() when no matching key is
         *        found.
         */
        CNamedValueTable(bool bCaseSensitiveKey = true, TValue defaultValue = 0) 
            : m_defaultValue(defaultValue), m_bCaseSensitive(bCaseSensitiveKey) {
        }

        /**
         * @brief Deletes all owned entries.
         */
        ~CNamedValueTable() {
            // Free the allocated keys
            for(auto * kvp : Entries) {
                delete(kvp);
            }
        }

#pragma region search and find functions.

        /**
         * @brief Checks if the table contains at least one entry with a value.
         * @param oValue Value to search for.
         * @return true if findFirstByValue() finds a matching entry.
         */
        bool hasValueEntry(TValue oValue) {
            return(findFirstByValue(oValue) != nullptr);
        }

        /**
         * @brief Finds the first entry with the given value.
         * @param oValue Value to search for.
         * @return First matching entry, or nullptr when the value is not found.
         */
        CNamedValueEntry<TValue> * findFirstByValue(TValue oValue) {
            CNamedValueEntry<TValue> * pResult = findNextByValue(oValue,nullptr);
            return(pResult);
        }

        /**
         * @brief Finds the next entry with the given value.
         *
         * Pass nullptr as pLastFound to search from the beginning. Pass a
         * previously returned entry to continue the search after that entry.
         *
         * @param oValue Value to search for.
         * @param pLastFound Previous match, or nullptr to start at the beginning.
         * @return Next matching entry, or nullptr when no further match exists.
         */
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
         * @brief Checks if the table contains at least one entry with a key.
         * @param pszKey Key to search for.
         * @return true if findFirstByKey() finds a matching entry.
         */
        bool hasKeyEntry(const char *pszKey) {
            return(findFirstByKey(pszKey) != nullptr);
        }

        /**
         * @brief Finds the first entry with the given key.
         * @param pszKey Key to search for. Matching respects the configured case
         *        sensitivity.
         * @return First matching entry, or nullptr when the key is not found.
         */
        CNamedValueEntry<TValue> * findFirstByKey(const char *pszKey) {
            DEBUG_FUNC_START_PARMS("%s",pszKey);
            CNamedValueEntry<TValue> * pResult = findNextByKey(pszKey,nullptr);
            DEBUG_FUNC_END_PARMS("%p",pResult);
            return(pResult);
        }

        /**
         * @brief Finds the next entry with the given key.
         *
         * Pass nullptr as pLastFound to search from the beginning. Pass a
         * previously returned entry to continue the search after that entry.
         *
         * @param pszKey Key to search for. Matching respects the configured case
         *        sensitivity.
         * @param pLastFound Previous match, or nullptr to start at the beginning.
         * @return Next matching entry, or nullptr when no further match exists.
         */
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

        /**
         * @brief Verifies that an entry pointer belongs to this table.
         * @param pNamedEntry Entry pointer to check.
         * @return The same pointer when it is stored in Entries, otherwise
         *         nullptr.
         */
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
         * @brief Sets or inserts a named value.
         *
         * Base table behavior:
         * - Duplicate keys are allowed.
         * - Duplicate values are allowed.
         * - A new entry is appended on every successful set().
         *
         * Derived table behavior:
         * - CUniqueNameMultiValueTable updates an existing key instead of adding
         *   another entry with the same key.
         * - CMultiNameUniqueValueTable rejects a new entry when the value already
         *   exists.
         *
         * @param pszKey Key for the entry. May be nullptr, but nullptr keys cannot
         *        be found by key lookup.
         * @param value Value to store.
         * @return Address of the inserted or updated entry, or nullptr when a
         *         uniqueness rule rejects the operation.
         */
        CNamedValueEntry<TValue> *  set(const char *pszKey, const TValue value) {
            CNamedValueEntry<TValue> * pEntry = nullptr;
            DEBUG_FUNC_START_PARMS("%s,%p",pszKey,value);
            bool bHasKeyEntry   = hasKeyEntry(pszKey);
            bool bHasValueEntry = hasValueEntry(value);

            bool doInsert = true;
            if(m_bOnlyUniqueKeys && bHasKeyEntry) {
                doInsert = true;
            } else {
                if(m_bOnlyUniqueKeys)   doInsert = !bHasKeyEntry;
                if(m_bOnlyUniqueValues) doInsert = !bHasValueEntry;
            }

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
         * @brief Gets the number of entries in the table.
         * @return Current number of stored entries.
         */
        int size() {
            return(Entries.size());
        }

        /**
         * @brief Gets all known keys in insertion order.
         *
         * The returned vector is a copy of the internal key cache. The contained
         * pointers are owned by the table entries and must not be freed by the
         * caller.
         *
         * @return Vector of key pointers.
         */
        std::vector<const char *> getKeys() {
            return(m_tKeyNameCache);
        }

        /**
         * @brief Gets the value for a key.
         *
         * If duplicate keys exist, the value of the first matching entry is
         * returned. Key matching respects the configured case sensitivity.
         *
         * @param pszKey Key to search for.
         * @return Stored value of the first matching entry, or the configured
         *         default value when no entry matches.
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
 * @brief Named-value table that allows many names but only unique values.
 *
 * A value may appear only once in the table. Multiple keys are allowed as long
 * as every key references a different value. set() returns nullptr when a new
 * entry would duplicate an existing value.
 * 
 * This is useful for registries where the same object pointer must not be
 * registered twice, even if callers use different names.
 *
 * @tparam TValue Type of the stored values.
 */
template<typename TValue>
class CMultiNameUniqueValueTable : public CNamedValueTable<TValue> {

    public:
        /**
         * @brief Creates a table with unique values and non-unique keys.
         * @param bCaseSensitiveKey true for case-sensitive key lookup, false for
         *        case-insensitive key lookup.
         * @param oDefaultValue Value returned by get() when no matching key is
         *        found.
         */
        CMultiNameUniqueValueTable(bool bCaseSensitiveKey = true, TValue oDefaultValue = 0) : CNamedValueTable<TValue>(bCaseSensitiveKey, oDefaultValue){
            this->m_bOnlyUniqueKeys = false;
            this->m_bOnlyUniqueValues = true;
        }
};

/**
 * @brief Named-value table that allows unique names and repeated values.
 *
 * A key may appear only once in the table. Setting an existing key updates the
 * stored value. The same value may be stored under multiple different keys.
 * 
 * This is useful for maps where one name must resolve to one current value, but
 * several names may intentionally share that value.
 *
 * @tparam TValue Type of the stored values.
 */
template<typename TValue>
class CUniqueNameMultiValueTable : public CNamedValueTable<TValue> {

    public:
        /**
         * @brief Creates a table with unique keys and non-unique values.
         * @param bCaseSensitiveKey true for case-sensitive key lookup, false for
         *        case-insensitive key lookup.
         * @param oDefaultValue Value returned by get() when no matching key is
         *        found.
         */
        CUniqueNameMultiValueTable(bool bCaseSensitiveKey = true, TValue oDefaultValue = 0) : CNamedValueTable<TValue>(bCaseSensitiveKey, oDefaultValue){
            this->m_bOnlyUniqueKeys = true;
            this->m_bOnlyUniqueValues = false;
        }
};
