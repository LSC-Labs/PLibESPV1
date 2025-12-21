#ifndef DEBUG_LSC_INPUTPINCONTROLLER
    #undef DEBUGINFOS
#endif

#include <InputPinController.h>
#include <DevelopmentHelper.h>

#ifdef ESP8266
    #define LSC_PIN_PULLDOWN INPUT_PULLDOWN_16 
#else
    #define LSC_PIN_PULLDOWN INPUT_PULLDOWN 
#endif

/// @brief constructor
/// @param nPin Pin of chip to be controlled
/// @param bLowLevelIsOn true (default) if level is low, the pin is "On/Activ",
/// @param bWithPullUpOrDown Use a pull up or a pull down, depending on bLowLevelIsOn
CInputPinController::CInputPinController(int nPin, bool bLowLevelIsOn, bool bWithPullUpOrDown)
{
    setup(nPin,bLowLevelIsOn,bWithPullUpOrDown);
}

void CInputPinController::setup(int nPin, bool bLowLevelIsOn, bool bWithPullUpDown) {
    DEBUG_FUNC_START_PARMS("%d,%d,%d",nPin,bLowLevelIsOn,bWithPullUpDown);
    this->m_nPin = nPin;
    this->m_bLowLevelIsOn = bLowLevelIsOn;
    int nMode = INPUT;
    if(bWithPullUpDown) {
        if(m_bLowLevelIsOn) nMode = INPUT_PULLUP;
        else                nMode = LSC_PIN_PULLDOWN;
    }
    m_nMode = nMode;
    pinMode(this->m_nPin,nMode);
};

bool CInputPinController::canSendInterrupts() {
    return(digitalPinToInterrupt(m_nPin) > 0);
}

/// @brief check if the pin is logical ON/Activ, depending on the setup
/// @return true or false if the pin is logical activ / on
bool CInputPinController::isPinLogicalOn() {
    DEBUG_FUNC_START();
    int nStatus = digitalRead(m_nPin);
    bool isOn = false;
    if(m_bLowLevelIsOn) isOn = (nStatus == LOW)   ? true : false;
    else                isOn = (nStatus == HIGH)  ? true : false;
    DEBUG_FUNC_END_PARMS("%d",isOn);
    return(isOn);
}
bool CInputPinController::isPinLogicalOff() {
    return(!isPinLogicalOn());
}