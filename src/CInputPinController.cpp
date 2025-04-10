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

CInputPinController::CInputPinController(int nPin, bool bLowLevelIsOff, bool bWithPullUpOrDown)
{
    setup(nPin,bLowLevelIsOff,bWithPullUpOrDown);
}

void CInputPinController::setup(int nPin, bool bLowLevelIsOff, bool bWithPullUpDown) {
    DEBUG_FUNC_START_PARMS("%d,%d,%d",nPin,bLowLevelIsOff,bWithPullUpDown);
    this->m_nPin = nPin;
    this->m_bLowLevelIsOff = bLowLevelIsOff;
    int nMode = INPUT;
    if(bWithPullUpDown) {
        if(m_bLowLevelIsOff) nMode = LSC_PIN_PULLDOWN;
        else                 nMode = INPUT_PULLUP;
    }
    pinMode(this->m_nPin,nMode);
};

bool CInputPinController::canSendInterrupts() {
    return(digitalPinToInterrupt(m_nPin) > 0);
}


bool CInputPinController::isPinLogicalON() {
    DEBUG_FUNC_START();
    int nStatus = digitalRead(m_nPin);
    bool isOn = false;
    if(m_bLowLevelIsOff) isOn = (nStatus == HIGH) ? true : false;
    else                 isOn = (nStatus == LOW)  ? true : false;
    DEBUG_FUNC_END_PARMS("%d",isOn);
    return(isOn);
}
bool CInputPinController::isPinLogicalOFF() {
    return(!isPinLogicalON());
}