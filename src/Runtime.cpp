#include "Runtime.h"

#ifdef NATIVE_RUNTIME
    NativeSerial Serial;

    char* strlwr(char* s)
    {
        char* tmp = s;
        for (;*tmp;++tmp) {
            *tmp = tolower((unsigned char) *tmp);
        }
        return s;
    }

    unsigned long millis()
    {
        static const auto oStartTime = std::chrono::steady_clock::now();
        const auto oElapsed = std::chrono::steady_clock::now() - oStartTime;
        return static_cast<unsigned long>(1 + 
            std::chrono::duration_cast<std::chrono::milliseconds>(oElapsed).count()
        );
    }
#else


#endif
