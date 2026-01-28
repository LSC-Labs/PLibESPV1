
#include <ConfigHandler.h>
#include <ArduinoJson.h>
#include <map>
using namespace std;

void CConfigHandler::addConfigHandler(String strName, IConfigHandler *pHandler){
    addConfigHandler(strName.c_str(),pHandler);
}

/// @brief Joins a ConfigHandler into the ConfigHandler queue...
/// Ignore, if already exists with the same name.
/// @param strName 
/// @param pHandler 
void CConfigHandler::addConfigHandler(const char* pszName, IConfigHandler *pHandler){
    if(pHandler && pszName) {
        // If already registered, do nothing - otherwise register.
        if(!getConfigHandler(pszName)) {
            HandlerEntry oEntry { strdup(pszName),pHandler};
            m_tListOfConfigHandlers.push_back(oEntry);
        }
    }
}


IConfigHandler *CConfigHandler::getConfigHandler(String strName) {
    return(getConfigHandler(strName.c_str()));
}

IConfigHandler *CConfigHandler::getConfigHandler(const char* pszName) {
    IConfigHandler *pHandler = nullptr;
    for (const auto& oEntry : m_tListOfConfigHandlers) { 
        if(strcmp(oEntry.pszName,pszName)) pHandler = oEntry.pHandler;
    }
    return(pHandler);
}


/// @brief  Implement this handler and call back the base class to continue proceeding
/// @param oNode 
void CConfigHandler::writeConfigTo(JsonObject &oNode, bool bHideCritical){
    for (const auto& oEntry : m_tListOfConfigHandlers) { 
        #if ARDUINOJSON_VERSION_MAJOR < 7
            JsonObject oSubNode = oNode[oEntry.pszName];]
            if(!oSubNode) oSubNode = oNode.createNestedObject(oEntry.pszName);
        #else
            // Version 7 - create or get subobject
            JsonObject oSubNode = oNode[oEntry.pszName].to<JsonObject>();
        #endif
        if(oEntry.pHandler) {
            oEntry.pHandler->writeConfigTo(oSubNode,bHideCritical);
        }
    }
}

void CConfigHandler::readConfigFrom(JsonObject &oNode) {
    serializeJsonPretty(oNode,Serial);
    // Iterate through known Handler... maybe duplicate names of node !
    for (const auto& oEntry : m_tListOfConfigHandlers) { 
        JsonObject oSubNode = oNode[oEntry.pszName];
        if(oSubNode && oEntry.pHandler) {
            oEntry.pHandler->readConfigFrom(oSubNode);
        }
    }
}
