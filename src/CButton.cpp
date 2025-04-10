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

CButton::CButton(int nPin, bool bLowLevelIsOff,bool bUsePullUpDown) {
    // Initialize the button immediatly...
    setup(nPin,bLowLevelIsOff,bUsePullUpDown);
};

void CButton::setup(int nPin,bool bLowLevelIsOff, bool bUsePullUpDown) {
    CInputPinController::setup(nPin,bLowLevelIsOff,bUsePullUpDown);
    // m_nButtonActiveStatus = bLowLevelIsOff ? HIGH : LOW;
    m_nCurStatus = isPinLogicalON() ? BUTTON_STATUS_ON : BUTTON_STATUS_OFF;
}

void CButton::startMonitoring(){
    DEBUG_FUNC_START();
    if(canSendInterrupts()) {
        DEBUG_INFO("BTN: - initializing interrupt handler....");
        attachInterrupt(m_nPin, std::bind(&CButton::interruptHandler, this), CHANGE);
    } 
}

void CButton::stopMonitoring(){
    detachInterrupt(m_nPin);
}

/// @brief Interrupt Handler for Hardware Interrupts
/// respects the debouncing time of the last pressed button
void IRAM_ATTR CButton::interruptHandler() {

    m_nCurStatus = isPinLogicalON() ? BUTTON_STATUS_ON : BUTTON_STATUS_OFF;
    int nMsg = m_nCurStatus == BUTTON_STATUS_ON ? MSG_BUTTON_ON : MSG_BUTTON_OFF;
    Appl.MsgBus.sendEvent(this,nMsg ,nullptr,m_nPin);
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