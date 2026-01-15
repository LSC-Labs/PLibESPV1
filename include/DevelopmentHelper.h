#pragma once

// #define NULL_FUNCTION do { (void)0; } while (0)
#define NULL_FUNCTION {}

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
    #define DEBUG_JSON_OBJ(oJsonObj)        {serializeJson(oJsonObj,Serial);Serial.println();}    
    // (!) Be carefully, this delay can have side effects. (also in Async function callbacks !)
    #define DEBUG_DELAY(ms)                 delay(ms)
#else
    
    #pragma GCC diagnostic ignored "-Wunused-value"
    /*
        using nullFunction - cause the compile for ESP32 throws a lot of warnings when using ((void*)0)
    */
   namespace LSC {
        void nullFunction();
    }
    /*
    #define DEBUG_FUNC_START()              LSC::nullFunction()
    #define DEBUG_FUNC_END()                LSC::nullFunction()
    #define DEBUG_FUNC_START_PARMS(str,...) LSC::nullFunction()
    #define DEBUG_FUNC_END_PARMS(str,...)   LSC::nullFunction()
    #define DEBUG_DELAY(ms)                 LSC::nullFunction()
    #define DEBUG_INFOS(str,...)            LSC::nullFunction() 
    #define DEBUG_INFO(str)                 LSC::nullFunction()
    #define DEBUG_JSON_OBJ(oJsonObj)        LSC::nullFunction()
    */
    
    #define DEBUG_FUNC_START()              NULL_FUNCTION
    #define DEBUG_FUNC_END()                NULL_FUNCTION
    #define DEBUG_FUNC_START_PARMS(str,...) NULL_FUNCTION
    #define DEBUG_FUNC_END_PARMS(str,...)   NULL_FUNCTION
    #define DEBUG_DELAY(ms)                 NULL_FUNCTION
    #define DEBUG_INFOS(str,...)            NULL_FUNCTION 
    #define DEBUG_INFO(str)                 NULL_FUNCTION
    #define DEBUG_JSON_OBJ(oJsonObj)        NULL_FUNCTION

    
#endif

#define NULL_POINTER_STRING(str)  (str == nullptr ? "-nullptr-" : str)


