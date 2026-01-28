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
            const char      *pszName = nullptr;
            IConfigHandler  *pHandler = nullptr;
        };

        // HandlerEntry *pListOfHandlerEntries;
        std::vector<HandlerEntry> m_tListOfConfigHandlers;

    public:
        CConfigHandler() {}

        void addConfigHandler(String strName, IConfigHandler *pHandler);   // Register a config handler
        void addConfigHandler(const char *pszName, IConfigHandler *pHandler);   // Register a config handler
        IConfigHandler *getConfigHandler(String strName);  // Get a Config Handler by his name
        IConfigHandler *getConfigHandler(const char *pszName);  // Get a Config Handler by his name

        /** Interface implementation */
        virtual void writeConfigTo(JsonObject &oNode, bool bHideCritical) override;          // Write your config into this Json Object
        virtual void readConfigFrom(JsonObject &oNode) override;         // Read your config from this Json Object
};

