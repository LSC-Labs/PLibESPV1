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
    #ifdef DEBUG_SHORT_INFOS
        #define DEBUG_INFOS(str,...)        Serial.printf( "[D] " str "\n",__VA_ARGS__)
        #define DEBUG_INFO(str)             Serial.println("[D] " str) 
    #else
        #define DEBUG_INFOS(str,...)        Serial.printf( "[D] (%s #%03d): " str "\n",__FILE__,__LINE__,__VA_ARGS__)
        #define DEBUG_INFO(str)             Serial.printf( "[D] (%s #%03d): " str "\n",__FILE__,__LINE__) 
    #endif
    #define DEBUG_FUNC_INFOS(str,...)       Serial.printf( "[D] (%s #%03d %s()): " str "\n",__FILE__,__LINE__,__FUNCTION__,__VA_ARGS__)
    #define DEBUG_FUNC_INFO(str)            Serial.printf( "[D] (%s #%03d %s()): " str "\n",__FILE__,__LINE__,__FUNCTION__) 

    #define DEBUG_FUNC_START()              Serial.printf( "[D] Function start >>>: %s\n",__PRETTY_FUNCTION__)
    #define DEBUG_FUNC_END()                Serial.printf( "[D] Function end <<<<<: %s\n",__PRETTY_FUNCTION__)
    #define DEBUG_FUNC_START_PARMS(str,...) Serial.printf( "[D] Function start >>>: %s - (" str ")\n",__PRETTY_FUNCTION__,__VA_ARGS__)
    #define DEBUG_FUNC_END_PARMS(str,...)   Serial.printf( "[D] Function end <<<<<: %s - (" str ")\n",__PRETTY_FUNCTION__,__VA_ARGS__)
    #define DEBUG_JSON_OBJ(oJsonObj)        {serializeJson(oJsonObj,Serial);Serial.println();}    
    #define DEBUG_TODO(str)                 Serial.printf("TODO - (%s - #%d): %s",__FILE__,__LINE__,str)
    #define DEBUG_TODO_INFUNC(str)          Serial.printf("TODO - (%s - #%d %s()): %s",__FILE__,__LINE__,__func__,str)
    // (!) Be carefully, this delay can have side effects. (also in Async function callbacks !)
    #define DEBUG_DELAY(ms)                 delay(ms)

#else
    
    // #pragma GCC diagnostic ignored "-Wunused-value"

    #define DEBUG_FUNC_START()              NULL_FUNCTION
    #define DEBUG_FUNC_END()                NULL_FUNCTION
    #define DEBUG_FUNC_START_PARMS(str,...) NULL_FUNCTION
    #define DEBUG_FUNC_END_PARMS(str,...)   NULL_FUNCTION
    #define DEBUG_INFOS(str,...)            NULL_FUNCTION 
    #define DEBUG_INFO(str)                 NULL_FUNCTION
    #define DEBUG_FUNC_INFOS(str,...)       NULL_FUNCTION
    #define DEBUG_FUNC_INFO(str)            NULL_FUNCTION
    #define DEBUG_JSON_OBJ(oJsonObj)        NULL_FUNCTION
    #define DEBUG_DELAY(ms)                 NULL_FUNCTION
    #define DEBUG_TODO(str)                 NULL_FUNCTION
    #define DEBUG_TODO_INFUNC(str)          NULL_FUNCTION

    
#endif

#define NULL_POINTER_STRING(str)  (str == nullptr ? "-nullptr-" : str)

#ifdef PIO_FRAMEWORK_ARDUINO_ENABLE_EXCEPTIONS
    #define LSC_ENABLE_EXCEPTIONS
#endif
