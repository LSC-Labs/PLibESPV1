#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <vector>

class IConfigHandler {
    public:
        virtual void writeConfigTo(JsonObject &oCfgNode, bool bHideCritical) = 0;
        virtual void readConfigFrom(JsonObject &oCfgNode) = 0;
};

class CConfigHandler : public IConfigHandler {
    private:
        struct HandlerEntry {
            const char      *CfgName = nullptr;
            IConfigHandler  *Handler = nullptr;
        };

        HandlerEntry *pListOfHandlerEntries;
        std::vector<HandlerEntry> tListOfEntries;

    public:
        CConfigHandler() {}
        void addConfigHandler(const char *strName, IConfigHandler *pHandler);   // Register a config handler
        // virtual int  getJsonConfigSize();                    // Needed as long as the old Arduino Json is in place
        virtual void writeConfigTo(JsonObject &oNode, bool bHideCritical) override;          // Write your config into this Json Object
        virtual void readConfigFrom(JsonObject &oNode) override;         // Read your config from this Json Object
        IConfigHandler *getConfigHandler(const char *strName);  // Get a Config Handler by his name
};

