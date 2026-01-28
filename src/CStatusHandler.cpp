#ifndef DEBUG_LSC_STATUSHANDLER
    #undef DEBUGINFOS
#endif
#include <StatusHandler.h>
#include <DevelopmentHelper.h>

IStatusHandler * CStatusHandler::getStatusHandler(String strName) {
    return(getStatusHandler(strName.c_str()));
}

IStatusHandler * CStatusHandler::getStatusHandler(const char *pszName) {
    IStatusHandler *pHandler = nullptr;
    for (const auto& oEntry : m_tListOfStatusHandler) { 
        if(strcmp(oEntry.pszName,pszName) == 0) pHandler = oEntry.pHandler;
    }
    return(pHandler);
}
void CStatusHandler::addStatusHandler(String strName, IStatusHandler *pHandler) {
    addStatusHandler(strName.c_str(),pHandler);
}

void CStatusHandler::addStatusHandler(const char *pszName, IStatusHandler *pHandler) {
    IStatusHandler *pOrgHandler = getStatusHandler(pszName);
    if(!pOrgHandler) {
        StatusHandlerEntry oEntry { strdup(pszName), pHandler };
        m_tListOfStatusHandler.push_back(oEntry);
    } 
}

/**
 * Basic write operation of status
 * Override and write your own settings, then call this base class
 * to iterate through all sub - status handler...
 */
void CStatusHandler::writeStatusTo(JsonObject &oStatusNode){
    DEBUG_FUNC_START();
    for (const auto& oEntry : m_tListOfStatusHandler) { 
        #if ARDUINOJSON_VERSION_MAJOR < 7
            JsonObject oSubNode = oStatusNode[oEntry.pszName];
            if(!oSubNode) oSubNode = oStatusNode.createNestedObject(oEntry.pszName);
        #else
            JsonObject oSubNode = oStatusNode[oEntry.pszName].to<JsonObject>();
        #endif
        if(oEntry.pHandler) {
            DEBUG_INFOS(" - writing status of handler : %s",oEntry.pszName);
            oEntry.pHandler->writeStatusTo(oSubNode);
        }
    }
    DEBUG_FUNC_END();
}