#ifndef DEBUG_LSC_LIGHTSWITCH
    #undef DEBUGINFOS
#endif
#include <LightSwitch.h>
#include <Appl.h>


/**
 * @brief Creates an unconfigured light switch.
 *
 * Call setup() before using the output.
 */
CLightSwitch::CLightSwitch() {}

/**
 * @brief Creates and configures a light switch output.
 * @param nSwitchPin GPIO output pin.
 * @param bLowLevelIsOn true when LOW means logical ON.
 */
CLightSwitch::CLightSwitch(int nSwitchPin, bool bLowLevelIsOn){
    setup(nSwitchPin,bLowLevelIsOn); 
}

/**
 * @brief Toggles the output with the requested on/off timing.
 *
 * Changing the timing resets the blink sequence so the new cadence starts
 * immediately.
 */
void CLightSwitch::blink(unsigned long ulOnMillis,unsigned long ulOffMillis) {
    // static unsigned long _ulNextChange = 0;
    // static unsigned long _ulOnMillis = 0;
    // static unsigned long _ulOffMillis = 0;
    // Is a change of frequency in place... then do it immediatly (!)
    if(_ulBlinkOnMillis != ulOnMillis || _ulBlinkOffMillis != ulOffMillis) {
        _ulNextChange = 0;
        _ulBlinkOnMillis = ulOnMillis;
        _ulBlinkOffMillis = ulOffMillis;
    }
    if(millis() > _ulNextChange) {
        if(_bBlinkIsOn) {            
            switchOff();
            _bBlinkIsOn = false;
            _ulNextChange = millis() + _ulBlinkOffMillis;
        } else {
            switchOn();
            _bBlinkIsOn = true;
            _ulNextChange = millis() + _ulBlinkOnMillis;
        }
    }
}


/**
 * @brief Runs a fade-in/fade-out wave pattern on the output.
 *
 * The method is intended to be called repeatedly from a loop. It advances the
 * current brightness based on elapsed time and compensates for slow loop calls
 * by stepping until the next scheduled update lies in the future.
 */
void CLightSwitch::wave(unsigned long ulFadeInMillis, 
                        unsigned long ulFadeOutMillis, 
                        unsigned long ulOnMillis, 
                        unsigned long ulOffMillis, 
                        int nMaxLevelInPercent) {
    // static unsigned long ulNextChange = 0;  // When is next update necessary ?
    // static bool isFadeIn = true;            // Default, we start with 0 and fade in...
    // static int nCurPercent = 0;

    // If no explicit Level (in percent) - use the current setting of the light
    if(nMaxLevelInPercent < 10) nMaxLevelInPercent = this->m_nLevelInPercent;
    // Refresh necessary ?
    if(millis() > _ulNextChange) {
        if(_nWaveCurPercent > nMaxLevelInPercent) {
            // Max Fade In reached... stay on for requested time
            _ulNextChange = millis() + ulOnMillis;
            _nWaveCurPercent = nMaxLevelInPercent;
            _isWaveFadeIn = false;
        } else if(_nWaveCurPercent < 0) {
            // Light is off, stay off for requested time
            _ulNextChange = millis() + ulOffMillis;
            _nWaveCurPercent = 0;
            _isWaveFadeIn = true;
        } else {
            // ensure, already initialized...
            if(_ulNextChange < 100) _ulNextChange = millis();
            
            // set the calculated value in percent...
            // Appl.Log.logTrace("Setting output %d level to %d (off max %d)",m_nSwitchPin, nCurPercent, nMaxLevelInPercent);
            DEBUG_INFOS("Setting LED Cur % : %d, Value : %d  \r",_nWaveCurPercent,getPinLevelValueForPWM(_nWaveCurPercent));
            analogWrite(m_nPin,getPinLevelValueForPWM(_nWaveCurPercent));
 
            // Adjust new settings for light...
            if(_isWaveFadeIn) {
                // calculate and adjust fade in - the main loop needed time will be adjusted
                unsigned long ulFadeInStepTime = ulFadeInMillis / nMaxLevelInPercent;
                while(_ulNextChange <= millis()) {
                    _ulNextChange += ulFadeInStepTime;
                    _nWaveCurPercent++;
                }
            } else {
                // calculate and adjust fade out - the main loop needed time will be adjusted
                unsigned long ulFadeOutStepTime = ulFadeOutMillis / nMaxLevelInPercent;
                while(_ulNextChange <= millis()) {
                    _ulNextChange += ulFadeOutStepTime;
                    _nWaveCurPercent--;
                }
            }
       }
    }
}

/**
 * @brief Sets the output brightness in percent.
 */
void CLightSwitch::setBrightness(int nPercent) {
    setOutputLevelInPercent(nPercent);
}

/**
 * @brief Gets the output brightness in percent.
 */
int CLightSwitch::getBrightness() {
    return(getOutputLevelInPercent());
}

/**
 * @brief Runs a simple on/off diagnostic pattern on the configured pin.
 */
void CLightSwitch::runTests() {
    Serial.printf(" - testing pin : %2d  - (Light Switch)",m_nPin);
    Serial.print(" -> ");
    for(int nLoops = 2; nLoops > 0; nLoops--) {
        switchOn();
        Serial.printf("%d",m_bLowLevelIsOn ? LOW : HIGH);
        delay(1000);
        switchOff();
        Serial.printf("%d", m_bLowLevelIsOn ? HIGH : LOW);
        delay(500);
    }
    Serial.println(" ...done");
}
