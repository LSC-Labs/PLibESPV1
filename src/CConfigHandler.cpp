#ifndef DEBUG_LSC_CONFIGHANDLER
	#undef DEBUGINFOS
#endif
#include <ConfigHandler.h>
#include <DevelopmentHelper.h>
#include <ArduinoJson.h>
#include <JsonHelper.h>
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
    DEBUG_FUNC_START_PARMS("%s,%p",NULL_POINTER_STRING(pszName),pHandler);
    if(pHandler && pszName) {
        // If already registered with name, do nothing - otherwise register.
        if(!getConfigHandler(pszName)) {
            HandlerEntry oEntry { strdup(pszName),pHandler};
            m_tListOfConfigHandlers.push_back(oEntry);
        } 
    }
    DEBUG_FUNC_END();
}

IConfigHandler *CConfigHandler::getConfigHandler(String strName) {
    return(getConfigHandler(strName.c_str()));
}

IConfigHandler *CConfigHandler::getConfigHandler(const char* pszName) {
    IConfigHandler *pHandler = nullptr;
    for (const auto& oEntry : m_tListOfConfigHandlers) { 
        if(strcmp(oEntry.pszName,pszName) == 0) pHandler = oEntry.pHandler;
    }
    return(pHandler);
}


/// @brief  Implement this handler and call back the base class to continue proceeding
/// @param oNode 
void CConfigHandler::writeConfigTo(JsonObject &oNode, bool bHideCritical){
    DEBUG_FUNC_START_PARMS("oNode=%p,bHideCritical=%d", &oNode, bHideCritical);
    for (const auto& oEntry : m_tListOfConfigHandlers) { 
        JsonObject oSubNode = GetOrCreateJsonObject(oNode,oEntry.pszName);
        if(oEntry.pHandler) {
            oEntry.pHandler->writeConfigTo(oSubNode,bHideCritical);
        }
    }
    DEBUG_FUNC_END();
}

void CConfigHandler::readConfigFrom(JsonObject &oNode) {
    DEBUG_FUNC_START();
    DEBUG_JSON_OBJ(oNode);
    // Iterate through known Handler... maybe duplicate names of node !
    for (const auto& oEntry : m_tListOfConfigHandlers) { 
        DEBUG_INFOS(" -- Reading subnode '%s'", oEntry.pszName);
        JsonObject oSubNode = oNode[oEntry.pszName];
        if(oSubNode && oEntry.pHandler) {
            oEntry.pHandler->readConfigFrom(oSubNode);
        } else {
            DEBUG_INFOS("---- Subnode '%s' not found in config - skipping", oEntry.pszName);
        }
    }
    DEBUG_FUNC_END();
}
