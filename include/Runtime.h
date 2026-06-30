#pragma once

/**
 * Runtime.h
 * 
 * Runtime includes per Default "Arduino.h"
 * 
 * But, as this runtime is not available in native mode, this runtime inlcudes is also a helper
 * to test a lot of Arduino functions in native mode.
 * This reduces the build/upload/run tests for the module, by using the native OS as runtime.
 * But for this it is necessary to map several parts from Arduino to standard libraries,
 * cause Arduino runtime is not available in native environments.
 * 
 * Base definition to enable the mapping is "NATIVE_RUNTIME". If not defined, Arduino. stays in place.
 * 
 */

#define JOIN_(X,Y) X##Y
#define JOIN(X,Y) JOIN_(X,Y)

#ifdef NATIVE_RUNTIME
    #include <string.h>
    #include <chrono>
    #include <iostream>
    

    #define String std::string
    #define SerialPrintf printf
    #define ICACHE_FLASH_ATTR
    #define F(value) value

    class NativeSerial {
        public:
            template<typename... Args>
            void printf(const char *pszFormat, Args... args) { ::printf(pszFormat,args...); }
            void print(const char *pszValue) { ::printf("%s",pszValue); }
            void println(const char *pszValue) { ::printf("%s\n",pszValue); }
    };

    extern NativeSerial Serial;

    char* strlwr(char* s);
    unsigned long millis();
#else
    #include <Arduino.h>
    
    #define SerialPrintf Serial.printf
    #define puts(s)      Serial.println(s)
#endif
