
#ifndef DEBUG_LSC_OUTPUTPINCONTROLLER
    #undef DEBUGINFOS
#endif


#include <OutputPinController.h>
#include <DevelopmentHelper.h>


/**
 * @brief Creates an output pin controller without configuring a pin.
 *
 * Use this constructor when setup() will be called later.
 */
COutputPinController::COutputPinController() {}

/**
 * @brief Creates and configures an output pin controller.
 * @param nSwitchPin GPIO output pin.
 * @param bLowLevelIsOn true: ON writes HIGH (active high), false: ON writes LOW (active low).
 *        Inverted to the name - see the class documentation in OutputPinController.h.
 */
COutputPinController::COutputPinController(int nSwitchPin, bool bLowLevelIsOn){
    setup(nSwitchPin,bLowLevelIsOn);
}

/**
 * @brief Configures the GPIO pin and switches the output off initially.
 * @param nSwitchPin GPIO output pin.
 * @param bLowLevelIsOn true: ON writes HIGH (active high), false: ON writes LOW (active low).
 *        Inverted to the name - see the class documentation in OutputPinController.h.
 */
void COutputPinController::setup(int nSwitchPin, bool bLowLevelIsOn) {
    m_nPin = nSwitchPin;
    m_bLowLevelIsOn = bLowLevelIsOn;
    m_bPwmActive = false;
    if(m_nPin > -1) {
        pinMode(m_nPin, OUTPUT);
        switchOff();
    }
}

/**
 * @brief Writes an output level - all writes to the pin go through this function.
 *
 * 0 % and 100 % are written digitally, other levels with PWM (analogWrite).
 * ESP32: analogWrite() routes the pin to the PWM unit (LEDC) - afterwards
 * digitalWrite() has no effect on the pin. Once PWM was used, also 0 % and
 * 100 % are written with PWM (duty 0 / max), otherwise switchOff() would leave
 * the LED at the last PWM level.
 * ESP8266: digitalWrite() stops a running PWM itself - no special handling.
 *
 * @param nLevelInPercent 0 (off) .. 100 (full on), clamped.
 */
void COutputPinController::writeOutputLevel(int nLevelInPercent) {
    if(m_nPin < 0) return;
    if(nLevelInPercent < 0)   nLevelInPercent = 0;
    if(nLevelInPercent > 100) nLevelInPercent = 100;
    if(!m_bPwmActive && (nLevelInPercent == 0 || nLevelInPercent == 100)) {
        bool bOn = nLevelInPercent == 100;
        // bLowLevelIsOn == true: ON = HIGH (inverted to the name - see class doc)
        digitalWrite(m_nPin, bOn == m_bLowLevelIsOn ? HIGH : LOW);
        return;
    }
    analogWrite(m_nPin, getPinLevelValueForPWM(nLevelInPercent));
    #ifdef ESP32
        m_bPwmActive = true;
    #endif
}

/**
 * @brief Switches the output off according to the configured active level.
 *
 * Writes LOW for bLowLevelIsOn == true, HIGH for false (see class doc).
 */
void COutputPinController::switchOff() {
    if(m_nPin > -1) {
        writeOutputLevel(0);
        m_nState = 0;
    }
}

/**
 * @brief Switches the output on with the configured level.
 *
 * At 100 percent the output is written digitally (HIGH for bLowLevelIsOn == true,
 * LOW for false - see class doc). Lower levels use PWM and are adapted for
 * active-high or active-low wiring.
 */
void COutputPinController::switchOn() {
    if(m_nPin > -1) {
        writeOutputLevel(m_nLevelInPercent);
        m_nState = 1;
    }
}

/**
 * @brief Toggles between switchOn() and switchOff().
 * @return New logical state after toggling.
 */
bool COutputPinController::toggleSwitch() {
    if(isOn()) switchOff();
    else switchOn();
    return(isOn());
}   

/**
 * @brief Converts a percentage into a PWM value for the configured active level.
 *
 * bLowLevelIsOn == true: duty cycle = level (active high),
 * false: duty cycle inverted (active low).
 * @param nLevelInPercent Requested level from 0 to 100.
 * @return PWM value to write to the GPIO.
 */
int COutputPinController::getPinLevelValueForPWM(int nLevelInPercent) {
    if(nLevelInPercent < 0)   nLevelInPercent = 0;
    if(nLevelInPercent > 100) nLevelInPercent = 100;
    int nBrightnessNormal    = (nLevelInPercent * m_nMaxOutputLevel) / 100;
    return(m_bLowLevelIsOn ? nBrightnessNormal : m_nMaxOutputLevel - nBrightnessNormal);
}  

/**
 * @brief Gets the cached logical output state.
 * @return true when the output is currently considered on.
 */
bool COutputPinController::isOn() { return(m_nState == 1); }

/**
 * @brief Sets the PWM output level in percent.
 *
 * Values outside 0..100 are clamped. If the output is currently on, the new
 * level is applied immediately by calling switchOn().
 *
 * @param nLevelInPercent Requested level in percent.
 */
void COutputPinController::setOutputLevelInPercent(int nLevelInPercent) {
    if(nLevelInPercent > 100) nLevelInPercent = 100;
    if(nLevelInPercent < 0  ) nLevelInPercent = 0;
    m_nLevelInPercent = nLevelInPercent;
    if(m_nState == 1) switchOn();
}

/**
 * @brief Gets the configured output level.
 * @return Current level in percent.
 */
int COutputPinController::getOutputLevelInPercent() { return(m_nLevelInPercent); }
