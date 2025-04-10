#ifndef DEBUG_LSC_LIGHTSWITCH
    #undef DEBUGINFOS
#endif
#include <LightSwitch.h>
#include <Appl.h>


/// @brief Use this constructor if you want to setup later
CLightSwitch::CLightSwitch() {}

/// @brief Constructor of the class to handle the OnAir Light
/// use this constructor if you want to handel the setup immediatly.
CLightSwitch::CLightSwitch(int nSwitchPin, bool bLowLevelIsOff){
    setup(nSwitchPin,bLowLevelIsOff); 
}


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

void CLightSwitch::setBrightness(int nPercent) {
    setOutputLevelInPercent(nPercent);
}

int CLightSwitch::getBrightness() {
    return(getOutputLevelInPercent());
}

void CLightSwitch::runTests() {
    Serial.printf(" - testing pin : %2d  - (Light Switch)",m_nPin);
    Serial.print(" -> ");
    for(int nLoops = 2; nLoops > 0; nLoops--) {
        switchOn();
        Serial.printf("%d",m_bLowLevelIsOff ? HIGH : LOW);
        delay(1000);
        switchOff();
        Serial.printf("%d", m_bLowLevelIsOff ? LOW : HIGH);
        delay(500);
    }
    Serial.println(" ...done");
}

