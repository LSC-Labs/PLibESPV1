#pragma once

#include <Arduino.h>
#include <OutputPinController.h>

#define LIGHT_STATE_ON 1
#define LIGHT_STATE_OFF 0

class CLightSwitch : public COutputPinController {
    private:
        unsigned long _ulNextChange = 0;
        unsigned long _ulBlinkOnMillis   = 0;
        unsigned long _ulBlinkOffMillis  = 0;
        bool          _bBlinkIsOn = false;
        bool          _isWaveFadeIn = true;            // Default, we start with 0 and fade in...
        int           _nWaveCurPercent = 0;
    
    public:
        CLightSwitch();
        CLightSwitch(int nSwitchPin, bool bLowLevelIsOff = true);

        void blink(unsigned long nOnMillis = 1000,unsigned long nOffMillis = 1000);
        void wave(unsigned long ulFadeInMillis=200, unsigned long ulFadeOutMillis = 200, unsigned long ulOnTime = 1000, unsigned long ulOffTime = 1000, int nMaxLevelInPercent = -1);
        void setBrightness(int nLevelInPercent = 100);
        int  getBrightness();        
        void runTests();
};


