#ifndef DEBUG_LSC_BUTTON
    #undef DEBUGINFOS
#endif

#include <Appl.h>
#include <Button.h>

#include <DevelopmentHelper.h>
#include <FunctionalInterrupt.h>

/**
 * @brief Creates an unconfigured button.
 *
 * Call setup() before reading the button or starting interrupt monitoring.
 */
CButton::CButton() {}

/**
 * @brief Stops interrupt monitoring when the button object is destroyed.
 */
CButton::~CButton() {
    stopMonitoring();
}

/**
 * @brief Creates and configures a button input.
 * @param nPin GPIO pin connected to the button.
 * @param bLowLevelIsOn true when LOW means pressed.
 * @param bUsePullUpDown true to enable the matching internal pull resistor.
 */
CButton::CButton(int nPin, bool bLowLevelIsOn,bool bUsePullUpDown) {
    // Initialize the button immediatly...
    setup(nPin,bLowLevelIsOn,bUsePullUpDown);
};

/**
 * @brief Configures the button pin and initializes the cached status.
 * @param nPin GPIO input pin of the button.
 * @param bLowLevelIsOn true when LOW means pressed.
 * @param bUsePullUpDown true to enable the matching internal pull resistor.
 */
void CButton::setup(int nPin,bool bLowLevelIsOn, bool bUsePullUpDown) {
    CInputPinController::setup(nPin,bLowLevelIsOn,bUsePullUpDown);
    m_nCurStatus = isPinLogicalOn() ? BUTTON_STATUS_ON : BUTTON_STATUS_OFF;
}

/**
 * @brief Attaches the interrupt handler when the selected pin supports it.
 */
void CButton::startMonitoring(){
    DEBUG_FUNC_START();
    if(canSendInterrupts()) {
        DEBUG_INFO("BTN: - initializing interrupt handler....");
        attachInterrupt(m_nPin, std::bind(&CButton::interruptHandler, this), CHANGE);
    } 
    DEBUG_FUNC_END();
}

/**
 * @brief Detaches the interrupt handler from the configured pin.
 */
void CButton::stopMonitoring(){
    detachInterrupt(m_nPin);
}

/**
 * @brief Hardware interrupt handler that publishes button state changes.
 *
 * The handler updates the cached state immediately and sends MSG_BUTTON_ON or
 * MSG_BUTTON_OFF through the application message bus. Software debounce is done
 * by isPressed(); the interrupt path is intentionally direct so transitions are
 * not lost.
 */
void IRAM_ATTR CButton::interruptHandler() {
    m_nCurStatus = isPinLogicalOn() ? BUTTON_STATUS_ON : BUTTON_STATUS_OFF;
    DEBUG_INFOS("BTN: pin %d is %d (logicalOn == %d) - operation mode(%d)",
                m_nPin,
                digitalRead(m_nPin),
                isPinLogicalOn(),
                m_nMode);
    int nMsg = m_nCurStatus == BUTTON_STATUS_ON ? MSG_BUTTON_ON : MSG_BUTTON_OFF;
    Appl.MsgBus.sendEvent(this,nMsg ,nullptr,m_nPin);
    DEBUG_INFOS("BTN: pin %d is %s (active level == %s)",
                m_nPin,
                m_nCurStatus == BUTTON_STATUS_ON ? "pressed" : "released",
                m_bLowLevelIsOn ? "low" : "high");

}

/**
 * @brief Reads the debounced button state.
 *
 * The pin is sampled only after the configured debounce interval has elapsed.
 * Between samples, the cached status is returned.
 *
 * @return true when the button is currently considered pressed.
 */
bool CButton::isPressed() {
    DEBUG_FUNC_START();
    if(m_ulLastCheckTime + m_ulDebouncingTime < millis()) { 
        m_nCurStatus = isPinLogicalOn() ? BUTTON_STATUS_ON : BUTTON_STATUS_OFF;
        m_ulLastCheckTime = millis();
    }
    bool bResult = m_nCurStatus == BUTTON_STATUS_ON;
    DEBUG_FUNC_END_PARMS("%d",bResult);
    return(bResult);
}
