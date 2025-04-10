#pragma once

/**
 * Development Helpers
 * 
 * Modules only available in Developemnet mode, controlled by preprocessor directives
 * 
 * -D DEBUGINFOS: Insert Debug Infos into the prog, which are removed in final version.
 *     DEBUG_FUNC_START     : Writes the called function name to serial port
 *     DEBUG_FUNC_END       : Writes the end of the called function to serial port         
 */
#ifdef DEBUGINFOS
    #define DEBUG_TODO(str)                 Serial.printf("TODO in file %s (%d): " str,__file__,__LINE__)
    #define DEBUG_FUNC_TODO(str)            Serial.printf("TODO in file %s (line %d) -> %s(): " str,__FILE__,__LINE__,__func__)
    #define DEBUG_FUNC_START()              Serial.printf( "[D] Function start: %s\n",__PRETTY_FUNCTION__)
    #define DEBUG_FUNC_END()                Serial.printf( "[D] Function -end-: %s\n",__PRETTY_FUNCTION__)
    #define DEBUG_FUNC_START_PARMS(str,...) Serial.printf( "[D] Function start: %s - (" str ")\n",__PRETTY_FUNCTION__,__VA_ARGS__)
    #define DEBUG_FUNC_END_PARMS(str,...)   Serial.printf( "[D] Function -end-: %s - (" str ")\n",__PRETTY_FUNCTION__,__VA_ARGS__)
    #define DEBUG_INFOS(str,...)            Serial.printf( "[D] " str "\n",__VA_ARGS__)
    #define DEBUG_INFO(str)                 Serial.println("[D] " str) 
    #define DEBUG_JSON_OBJ(oJsonObj)        {serializeJson(oJsonObj,Serial);Serial.println();}    // (!) Be carefully, this delay can have side effects. (also in Async function callbacks !)
    #define DEBUG_DELAY(ms)                 delay(ms)
#else
    #pragma GCC diagnostic ignored "-Wunused-value"
    #define DEBUG_FUNC_START()              ((void*)0)
    #define DEBUG_FUNC_END()                ((void*)0)
    #define DEBUG_FUNC_START_PARMS(str,...) ((void*)0)
    #define DEBUG_FUNC_END_PARMS(str,...)   ((void*)0)
    #define DEBUG_DELAY(ms)                 ((void*)0)
    #define DEBUG_INFOS(str,...)            ((void*)0) 
    #define DEBUG_INFO(str)                 ((void*)0)
    #define DEBUG_JSON_OBJ(oJsonObj)        ((void*)0)
#endif

#define NULL_POINTER_STRING(str)  (str == nullptr ? "-nullptr-" : str)


