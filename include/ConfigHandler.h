#pragma once

#include <Runtime.h>
#include <JsonNode.h>
#include <vector>
#include <Registration.h>

class CConfigHandler;

/**
 * Interface for configuration handlers
 */
class IConfigHandler {
    friend class CConfigHandler;
    private:
        CRegistrations m_oConfigRegistrations;  // config handler lists this handler is registered in

    public:
        /// @brief Removes this handler automatically from all config handler lists.
        virtual ~IConfigHandler() { m_oConfigRegistrations.releaseAll(this); }

        virtual void writeConfigTo( JsonNode &oCfgNode, bool bHideCritical) = 0;
        virtual void readConfigFrom(JsonNode &oCfgNode) = 0;

        // Override, if you want the parent config handler not to create the subnode,
        // if no values are available to be stored. Default is - has values.
        virtual bool hasConfigValues() { return true; }

        // Override, if you want to migrate your configuration to a new version
        // oCfgDoc is the whole configuration file,
        // oCfgNode is the currently registered config sub node (maybe not available).
        // To migrate, pull infos from the configuration, and if no longer valid, delete obsolet informations.
        // This function is called, after Appl was reading the file from the filesystem
        // and before readConfigFrom() is called.
        // @param oCfgDoc The main configuration root node
        // @param oCfgNode If already available, the config section of this handler.
        virtual void migrateConfig(JsonNode &oCfgDoc, JsonNode &oCfgNode) {}
};


/**
 * Configuration handler manager.
 *
 * Holds named child handlers; each one owns the JSON sub object with its name.
 * - A name can be registered only once (first registration wins).
 * - A handler may be registered under several names.
 * - Handlers can be removed or destroyed while the list is processed
 *   (removed handlers are skipped, no other handler is skipped).
 * - Re-entrant calls (handler registered in itself or in a cycle) are ignored
 *   instead of recursing endlessly.
 */
class CConfigHandler : public IConfigHandler, public IRegistry {
    private:
        struct HandlerEntry {
            const char      *pszName = nullptr;
            IConfigHandler  *pHandler = nullptr;
        };

        std::vector<HandlerEntry> m_tListOfConfigHandlers;
        bool m_bInCall = false;     // guard against recursion (cyclic registration)

        /// @brief true if the entry (same name buffer and handler) is still registered.
        bool isEntryRegistered(const HandlerEntry &oEntry) const;

    protected:
        /// @brief Remove all entries of a handler (frees the names).
        void removeHandlerEntries(const void *pHandler);

    public:
        CConfigHandler() {}
        CConfigHandler(const CConfigHandler &) = delete;
        CConfigHandler & operator=(const CConfigHandler &) = delete;
        /// @brief Detaches all registered handlers and frees the names.
        ~CConfigHandler();

        /// @brief Remove a handler from this list (all names).
        void removeConfigHandler(IConfigHandler *pHandler);
        /// @brief IRegistry - a registered handler is being destroyed.
        void forgetRegisteredObject(const void *pObject) override { removeHandlerEntries(pObject); }

        /// @brief Register a config handler for the section pszName.
        /// @return true if registered (or already registered with this name), false if the
        ///         name is used by another handler or the handler is this list itself.
        bool addConfigHandler(String strName, IConfigHandler *pHandler, bool bForceInclude = false);
        /// @copydoc addConfigHandler(String,IConfigHandler*,bool)
        bool addConfigHandler(const char *pszName, IConfigHandler *pHandler, bool bForceInclude = false);
        IConfigHandler * getConfigHandler(String      strName,  int nIdx = 0);  // Get a Config Handler by his name
        IConfigHandler * getConfigHandler(const char *pszName,  int nIdx = 0);  // Get a Config Handler by his name

        /** Interface implementation */
        virtual void writeConfigTo(JsonNode &oNode, bool bHideCritical) override;          // Write your config into this Json Object
        virtual void readConfigFrom(JsonNode &oNode) override;         // Read your config from this Json Object
        void migrateConfig(JsonNode & oCfgDoc, JsonNode & oCfgNode ) override;
        void dumpConfigHandler() {
            for(HandlerEntry oEntry : m_tListOfConfigHandlers) {
                SerialPrintf("CFG: - registered config handler: %p - (%s)\n",
                              oEntry.pHandler,
                              oEntry.pszName ? oEntry.pszName : "-");
            }
        };
};

