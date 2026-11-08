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
    stopMonitoring();       // the bus registration is removed automatically (IMsgEventReceiver)
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
 * @brief Attaches the interrupt handler and registers on the message bus.
 *
 * The bus registration is needed to process the pin changes in the loop.
 */
void CButton::startMonitoring(){
    DEBUG_FUNC_START();
    if(canSendInterrupts() && !m_bMonitoring) {
        DEBUG_INFO("BTN: - initializing interrupt handler....");
        Appl.MsgBus.registerEventReceiver(this,"Button");
        attachInterrupt(m_nPin, std::bind(&CButton::interruptHandler, this), CHANGE);
        m_bMonitoring = true;
    } 
    DEBUG_FUNC_END();
}

/**
 * @brief Detaches the interrupt handler from the configured pin.
 */
void CButton::stopMonitoring(){
    if(m_bMonitoring) {
        detachInterrupt(m_nPin);
        m_bMonitoring = false;
    }
}

/**
 * @brief Hardware interrupt handler - only remembers the change.
 *
 * Runs in interrupt context: no events, no logging, no memory allocation, no
 * mutex - all of that crashes in an ISR (ESP32 "Guru Meditation", ESP8266
 * exception). The change is processed in the loop (processPinChange()).
 */
void IRAM_ATTR CButton::interruptHandler() {
    m_bIrqChanged = true;
}

/**
 * @brief Processes pin changes in the loop: debounce, then publish the new state.
 *
 * After a change the pin is read again when the debouncing time has elapsed.
 * Only a state that differs from the last published state is sent.
 */
void CButton::processPinChange() {
    if(m_bIrqChanged) {
        m_bIrqChanged = false;
        m_oDebounce.restart();              // (re)start debouncing - contact still bouncing
        return;
    }
    if(!m_oDebounce.isActive() || !m_oDebounce.isDone()) return;
    m_oDebounce.stop();
    int nNewStatus = isPinLogicalOn() ? BUTTON_STATUS_ON : BUTTON_STATUS_OFF;
    if(nNewStatus == m_nCurStatus) return;  // bounced back - no change
    m_nCurStatus = nNewStatus;
    DEBUG_INFOS("BTN: pin %d is %s", m_nPin, m_nCurStatus == BUTTON_STATUS_ON ? "pressed" : "released");
    Appl.MsgBus.sendEvent(this, m_nCurStatus == BUTTON_STATUS_ON ? MSG_BUTTON_ON : MSG_BUTTON_OFF, nullptr, m_nPin);
}

/**
 * @brief Handles MSG_APPL_LOOP (debouncing and events of the interrupt path).
 */
int CButton::receiveEvent(const void * pSender, int nMsg, const void * pMessage, int nMsgInfo) {
    (void) pSender; (void) pMessage; (void) nMsgInfo;
    if(nMsg == MSG_APPL_LOOP) processPinChange();
    return(EVENT_MSG_RESULT_OK);
}

/**
 * @brief Reads the debounced button state.
 *
 * With interrupt monitoring the state of the interrupt path is returned.
 * Without, the pin is sampled at most once per debouncing time.
 *
 * @return true when the button is currently considered pressed.
 */
bool CButton::isPressed() {
    if(!m_bMonitoring && (!m_oDebounce.isActive() || m_oDebounce.isDone())) {
        m_nCurStatus = isPinLogicalOn() ? BUTTON_STATUS_ON : BUTTON_STATUS_OFF;
        m_oDebounce.restart();
    }
    return(m_nCurStatus == BUTTON_STATUS_ON);
}
