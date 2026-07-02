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
    IStatusHandler *pHandler = nullptr;
    for (const auto& oEntry : m_tListOfStatusHandler) { 
        if(strcmp(oEntry.pszName,pszName) == 0) pHandler = oEntry.pHandler;
    }
    return(pHandler);
}

/**
 * @brief Registers a status handler using a String name.
 * @param strName Section name used in the status JSON tree.
 * @param pHandler Handler that writes this status section.
 */
void CStatusHandler::addStatusHandler(String strName, IStatusHandler *pHandler) {
    addStatusHandler(strName.c_str(),pHandler);
}

/**
 * @brief Registers a status handler if the name is not already used.
 * @param pszName Section name used in the status JSON tree.
 * @param pHandler Handler that writes this status section.
 */
void CStatusHandler::addStatusHandler(const char *pszName, IStatusHandler *pHandler) {
    IStatusHandler *pOrgHandler = getStatusHandler(pszName);
    if(!pOrgHandler) {
        StatusHandlerEntry oEntry { strdup(pszName), pHandler };
        m_tListOfStatusHandler.push_back(oEntry);
    } 
}

/**
 * @brief Writes status sections for all registered child handlers.
 *
 * A subnode named after each handler is created, then passed to the handler.
 * Classes that derive from CStatusHandler can write their own status values
 * first and then call this base implementation for children.
 *
 * @param oStatusNode Parent JSON node receiving all child status sections.
 * @param nLevel Requested status verbosity.
 */
void CStatusHandler::writeStatusTo(JsonNode &oStatusNode, int nLevel){
    DEBUG_FUNC_START();
    for (const auto& oEntry : m_tListOfStatusHandler) { 
        JsonNode * pSubNode = oStatusNode.getObject(oEntry.pszName,true);
        if(oEntry.pHandler) {
            DEBUG_INFOS(" - writing status of handler : %s",oEntry.pszName);
            oEntry.pHandler->writeStatusTo(*pSubNode,nLevel);
        }
    }
    DEBUG_FUNC_END();
}
