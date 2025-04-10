
#include <ConfigHandler.h>
#include <ArduinoJson.h>
#include <map>
using namespace std;

/// @brief Joins a ConfigHandler into the ConfigHandler queue...
/// @param strName 
/// @param pHandler 
void CConfigHandler::addConfigHandler(const char* strName, IConfigHandler *pHandler){
    if(pHandler && strName) {
        HandlerEntry oEntry;
        oEntry.CfgName = strName;
        oEntry.Handler = pHandler;
        tListOfEntries.push_back(oEntry);
    }
}

/// @brief  Implement this handler and call back the base class to continue proceeding
/// @param oNode 
void CConfigHandler::writeConfigTo(JsonObject &oNode, bool bHideCritical){
    for (const auto& oEntry : tListOfEntries) { 
        #if ARDUINOJSON_VERSION_MAJOR < 7
            JsonObject oSubNode = oNode[oEntry.CfgName];]
            if(!oSubNode) oSubNode = oNode.createNestedObject(oEntry.CfgName);
        #else
            // Version 7 - create or get subobject
            JsonObject oSubNode = oNode[oEntry.CfgName].to<JsonObject>();
        #endif
        if(oEntry.Handler) {
            oEntry.Handler->writeConfigTo(oSubNode,bHideCritical);
        }
    }
}

void CConfigHandler::readConfigFrom(JsonObject &oNode) {
    serializeJsonPretty(oNode,Serial);
    // Iterate through known Handler... maybe duplicate names of node !
    for (const auto& oEntry : tListOfEntries) { 
        JsonObject oSubNode = oNode[oEntry.CfgName];
        if(oSubNode && oEntry.Handler) {
            oEntry.Handler->readConfigFrom(oSubNode);
        }
    }
}

IConfigHandler *CConfigHandler::getConfigHandler(const char* strName) {
    IConfigHandler *pHandler = nullptr;
    for (const auto& oEntry : tListOfEntries) { 
        if(strcmp(oEntry.CfgName,strName)) pHandler = oEntry.Handler;
    }
    return(pHandler);
}
