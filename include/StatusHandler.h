#pragma once
#include <ArduinoJson.h>
#include <vector>


class IStatusHandler {
    public:
        virtual void writeStatusTo(JsonObject &oStatusNode) = 0;
};

class CStatusHandler : public IStatusHandler {
    struct StatusHandlerEntry {
        String Name;
        IStatusHandler *pHandler;
    };
    std::vector<StatusHandlerEntry> tListOfStatusHandler;

    public:
        void addStatusHandler(String strName, IStatusHandler *pHandler);
        void writeStatusTo(JsonObject &oStatusNode) override;
        IStatusHandler * getStatusHandler(String strName);  // Get a Config Handler by his name
};
