#pragma once
#include <ArduinoJson.h>
#include <vector>


class IStatusHandler {
    public:
        virtual void writeStatusTo(JsonObject &oStatusNode) = 0;
};

class CStatusHandler : public IStatusHandler {
    struct StatusHandlerEntry {
        const char * pszName;
        IStatusHandler *pHandler;
    };
    std::vector<StatusHandlerEntry> m_tListOfStatusHandler;

    public:
        void addStatusHandler(String strName, IStatusHandler *pHandler);
        void addStatusHandler(const char *pszName, IStatusHandler *pHandler);
        IStatusHandler * getStatusHandler(String strName);        // Get a Config Handler by his name
        IStatusHandler * getStatusHandler(const char * pszName);  // Get a Config Handler by his name

        /** Interface implementation */
        void writeStatusTo(JsonObject &oStatusNode) override;
};
