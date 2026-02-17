#ifndef DEBUG_LSC_CONFIGHANDLER
	#undef DEBUGINFOS
#endif
#include <ConfigHandler.h>
#include <DevelopmentHelper.h>
#include <ArduinoJson.h>
#include <JsonHelper.h>
#include <map>

using namespace std;

void CConfigHandler::addConfigHandler(String strName, IConfigHandler *pHandler, bool bForceInclude){
    addConfigHandler(strName.c_str(),pHandler,bForceInclude);
}

/**
 * @brief Add a ConfigHandler into the knowd ConfigHandler list...
 * Per default, ignore, if the handler already exists with the same name.
 * If you set bForceInclude to true, the handler will be inserted anyway,
 * => the config section can be shared between modules.
 * 
 * @param strName           Name of the handler like "wifi"
 * @param pHandler          The handler that can read/write the data
 * @param bForceInclude     if true, the handler will be inserted, even if the name already exists.
 *  */
void CConfigHandler::addConfigHandler(const char* pszName, IConfigHandler *pHandler, bool bForceInclude){
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

/**
 * Get a config handler by it's name.
 * If there are multiple handler with the same names, use the nIdx (zero based) counter.
 * @param strName   The registered name of the config handler
 * @param nIdx      The index of the config handler (zero based)
 */
IConfigHandler * CConfigHandler::getConfigHandler(String strName, int nIdx) {
    return(getConfigHandler(strName.c_str(),nIdx));
}

/**
 * Get a config handler by it's name.
 * If there are multiple handler with the same names, use the nIdx (zero based) counter.
 * @param pszName   The registered name of the config handler
 * @param nIdx      The index of the config handler (zero based)
 */
IConfigHandler * CConfigHandler::getConfigHandler(const char* pszName, int nIdx) {
    IConfigHandler *pHandler = nullptr;
    for (const auto& oEntry : m_tListOfConfigHandlers) { 
        if(strcmp(oEntry.pszName,pszName) == 0) {
            if (nIdx-- == 0) pHandler = oEntry.pHandler;
        }
    }
    return(pHandler);
}


/**
 * @brief  If you have sub configs, call this handler to operate them
 *         For each registered handler, a subnode with the name of the handler
 *         will be created, so the sub handler can store it's information inside.
 * @param oNode The Json node, the data should be written into.
 * @param bHideCritical if true, don't show critical values, they will be exposed!
 *  */
void CConfigHandler::writeConfigTo(JsonObject &oNode, bool bHideCritical){
    DEBUG_FUNC_START_PARMS("oNode=%p,bHideCritical=%d", &oNode, bHideCritical);
    for (const auto& oEntry : m_tListOfConfigHandlers) { 
        if(oEntry.pHandler) {
            // Write the node only, if there are config values to be written,
            // If the Handler says yes, create the JSON node and give it the config handler..
            if(oEntry.pHandler->hasConfigValues()) {
                JsonObject oSubNode = GetOrCreateJsonObject(oNode,oEntry.pszName);
                oEntry.pHandler->writeConfigTo(oSubNode,bHideCritical);
            }
        }
    }
    DEBUG_FUNC_END();
}

/**
 * Iterate all registered config handler and search their section
 * in the json node. If available, ask the handler to read their section.
 * @param oNode A JSON Object as base to iterate.
 */
void CConfigHandler::readConfigFrom(JsonObject &oNode) {
    DEBUG_FUNC_START();
    DEBUG_JSON_OBJ(oNode);
    // Iterate through known Handler... maybe duplicate names of node !
    for (const auto& oEntry : m_tListOfConfigHandlers) { 
        if(oEntry.pHandler && oEntry.pszName) {
            if(JsonKeyExists(oNode,oEntry.pszName,JsonObject)) {
                DEBUG_INFOS(" -- Reading subnode '%s'", oEntry.pszName);
                JsonObject oSubNode = GetJsonObject(oNode,oEntry.pszName);
                if(oSubNode) {
                    oEntry.pHandler->readConfigFrom(oSubNode);
                } 
            } else {
                DEBUG_INFOS("---- Subnode '%s' not found in config - skipping", oEntry.pszName);
            }
        }
    }
    DEBUG_FUNC_END();
}
