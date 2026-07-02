#ifndef DEBUG_LSC_CONFIGHANDLER
	#undef DEBUGINFOS
#endif
#include <ConfigHandler.h>
#include <DevelopmentHelper.h>
#include <JsonNode.h>
#include <map>

using namespace std;

/**
 * @brief Registers a configuration handler using a String name.
 * @param strName Section name used in the JSON configuration tree.
 * @param pHandler Handler that reads and writes this section.
 * @param bForceInclude Reserved compatibility flag; duplicate names are still
 *        ignored by the current implementation.
 */
void CConfigHandler::addConfigHandler(String strName, IConfigHandler *pHandler, bool bForceInclude){
    addConfigHandler(strName.c_str(),pHandler,bForceInclude);
}

/**
 * @brief Adds a configuration handler to the registry.
 *
 * The handler is stored under pszName and later receives the JSON subnode with
 * the same name. Duplicate names are ignored by the current implementation, so
 * one section is owned by one handler.
 * 
 * @param pszName Name of the handler section, for example "wifi".
 * @param pHandler Handler that can read/write the section data.
 * @param bForceInclude Reserved compatibility flag.
 */
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
 * @brief Gets a registered configuration handler by String name.
 * @param strName Registered section name.
 * @param nIdx Zero-based index when multiple handlers with the same name exist.
 * @return Matching handler pointer, or nullptr.
 */
IConfigHandler * CConfigHandler::getConfigHandler(String strName, int nIdx) {
    return(getConfigHandler(strName.c_str(),nIdx));
}

/**
 * @brief Gets a registered configuration handler by C string name.
 * @param pszName Registered section name.
 * @param nIdx Zero-based index when multiple handlers with the same name exist.
 * @return Matching handler pointer, or nullptr.
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
 * @brief Writes all registered child configurations into a parent node.
 *
 * For each registered handler with config values, a child object named after the
 * handler is created and passed to that handler.
 *
 * @param oNode Parent JSON node receiving all child config sections.
 * @param bHideCritical true when sensitive values should be masked by handlers.
 */
void CConfigHandler::writeConfigTo(JsonNode &oNode, bool bHideCritical){
    DEBUG_FUNC_START_PARMS("oNode=%p,bHideCritical=%d", &oNode, bHideCritical);
    for (const auto& oEntry : m_tListOfConfigHandlers) { 
        if(oEntry.pHandler) {
            // Write the node only, if there are config values to be written,
            // If the Handler says yes, create the JSON node and give it the config handler..
            if(oEntry.pHandler->hasConfigValues()) {
                JsonNode * pSubNode = oNode.getObject(oEntry.pszName,true);
                oEntry.pHandler->writeConfigTo(*pSubNode,bHideCritical);
            }
        }
    }
    DEBUG_FUNC_END();
}


/**
 * @brief Runs migration hooks for all registered child handlers.
 *
 * Call this after all handlers have been registered, otherwise unregistered
 * modules cannot migrate their sections.
 *
 * @param oCfgDoc Root configuration document.
 * @param oCfgNode Node that contains this handler's child sections.
 */
void CConfigHandler::migrateConfig(JsonNode & oCfgDoc, JsonNode & oCfgNode) {
    DEBUG_FUNC_START();
    for (const auto& oEntry : m_tListOfConfigHandlers) { 
        if(oEntry.pHandler && oEntry.pszName) {
            JsonNode * pCfgHandlerNode = oCfgNode.getObject(oEntry.pszName,true);
            oEntry.pHandler->migrateConfig(oCfgDoc,*pCfgHandlerNode);
        }
    }
    DEBUG_FUNC_END();
}

/**
 * @brief Reads all registered child configurations from a parent node.
 *
 * Missing child sections are skipped, leaving the corresponding handler defaults
 * untouched.
 *
 * @param oNode Parent JSON node containing child config sections.
 */
void CConfigHandler::readConfigFrom(JsonNode &oNode) {
    DEBUG_FUNC_START();
    DEBUG_JSON_OBJ(oNode);
    // Iterate through known Handler... maybe duplicate names of node !
    for (const auto& oEntry : m_tListOfConfigHandlers) { 
        if(oEntry.pHandler && oEntry.pszName) {
            JsonNode * pSubNode = oNode.getObject(oEntry.pszName);
            if(pSubNode) {
                DEBUG_INFOS(" -- Reading subnode '%s'", oEntry.pszName);
                oEntry.pHandler->readConfigFrom(*pSubNode);
            } else {
                DEBUG_INFOS("---- Subnode '%s' not found in config - skipping", oEntry.pszName);
            }
        }
    }
    DEBUG_FUNC_END();
}
