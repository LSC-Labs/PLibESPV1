#ifndef DEBUG_LSC_INPUTPINCONTROLLER
    #undef DEBUGINFOS
#endif

#include <InputPinController.h>
#include <DevelopmentHelper.h>


// #define LSC_PIN_PULLDOWN INPUT_PULLDOWN 


/// @brief constructor
/// @param nPin Pin of chip to be controlled
/// @param bLowLevelIsOn true (default) if level is low, the pin is "On/Activ",
/// @param bWithPullUpOrDown Use a pull up or a pull down, depending on bLowLevelIsOn
CInputPinController::CInputPinController(int nPin, bool bLowLevelIsOn, bool bWithPullUpOrDown)
{
    setup(nPin,bLowLevelIsOn,bWithPullUpOrDown);
}

/**
 * @brief setup the pin,
 * Keep in mind, on ESP8266 only pin 16 is able to pulldown
 * - If you want to use a pull down on this chip on other pins,
 *   you have to use an external pull down resistor (10 k)
 * @param nPin Pin Number
 * @param bLowLevelIsOn if true, the logical level is on, when signal is low
 * @param bWithPullUpDown if true, a pull up / down is used to get logical low - depending on bLowLevelIsOn.
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

bool CInputPinController::canSendInterrupts() {
    return(digitalPinToInterrupt(m_nPin) > 0);
}

/**
 * @brief check if the pin is logical ON/Activ, depending on the setup (bLowLevelIsOn)
 * @return true or false if the pin is logical activ / on
 *  */
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
 * @brief check if the pin is logical Off/Inactiv, depending on the setup (bLowLevelIsOn)
 * @return false or true if the pin is logical activ / off
 *  */
bool CInputPinController::isPinLogicalOff() {
    return(!isPinLogicalOn());
}