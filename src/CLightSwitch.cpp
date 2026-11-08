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
 * @param bLowLevelIsOn true: active HIGH (ON writes HIGH), false: active LOW.
 *        Inverted to the name - see COutputPinController.
 */
CLightSwitch::CLightSwitch(int nSwitchPin, bool bLowLevelIsOn){
    setup(nSwitchPin,bLowLevelIsOn); 
}

/**
 * @brief Toggles the output with the requested on/off timing.
 *
 * Call repeatedly (i.e. in every loop). The first call switches on; changing
 * the timing restarts the sequence so the new cadence starts immediately.
 * Timing by CSimpleDelay (overflow safe).
 */
void CLightSwitch::blink(unsigned long ulOnMillis,unsigned long ulOffMillis) {
    // Is a change of frequency in place... then do it immediatly (!)
    if(_ulBlinkOnMillis != ulOnMillis || _ulBlinkOffMillis != ulOffMillis) {
        _ulBlinkOnMillis  = ulOnMillis;
        _ulBlinkOffMillis = ulOffMillis;
        restartBlink();
    }
    if(!_oBlinkDelay.isActive()) {
        // First call (or restarted): start with the on phase
        switchOn();
        _bBlinkIsOn = true;
        _oBlinkDelay.restartWith(_ulBlinkOnMillis);
    } else if(_oBlinkDelay.isDone()) {
        if(_bBlinkIsOn) {
            switchOff();
            _bBlinkIsOn = false;
            _oBlinkDelay.restartWith(_ulBlinkOffMillis);
        } else {
            switchOn();
            _bBlinkIsOn = true;
            _oBlinkDelay.restartWith(_ulBlinkOnMillis);
        }
    }
}

/**
 * @brief Restarts the blink cycle - the next blink() call switches on immediately.
 *
 * Used to synchronize several channels (i.e. the colors of an RGB LED).
 */
void CLightSwitch::restartBlink() {
    _oBlinkDelay.stop();
    _bBlinkIsOn = false;
}

/**
 * @brief Writes a PWM level (percent of full scale) to the pin.
 */
void CLightSwitch::writeLevel(int nPercent) {
    DEBUG_INFOS("Setting LED Cur % : %d, Value : %d  \r",nPercent,getPinLevelValueForPWM(nPercent));
    writeOutputLevel(nPercent);     // common write path (ESP32 PWM/digital handling)
}

/**
 * @brief Runs a fade-in / hold on / fade-out / hold off wave pattern on the output.
 *
 * Call repeatedly (i.e. in every loop). Each brightness step lasts
 * fade time / max level (at least 1 ms). If the loop is slow, the missed steps
 * are applied at once, so the fade time stays correct.
 * Timing by CSimpleDelay (overflow safe).
 *
 * Fixed: a fade time shorter than the level (step time 0) looped forever,
 * a level of 0 % divided by zero, and millis() overflow stopped the wave.
 *
 * @param ulFadeInMillis  Time to fade from 0 to the max level.
 * @param ulFadeOutMillis Time to fade from the max level to 0.
 * @param ulOnMillis      Time to stay at the max level.
 * @param ulOffMillis     Time to stay off.
 * @param nMaxLevelInPercent Max level (10..100); smaller values use the configured level.
 */
void CLightSwitch::wave(unsigned long ulFadeInMillis, 
                        unsigned long ulFadeOutMillis, 
                        unsigned long ulOnMillis, 
                        unsigned long ulOffMillis, 
                        int nMaxLevelInPercent) {
    // If no explicit Level (in percent) - use the current setting of the light
    if(nMaxLevelInPercent < 10) nMaxLevelInPercent = this->m_nLevelInPercent;
    if(nMaxLevelInPercent > 100) nMaxLevelInPercent = 100;
    if(nMaxLevelInPercent <= 0) {          // nothing to show (and no division by 0)
        switchOff();
        restartWave();
        return;
    }
    // Duration of one brightness step (1 %) - at least 1 ms
    unsigned long ulFadeInStep  = ulFadeInMillis  / (unsigned long) nMaxLevelInPercent;
    unsigned long ulFadeOutStep = ulFadeOutMillis / (unsigned long) nMaxLevelInPercent;
    if(ulFadeInStep  == 0) ulFadeInStep  = 1;
    if(ulFadeOutStep == 0) ulFadeOutStep = 1;

    if(!_oWaveDelay.isActive()) {
        // First call (or restarted): start fading in from 0
        _eWavePhase = WAVE_FADE_IN;
        _nWaveCurPercent = 0;
        writeLevel(0);
        _oWaveDelay.restartWith(ulFadeInStep);
        return;
    }
    if(!_oWaveDelay.isDone()) return;

    switch(_eWavePhase) {
        case WAVE_FADE_IN: {
            // missed steps (slow loop) are applied at once
            unsigned long ulSteps = _oWaveDelay.getElapsed() / ulFadeInStep;
            _nWaveCurPercent += (int) (ulSteps > 0 ? ulSteps : 1);
            if(_nWaveCurPercent >= nMaxLevelInPercent) {
                _nWaveCurPercent = nMaxLevelInPercent;
                writeLevel(_nWaveCurPercent);
                _eWavePhase = WAVE_HOLD_ON;
                _oWaveDelay.restartWith(ulOnMillis);
            } else {
                writeLevel(_nWaveCurPercent);
                _oWaveDelay.restartWith(ulFadeInStep);
            }
            break;
        }
        case WAVE_HOLD_ON:
            _eWavePhase = WAVE_FADE_OUT;
            _oWaveDelay.restartWith(ulFadeOutStep);
            break;
        case WAVE_FADE_OUT: {
            unsigned long ulSteps = _oWaveDelay.getElapsed() / ulFadeOutStep;
            _nWaveCurPercent -= (int) (ulSteps > 0 ? ulSteps : 1);
            if(_nWaveCurPercent <= 0) {
                _nWaveCurPercent = 0;
                writeLevel(0);
                _eWavePhase = WAVE_HOLD_OFF;
                _oWaveDelay.restartWith(ulOffMillis);
            } else {
                writeLevel(_nWaveCurPercent);
                _oWaveDelay.restartWith(ulFadeOutStep);
            }
            break;
        }
        case WAVE_HOLD_OFF:
            _eWavePhase = WAVE_FADE_IN;
            _oWaveDelay.restartWith(ulFadeInStep);
            break;
    }
}

/**
 * @brief Restarts the wave cycle - the next wave() call starts fading in from 0.
 */
void CLightSwitch::restartWave() {
    _oWaveDelay.stop();
    _eWavePhase = WAVE_FADE_IN;
    _nWaveCurPercent = 0;
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
