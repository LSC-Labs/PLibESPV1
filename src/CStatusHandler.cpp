#ifndef DEBUG_LSC_STATUSHANDLER
    #undef DEBUGINFOS
#endif
#include <StatusHandler.h>
#include <DevelopmentHelper.h>

IStatusHandler * CStatusHandler::getStatusHandler(String strName) {
    IStatusHandler *pHandler = nullptr;
    for (const auto& oEntry : tListOfStatusHandler) { 
        if(oEntry.Name == strName) pHandler = oEntry.pHandler;
    }
    return(pHandler);
}

void CStatusHandler::addStatusHandler(String strName, IStatusHandler *pHandler) {
    IStatusHandler *pOrgHandler = getStatusHandler(strName);
    if(!pOrgHandler) {
        StatusHandlerEntry oEntry { strName, pHandler };
        tListOfStatusHandler.push_back(oEntry);
    } 
}

/**
 * Basic write operation of status
 * Override and write your own settings, then call this base class
 * to iterate through all sub - status handler...
 */
void CStatusHandler::writeStatusTo(JsonObject &oStatusNode){
    DEBUG_FUNC_START();
    for (const auto& oEntry : tListOfStatusHandler) { 
        #if ARDUINOJSON_VERSION_MAJOR < 7
            JsonObject oSubNode = oStatusNode[oEntry.Name];
            if(!oSubNode) oSubNode = oStatusNode.createNestedObject(oEntry.Name);
        #else
            JsonObject oSubNode = oStatusNode[oEntry.Name].to<JsonObject>();
        #endif
        if(oEntry.pHandler) {
            DEBUG_INFOS(" - writing status of handler : %s",oEntry.Name.c_str());
            oEntry.pHandler->writeStatusTo(oSubNode);
        }
    }
    DEBUG_FUNC_END();
}