
#ifndef DEBUG_LSC_OUPUTPINCONTROLLER
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
 * @param bLowLevelIsOn true when LOW should switch the external circuit on.
 */
COutputPinController::COutputPinController(int nSwitchPin, bool bLowLevelIsOn){
    setup(nSwitchPin,bLowLevelIsOn);
}

/**
 * @brief Configures the GPIO pin and switches the output off initially.
 * @param nSwitchPin GPIO output pin.
 * @param bLowLevelIsOn true when LOW means logical ON for the connected device.
 */
void COutputPinController::setup(int nSwitchPin, bool bLowLevelIsOn) {
    m_nPin = nSwitchPin;
    m_bLowLevelIsOn = bLowLevelIsOn;
    if(m_nPin > -1) {
        pinMode(m_nPin, OUTPUT);
        switchOff();
    }
}

/**
 * @brief Switches the output off according to the configured active level.
 */
void COutputPinController::switchOff() {
    if(m_nPin > -1) {
        digitalWrite(m_nPin, m_bLowLevelIsOn ? LOW : HIGH);
        m_nState = 0;
    }
}

/**
 * @brief Switches the output on.
 *
 * At 100 percent the output is written digitally. Lower levels use PWM and are
 * adapted for active-high or active-low wiring.
 */
void COutputPinController::switchOn() {
    if(m_nPin > -1) {
        if(m_nLevelInPercent == 100) digitalWrite(m_nPin,m_bLowLevelIsOn ? HIGH : LOW);
        else {
            analogWrite(m_nPin,getPinLevelValueForPWM(m_nLevelInPercent));
        }
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
 * @param nLevelInPercent Requested level from 0 to 100.
 * @return PWM value to write to the GPIO.
 */
int COutputPinController::getPinLevelValueForPWM(int nLevelInPercent) {
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
