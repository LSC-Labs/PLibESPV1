#include "Runtime.h"

#ifdef NATIVE_RUNTIME
    /**
     * @brief Minimal Serial replacement for native tests.
     */
    NativeSerial Serial;

    /**
     * @brief Converts a C string to lower case in place.
     * @param s Mutable null-terminated string.
     * @return The same pointer that was passed in.
     */
    char* strlwr(char* s)
    {
        char* tmp = s;
        for (;*tmp;++tmp) {
            *tmp = tolower((unsigned char) *tmp);
        }
        return s;
    }

    /**
     * @brief Native replacement for Arduino millis().
     *
     * The value starts at 1 so that 0 can keep its common meaning of "timer not
     * started" in classes such as CSimpleDelay.
     *
     * @return Milliseconds since the first call, plus one.
     */
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
