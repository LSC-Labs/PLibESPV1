#pragma once

#include <Runtime.h>
#include <JsonNode.h>
#include <vector>

/**
 * Interface for configuration handlers
 */
class IConfigHandler {
    public:
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
 * Configuration handler manager
 */
class CConfigHandler : public IConfigHandler {
    private:
        enum Oper {
            CALL_READ,
            CALL_WRITE,
            CALL_MIGRATE
        };
        struct HandlerEntry {
            const char      *pszName = nullptr;
            IConfigHandler  *pHandler = nullptr;
        };

        // HandlerEntry *pListOfHandlerEntries;
        std::vector<HandlerEntry> m_tListOfConfigHandlers;

    protected:
       

    public:
        CConfigHandler() {}

        void addConfigHandler(String strName, IConfigHandler *pHandler, bool bForceInclude = false);   // Register a config handler
        void addConfigHandler(const char *pszName, IConfigHandler *pHandler, bool bForceInclude = false);   // Register a config handler
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

