
#ifndef DEBUG_LSC_OUPUTPINCONTROLLER
    #undef DEBUGINFOS
#endif


#include <OutputPinController.h>
#include <DevelopmentHelper.h>


    /// @brief Use this constructor if you want to setup later
COutputPinController::COutputPinController() {}

/// @brief Constructor of the class to handle the OnAir Light
/// use this constructor if you want to handle the setup immediatly.
COutputPinController::COutputPinController(int nSwitchPin, bool bLowLevelIsOn){
    setup(nSwitchPin,bLowLevelIsOn);
}

/// @brief Initialize the Light Switch - call before first usage, or use constructor with parms.
void COutputPinController::setup(int nSwitchPin, bool bLowLevelIsOn) {
    m_nPin = nSwitchPin;
    m_bLowLevelIsOn = bLowLevelIsOn;
    if(m_nPin > -1) {
        pinMode(m_nPin, OUTPUT);
        switchOff();
    }
}

void COutputPinController::switchOff() {
    if(m_nPin > -1) {
        digitalWrite(m_nPin, m_bLowLevelIsOn ? LOW : HIGH);
        m_nState = 0;
    }
}

/// @brief Switch the light on
/// if a brightnes is set, calculate the value first and make a PWM operation.
void COutputPinController::switchOn() {
    if(m_nPin > -1) {
        if(m_nLevelInPercent == 100) digitalWrite(m_nPin,m_bLowLevelIsOn ? HIGH : LOW);
        else {
            analogWrite(m_nPin,getPinLevelValueForPWM(m_nLevelInPercent));
        }
        m_nState = 1;
    }
}

void COutputPinController::toggleSwitch() {
    if(isOn()) switchOff();
    else switchOn();
}   

/// @brief get the level to write on a pin, depending on the requested LevelInPercent
/// @return the value to write into the port.
int COutputPinController::getPinLevelValueForPWM(int nLevelInPercent) {
    int nBrightnessNormal    = (nLevelInPercent * m_nMaxOutputLevel) / 100;
    return(m_bLowLevelIsOn ? nBrightnessNormal : m_nMaxOutputLevel - nBrightnessNormal);
}  

bool COutputPinController::isOn() { return(m_nState == 1); }

/// @brief set the PWM level in percent (0 - 100).
///        if the light is on, the brightness will become active (switchOn)
/// @param nLevelInPercent  0 - 100. If out of range, it will be adjusted
void COutputPinController::setOutputLevelInPercent(int nLevelInPercent) {
    if(nLevelInPercent > 100) nLevelInPercent = 100;
    if(nLevelInPercent < 0  ) nLevelInPercent = 0;
    m_nLevelInPercent = nLevelInPercent;
    if(m_nState == 1) switchOn();
}

int COutputPinController::getOutputLevelInPercent() { return(m_nLevelInPercent); }