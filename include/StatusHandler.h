#pragma once
#include "Runtime.h"
#include "JsonNode.h"
#include <vector>
#include <Registration.h>

class CStatusHandler;

#define STATUS_LEVEL_VERBOSE   9        // With all known diag information
#define STATUS_LEVEL_INFO      5        // Info Level is default level (Frontend)
#define STATUS_LEVEL_STATE     3        // State of the device (to be published)
#define STATUS_LEVEL_MIN       0        // Minimized level - only prio 1 infos
#define STAUTS_LEVEL_MIN       STATUS_LEVEL_MIN   // old (misspelled) name - kept for compatibility
/**
 * Interface for a status handler module
 */
class IStatusHandler {
    friend class CStatusHandler;
    private:
        CRegistrations m_oStatusRegistrations;  // status handler lists this handler is registered in

    public:
        /// @brief Removes this handler automatically from all status handler lists.
        virtual ~IStatusHandler() { m_oStatusRegistrations.releaseAll(this); }

        virtual void writeStatusTo(JsonNode &oStatusNode, int nLevel = STATUS_LEVEL_INFO) = 0;
        /**
         * Implement if you don't want to insert your status section,
         * if there is no data in the requested level.
         * Default = true, create the section and ask me.
         * Checked by CStatusHandler::writeStatusTo() before the section is created.
         */
        virtual bool hasStatusValues(int nLevel = STATUS_LEVEL_INFO) { return true; }
};

/**
 * Status handler class.
 * Works as a status handler and can register further status handler modules.
 * - A name can be registered only once (first registration wins).
 * - A handler may be registered under several names.
 * - Handlers can be removed or destroyed while the status is written
 *   (removed handlers are skipped, no other handler is skipped).
 * - Re-entrant calls (handler registered in itself or in a cycle) are ignored
 *   instead of recursing endlessly.
 */
class CStatusHandler : public IStatusHandler, public IRegistry {
    struct StatusHandlerEntry {
        const char *     pszName  = nullptr;
        IStatusHandler * pHandler = nullptr;
    };
    std::vector<StatusHandlerEntry> m_tListOfStatusHandler;
    bool m_bInCall = false;     // guard against recursion (cyclic registration)

    /// @brief Remove all entries of a handler (frees the names).
    void removeHandlerEntries(const void *pHandler);
    /// @brief true if the entry (same name buffer and handler) is still registered.
    bool isEntryRegistered(const StatusHandlerEntry &oEntry) const;

    public:
        CStatusHandler() {}
        CStatusHandler(const CStatusHandler &) = delete;
        CStatusHandler & operator=(const CStatusHandler &) = delete;
        /// @brief Detaches all registered handlers and frees the names.
        ~CStatusHandler();

        /// @brief Remove a handler from this list (all names).
        void removeStatusHandler(IStatusHandler *pHandler);
        /// @brief IRegistry - a registered handler is being destroyed.
        void forgetRegisteredObject(const void *pObject) override { removeHandlerEntries(pObject); }

        /// @brief Register a status handler for the section pszName.
        /// @return true if registered (or already registered with this name), false if the
        ///         name is used by another handler or the handler is this list itself.
        bool addStatusHandler(String strName, IStatusHandler *pHandler);
        /// @copydoc addStatusHandler(String,IStatusHandler*)
        bool addStatusHandler(const char *pszName, IStatusHandler *pHandler);
        IStatusHandler * getStatusHandler(String strName);        // Get a Status Handler by his name
        IStatusHandler * getStatusHandler(const char * pszName);  // Get a Status Handler by his name

        /** Interface implementation */
        void writeStatusTo(JsonNode &oStatusNode, int nLevel = STATUS_LEVEL_INFO) override;
};
