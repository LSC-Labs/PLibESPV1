#ifndef DEBUG_LSC_BUTTON
    #undef DEBUGINFOS
#endif

#include <Appl.h>
#include <Button.h>

#include <DevelopmentHelper.h>
#include <FunctionalInterrupt.h>

CButton::CButton() {}
CButton::~CButton() {
    stopMonitoring();
}
/// @brief constructor, is using setup()
/// @param nPin Pin of the button
/// @param bLowLevelIsOn Default is true => the button is activ on low signal
/// @param bUsePullUpDown Default is true => use a pull up / down, depending in bLowLevelIsOn
CButton::CButton(int nPin, bool bLowLevelIsOn,bool bUsePullUpDown) {
    // Initialize the button immediatly...
    setup(nPin,bLowLevelIsOn,bUsePullUpDown);
};

/// @brief set the mode of the button pin
/// @param nPin Input - Pin of the button
/// @param bLowLevelIsOn Defaul is true => the button is activ on low signal 
/// @param bUsePullUpDown Default is true => use a pull up
void CButton::setup(int nPin,bool bLowLevelIsOn, bool bUsePullUpDown) {
    CInputPinController::setup(nPin,bLowLevelIsOn,bUsePullUpDown);
    m_nCurStatus = isPinLogicalON() ? BUTTON_STATUS_ON : BUTTON_STATUS_OFF;
}

void CButton::startMonitoring(){
    DEBUG_FUNC_START();
    if(canSendInterrupts()) {
        DEBUG_INFO("BTN: - initializing interrupt handler....");
        attachInterrupt(m_nPin, std::bind(&CButton::interruptHandler, this), CHANGE);
    } 
    DEBUG_FUNC_END();
}

void CButton::stopMonitoring(){
    detachInterrupt(m_nPin);
}

/// @brief Interrupt Handler for Hardware Interrupts
/// respects the debouncing time of the last pressed button
void IRAM_ATTR CButton::interruptHandler() {
    m_nCurStatus = isPinLogicalON() ? BUTTON_STATUS_ON : BUTTON_STATUS_OFF;
    DEBUG_INFOS("BTN: pin %d is %d logical(%d) - operation mode(%d)",
                m_nPin,
                digitalRead(m_nPin),
                isPinLogicalON(),
                m_nMode);
    int nMsg = m_nCurStatus == BUTTON_STATUS_ON ? MSG_BUTTON_ON : MSG_BUTTON_OFF;
    Appl.MsgBus.sendEvent(this,nMsg ,nullptr,m_nPin);
    DEBUG_INFOS("BTN: pin %d is %s (active level == %s)",
                m_nPin,
                m_nCurStatus == BUTTON_STATUS_ON ? "pressed" : "released",
                m_bLowLevelIsOn ? "low" : "high");

}

/// @brief Check if the button is pressed...
///        Try to debounce the butten by software delay...
///        Remember the last status, to be able to detect a double click.
/// @return true or false
bool CButton::isPressed() {
    DEBUG_FUNC_START();
    if(m_ulLastCheckTime + m_ulDebouncingTime < millis()) { 
        m_nCurStatus = isPinLogicalON() ? BUTTON_STATUS_ON : BUTTON_STATUS_OFF;
        m_ulLastCheckTime = millis();
    }
    bool bResult = m_nCurStatus == BUTTON_STATUS_ON;
    DEBUG_FUNC_END_PARMS("%d",bResult);
    return(bResult);
}