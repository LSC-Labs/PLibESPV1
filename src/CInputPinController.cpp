#ifndef DEBUG_LSC_INPUTPINCONTROLLER
    #undef DEBUGINFOS
#endif

#include <InputPinController.h>
#include <DevelopmentHelper.h>


// #define LSC_PIN_PULLDOWN INPUT_PULLDOWN 


/**
 * @brief Creates and configures an input pin controller.
 * @param nPin GPIO pin to read.
 * @param bLowLevelIsOn true when LOW should be interpreted as logical ON.
 * @param bWithPullUpOrDown true to enable an internal pull resistor that matches
 *        the active level.
 */
CInputPinController::CInputPinController(int nPin, bool bLowLevelIsOn, bool bWithPullUpOrDown)
{
    setup(nPin,bLowLevelIsOn,bWithPullUpOrDown);
}

/**
 * @brief Configures the GPIO mode and logical active level.
 *
 * On ESP8266, only pin 16 supports INPUT_PULLDOWN_16. Other pins need an
 * external pulldown resistor when LOW must be the inactive state.
 *
 * @param nPin GPIO pin number.
 * @param bLowLevelIsOn true when LOW means logical ON.
 * @param bWithPullUpDown true to enable an internal pullup or pulldown.
 */
void CInputPinController::setup(int nPin, bool bLowLevelIsOn, bool bWithPullUpDown) {
    DEBUG_FUNC_START_PARMS("%d,%d,%d",nPin,bLowLevelIsOn,bWithPullUpDown);
    this->m_nPin = nPin;
    this->m_bLowLevelIsOn = bLowLevelIsOn;
    int nMode = INPUT;
    if(bWithPullUpDown) {
        if(m_bLowLevelIsOn) nMode = INPUT_PULLUP;
        else   {
            #ifdef ESP8266
                nMode = INPUT_PULLDOWN_16; // Only pin 16 is able to pulldown - others withou effects (!)
            #else
                nMode = INPUT_PULLDOWN;
            #endif
        }             
    }
    m_nMode = nMode;
    pinMode(this->m_nPin,nMode);
};

/**
 * @brief Checks whether the configured pin can be used for interrupts.
 * @return true when digitalPinToInterrupt() reports a valid interrupt number.
 */
bool CInputPinController::canSendInterrupts() {
    return(digitalPinToInterrupt(m_nPin) > 0);
}

/**
 * @brief Checks if the current GPIO level maps to logical ON.
 * @return true when the configured active level is currently present.
 */
bool CInputPinController::isPinLogicalOn() {
    DEBUG_FUNC_START();
    int nStatus = digitalRead(m_nPin);
    bool isOn = false;
    if(m_bLowLevelIsOn) isOn = (nStatus == LOW)   ? true : false;
    else                isOn = (nStatus == HIGH)  ? true : false;
    DEBUG_FUNC_END_PARMS("%d",isOn);
    return(isOn);
}

/**
 * @brief Checks if the current GPIO level maps to logical OFF.
 * @return true when isPinLogicalOn() is false.
 */
bool CInputPinController::isPinLogicalOff() {
    return(!isPinLogicalOn());
}
