#include "Runtime.h"

#ifdef NATIVE_RUNTIME
    char* strlwr(char* s)
    {
        char* tmp = s;
        for (;*tmp;++tmp) {
            *tmp = tolower((unsigned char) *tmp);
        }
        return s;
    }
#else


#endif