#pragma once

#include <InputPinController.h>
#include <Ticker.h>

#define BUTTON_STATUS_ON  1
#define BUTTON_STATUS_OFF 0

/// Debouncing time of button... 
/// Default is 100 ms
#ifndef BUTTON_DEBOUNCING_TIME
    #define BUTTON_DEBOUNCING_TIME 0
#endif

class CButton : CInputPinController {
    private:
        unsigned long m_ulDebouncingTime = BUTTON_DEBOUNCING_TIME;  // Default 30ms...
        unsigned long m_ulLastCheckTime= 0L;
        volatile int m_nCurStatus = BUTTON_STATUS_OFF;
        
        void IRAM_ATTR interruptHandler();
       
        
    public:
        CButton();   
        CButton(   int nPin, bool bLowLevelIsOn = false, bool bUsePullUpDown = true);
        ~CButton();
        void setup(int nPin, bool bLowLevelIsOn = false, bool bUsePullUpDown = true);

        void startMonitoring();
        void stopMonitoring();

        bool isPressed();
};
