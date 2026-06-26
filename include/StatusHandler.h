#pragma once
#include "Runtime.h"
#include "JsonNode.h"
#include <vector>

#define STATUS_LEVEL_VERBOSE   9        // With all known diag information
#define STATUS_LEVEL_INFO      5        // Info Level is default level (Frontend)
#define STATUS_LEVEL_STATE     3        // State of the device (to be published)
#define STAUTS_LEVEL_MIN       0        // Minimized level - only prio 1 infos
/**
 * Interface for a status handler module
 */
class IStatusHandler {
    public:
        virtual void writeStatusTo(JsonNode &oStatusNode, int nLevel = STATUS_LEVEL_INFO) = 0;
        /**
         * Implement if you don't want to insert your status section,
         * if there is no data in the requested level
         * Default = true, create the section and ask me;
         */
        virtual bool hasStatusValues(int nLevel = STATUS_LEVEL_INFO) { return true; }
};

/**
 * Status handler class.
 * Works as a status handler and can register further status handler modules.
 */
class CStatusHandler : public IStatusHandler {
    struct StatusHandlerEntry {
        const char * pszName;
        IStatusHandler *pHandler;
    };
    std::vector<StatusHandlerEntry> m_tListOfStatusHandler;

    public:
        void addStatusHandler(String strName, IStatusHandler *pHandler);
        void addStatusHandler(const char *pszName, IStatusHandler *pHandler);
        IStatusHandler * getStatusHandler(String strName);        // Get a Status Handler by his name
        IStatusHandler * getStatusHandler(const char * pszName);  // Get a Status Handler by his name

        /** Interface implementation */
        void writeStatusTo(JsonNode &oStatusNode, int nLevel = STATUS_LEVEL_INFO) override;
};
