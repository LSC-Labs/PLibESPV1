#ifndef DEBUG_LSC_STATUSHANDLER
    #undef DEBUGINFOS
#endif
#include <StatusHandler.h>
#include <DevelopmentHelper.h>

/**
 * @brief Gets a registered status handler by String name.
 * @param strName Registered status section name.
 * @return Matching handler pointer, or nullptr.
 */
IStatusHandler * CStatusHandler::getStatusHandler(String strName) {
    return(getStatusHandler(strName.c_str()));
}

/**
 * @brief Gets a registered status handler by C string name.
 * @param pszName Registered status section name.
 * @return Matching handler pointer, or nullptr.
 */
IStatusHandler * CStatusHandler::getStatusHandler(const char *pszName) {
    if(!pszName) return(nullptr);
    for (const auto& oEntry : m_tListOfStatusHandler) { 
        if(oEntry.pszName && strcmp(oEntry.pszName,pszName) == 0) return(oEntry.pHandler);
    }
    return(nullptr);
}

/**
 * @brief Checks if an entry (copied before a handler call) is still registered.
 *
 * Compares the pointers only - the name buffer of a removed entry is freed
 * and must not be read.
 */
bool CStatusHandler::isEntryRegistered(const StatusHandlerEntry &oEntry) const {
    for (const auto& oCurrent : m_tListOfStatusHandler) {
        if(oCurrent.pszName == oEntry.pszName && oCurrent.pHandler == oEntry.pHandler) return(true);
    }
    return(false);
}

/**
 * @brief Registers a status handler using a String name.
 * @param strName Section name used in the status JSON tree.
 * @param pHandler Handler that writes this status section.
 * @return see addStatusHandler(const char*,IStatusHandler*)
 */
bool CStatusHandler::addStatusHandler(String strName, IStatusHandler *pHandler) {
    return(addStatusHandler(strName.c_str(),pHandler));
}

/**
 * @brief Registers a status handler if the name is not already used.
 *
 * One section is owned by one handler: a name that is already used by another
 * handler is rejected. A list can not contain itself (endless recursion).
 *
 * @param pszName Section name used in the status JSON tree.
 * @param pHandler Handler that writes this status section.
 * @return true if registered or already registered with this name.
 */
bool CStatusHandler::addStatusHandler(const char *pszName, IStatusHandler *pHandler) {
    if(!pHandler || !pszName || !*pszName || pHandler == this) return(false);
    IStatusHandler *pOrgHandler = getStatusHandler(pszName);
    if(pOrgHandler) {
        if(pOrgHandler != pHandler) DEBUG_INFOS("STATUS: name '%s' already used by another handler - ignored",pszName);
        return(pOrgHandler == pHandler);
    }
    StatusHandlerEntry oEntry;
    oEntry.pszName  = strdup(pszName);
    oEntry.pHandler = pHandler;
    if(!oEntry.pszName) return(false);
    m_tListOfStatusHandler.push_back(oEntry);
    pHandler->m_oStatusRegistrations.attach(this);
    return(true);
}

/**
 * @brief Writes status sections for all registered child handlers.
 *
 * For each handler that reports values for the level (hasStatusValues()), a
 * subnode named after the handler is created and passed to the handler.
 * Classes that derive from CStatusHandler can write their own status values
 * first and then call this base implementation for children.
 *
 * @param oStatusNode Parent JSON node receiving all child status sections.
 * @param nLevel Requested status verbosity.
 */
void CStatusHandler::writeStatusTo(JsonNode &oStatusNode, int nLevel){
    DEBUG_FUNC_START();
    if(m_bInCall) return;               // cyclic registration - do not recurse
    m_bInCall = true;
    // Snapshot - handlers may be added/removed/destroyed in the calls
    std::vector<StatusHandlerEntry> tEntries = m_tListOfStatusHandler;
    for (const auto &oEntry : tEntries) { 
        if(!oEntry.pHandler || !isEntryRegistered(oEntry)) continue;
        if(!oEntry.pHandler->hasStatusValues(nLevel)) continue;    // no section for this level
        JsonNode * pSubNode = oStatusNode.getObject(oEntry.pszName,true);
        if(pSubNode) {
            DEBUG_INFOS(" - writing status of handler : %s",oEntry.pszName);
            oEntry.pHandler->writeStatusTo(*pSubNode,nLevel);
        }
    }
    m_bInCall = false;
    DEBUG_FUNC_END();
}

/**
 * @brief Detaches all registered handlers and frees the stored names.
 */
CStatusHandler::~CStatusHandler() {
    for (auto &oEntry : m_tListOfStatusHandler) {
        if(oEntry.pHandler) oEntry.pHandler->m_oStatusRegistrations.detach(this);
        free((void *) oEntry.pszName);
    }
    m_tListOfStatusHandler.clear();
}

/**
 * @brief Removes all entries of a handler and frees their names.
 * @param pHandler Address of the IStatusHandler interface.
 */
void CStatusHandler::removeHandlerEntries(const void *pHandler) {
    for (size_t nIdx = 0; nIdx < m_tListOfStatusHandler.size(); ) {
        if((const void *) m_tListOfStatusHandler[nIdx].pHandler == pHandler) {
            free((void *) m_tListOfStatusHandler[nIdx].pszName);
            m_tListOfStatusHandler.erase(m_tListOfStatusHandler.begin() + nIdx);
        } else nIdx++;
    }
}

/**
 * @brief Removes a status handler (all names) from this list.
 */
void CStatusHandler::removeStatusHandler(IStatusHandler *pHandler) {
    if(!pHandler) return;
    removeHandlerEntries(pHandler);
    pHandler->m_oStatusRegistrations.detach(this);
}
