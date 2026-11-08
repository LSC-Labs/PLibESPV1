#pragma once

#include <Runtime.h>
#include <OutputPinController.h>
#include <SimpleDelay.h>

#define LIGHT_STATE_ON 1
#define LIGHT_STATE_OFF 0

class CLightSwitch : public COutputPinController {
    private:
        /// @brief Phases of the wave pattern.
        enum WAVE_PHASE { WAVE_FADE_IN, WAVE_HOLD_ON, WAVE_FADE_OUT, WAVE_HOLD_OFF };

        // Blink - timer of the current on/off phase (not active = not started)
        CSimpleDelay  _oBlinkDelay;
        unsigned long _ulBlinkOnMillis   = 0;
        unsigned long _ulBlinkOffMillis  = 0;
        bool          _bBlinkIsOn = false;

        // Wave - timer of the current step/hold phase (not active = not started)
        CSimpleDelay  _oWaveDelay;
        WAVE_PHASE    _eWavePhase      = WAVE_FADE_IN;
        int           _nWaveCurPercent = 0;

        /// @brief Write a PWM level in percent to the pin.
        void writeLevel(int nPercent);
    
    public:
        CLightSwitch();
        /// @param bLowLevelIsOn true = active HIGH (default), false = active LOW -
        ///        inverted to the name, see COutputPinController.
        CLightSwitch(int nSwitchPin, bool bLowLevelIsOn = true);

        /// @brief Blink - call repeatedly (i.e. in every loop), toggles when the phase time elapsed.
        void blink(unsigned long nOnMillis = 1000,unsigned long nOffMillis = 1000);
        /// @brief Restart the blink cycle (next blink() call starts with the on phase).
        void restartBlink();
        /// @brief Fade in - hold on - fade out - hold off. Call repeatedly (i.e. in every loop).
        void wave(unsigned long ulFadeInMillis=200, unsigned long ulFadeOutMillis = 200, unsigned long ulOnTime = 1000, unsigned long ulOffTime = 1000, int nMaxLevelInPercent = -1);
        /// @brief Restart the wave cycle (next wave() call starts fading in from 0).
        void restartWave();
        void setBrightness(int nLevelInPercent = 100);
        int  getBrightness();        
        void runTests();
};


