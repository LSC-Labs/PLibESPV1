#ifndef DEBUG_LSC_CONFIGHANDLER
	#undef DEBUGINFOS
#endif
#include <ConfigHandler.h>
#include <DevelopmentHelper.h>
#include <JsonNode.h>

/**
 * @brief Registers a configuration handler using a String name.
 * @param strName Section name used in the JSON configuration tree.
 * @param pHandler Handler that reads and writes this section.
 * @param bForceInclude Reserved compatibility flag (unused).
 * @return see addConfigHandler(const char*,IConfigHandler*,bool)
 */
bool CConfigHandler::addConfigHandler(String strName, IConfigHandler *pHandler, bool bForceInclude){
    return(addConfigHandler(strName.c_str(),pHandler,bForceInclude));
}

/**
 * @brief Adds a configuration handler to the registry.
 *
 * The handler is stored under pszName and later receives the JSON subnode with
 * the same name. One section is owned by one handler: a name that is already
 * used by another handler is rejected. A list can not contain itself (endless
 * recursion).
 * 
 * @param pszName Name of the handler section, for example "wifi".
 * @param pHandler Handler that can read/write the section data.
 * @param bForceInclude Reserved compatibility flag (unused).
 * @return true if registered or already registered with this name.
 */
bool CConfigHandler::addConfigHandler(const char* pszName, IConfigHandler *pHandler, bool bForceInclude){
    (void) bForceInclude;
    DEBUG_FUNC_START_PARMS("%s,%p",NULL_POINTER_STRING(pszName),pHandler);
    bool bResult = false;
    if(pHandler && pszName && *pszName && pHandler != this) {
        IConfigHandler *pExisting = getConfigHandler(pszName);
        if(pExisting) {
            bResult = (pExisting == pHandler);
            if(!bResult) DEBUG_INFOS("CFG: name '%s' already used by another handler - ignored",pszName);
        } else {
            HandlerEntry oEntry;
            oEntry.pszName  = strdup(pszName);
            oEntry.pHandler = pHandler;
            if(oEntry.pszName) {
                m_tListOfConfigHandlers.push_back(oEntry);
                pHandler->m_oConfigRegistrations.attach(this);
                bResult = true;
            }
        }
    }
    DEBUG_FUNC_END_PARMS("%d",bResult);
    return(bResult);
}

/**
 * @brief Detaches all registered handlers and frees the stored names.
 */
CConfigHandler::~CConfigHandler() {
    for (auto &oEntry : m_tListOfConfigHandlers) {
        if(oEntry.pHandler) oEntry.pHandler->m_oConfigRegistrations.detach(this);
        free((void *) oEntry.pszName);
    }
    m_tListOfConfigHandlers.clear();
}

/**
 * @brief Removes all entries of a handler and frees their names.
 * @param pHandler Address of the IConfigHandler interface.
 */
void CConfigHandler::removeHandlerEntries(const void *pHandler) {
    for (size_t nIdx = 0; nIdx < m_tListOfConfigHandlers.size(); ) {
        if((const void *) m_tListOfConfigHandlers[nIdx].pHandler == pHandler) {
            free((void *) m_tListOfConfigHandlers[nIdx].pszName);
            m_tListOfConfigHandlers.erase(m_tListOfConfigHandlers.begin() + nIdx);
        } else nIdx++;
    }
}

/**
 * @brief Removes a configuration handler (all names) from this list.
 */
void CConfigHandler::removeConfigHandler(IConfigHandler *pHandler) {
    if(!pHandler) return;
    removeHandlerEntries(pHandler);
    pHandler->m_oConfigRegistrations.detach(this);
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
    if(!pszName) return(nullptr);
    for (const auto& oEntry : m_tListOfConfigHandlers) { 
        if(oEntry.pszName && strcmp(oEntry.pszName,pszName) == 0) {
            if (nIdx-- == 0) return(oEntry.pHandler);
        }
    }
    return(nullptr);
}

/**
 * @brief Checks if an entry (copied before a handler call) is still registered.
 *
 * Compares the pointers only - the name buffer of a removed entry is freed
 * and must not be read.
 */
bool CConfigHandler::isEntryRegistered(const HandlerEntry &oEntry) const {
    for (const auto& oCurrent : m_tListOfConfigHandlers) {
        if(oCurrent.pszName == oEntry.pszName && oCurrent.pHandler == oEntry.pHandler) return(true);
    }
    return(false);
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
    if(m_bInCall) return;               // cyclic registration - do not recurse
    m_bInCall = true;
    // Snapshot - handlers may be added/removed/destroyed in the calls
    std::vector<HandlerEntry> tEntries = m_tListOfConfigHandlers;
    for (const auto &oEntry : tEntries) { 
        if(!oEntry.pHandler || !isEntryRegistered(oEntry)) continue;
        // Write the node only, if there are config values to be written,
        // If the Handler says yes, create the JSON node and give it the config handler..
        if(oEntry.pHandler->hasConfigValues()) {
            JsonNode * pSubNode = oNode.getObject(oEntry.pszName,true);
            if(pSubNode) oEntry.pHandler->writeConfigTo(*pSubNode,bHideCritical);
        }
    }
    m_bInCall = false;
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
    if(m_bInCall) return;               // cyclic registration - do not recurse
    m_bInCall = true;
    std::vector<HandlerEntry> tEntries = m_tListOfConfigHandlers;
    for (const auto &oEntry : tEntries) { 
        if(!oEntry.pHandler || !isEntryRegistered(oEntry)) continue;
        // The section is created, so the handler can move old settings into it
        JsonNode * pCfgHandlerNode = oCfgNode.getObject(oEntry.pszName,true);
        if(pCfgHandlerNode) oEntry.pHandler->migrateConfig(oCfgDoc,*pCfgHandlerNode);
    }
    m_bInCall = false;
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
    if(m_bInCall) return;               // cyclic registration - do not recurse
    m_bInCall = true;
    std::vector<HandlerEntry> tEntries = m_tListOfConfigHandlers;
    for (const auto &oEntry : tEntries) { 
        if(!oEntry.pHandler || !isEntryRegistered(oEntry)) continue;
        JsonNode * pSubNode = oNode.getObject(oEntry.pszName);
        if(pSubNode) {
            DEBUG_INFOS(" -- Reading subnode '%s'", oEntry.pszName);
            oEntry.pHandler->readConfigFrom(*pSubNode);
        } else {
            DEBUG_INFOS("---- Subnode '%s' not found in config - skipping", oEntry.pszName);
        }
    }
    m_bInCall = false;
    DEBUG_FUNC_END();
}
