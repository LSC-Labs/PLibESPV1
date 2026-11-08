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
 * ESP8266: only GPIO16 has a pulldown (INPUT_PULLDOWN_16) and GPIO16 has no
 * pullup. INPUT_PULLDOWN_16 on another pin is ignored by pinMode() - the pin
 * would stay unconfigured - so these pins are set to INPUT (external resistor needed).
 * ESP32: GPIO34..39 have no internal pull resistors (the mode is accepted, but has no effect).
 *
 * @param nPin GPIO pin number (negative = not configured, nothing is done).
 * @param bLowLevelIsOn true when LOW means logical ON.
 * @param bWithPullUpDown true to enable an internal pullup or pulldown.
 */
void CInputPinController::setup(int nPin, bool bLowLevelIsOn, bool bWithPullUpDown) {
    DEBUG_FUNC_START_PARMS("%d,%d,%d",nPin,bLowLevelIsOn,bWithPullUpDown);
    this->m_nPin = nPin;
    this->m_bLowLevelIsOn = bLowLevelIsOn;
    this->m_bWithPullUpDown = bWithPullUpDown;
    int nMode = INPUT;
    if(bWithPullUpDown) {
        #ifdef ESP8266
            if(nPin == 16) nMode = m_bLowLevelIsOn ? INPUT : INPUT_PULLDOWN_16;  // GPIO16: pulldown only
            else           nMode = m_bLowLevelIsOn ? INPUT_PULLUP : INPUT;      // no pulldown on GPIO0..15
        #else
            nMode = m_bLowLevelIsOn ? INPUT_PULLUP : INPUT_PULLDOWN;
        #endif
    }
    m_nMode = nMode;
    if(m_nPin >= 0) pinMode(this->m_nPin,nMode);
    DEBUG_FUNC_END();
}

/**
 * @brief Checks whether the configured pin can be used for interrupts.
 *
 * digitalPinToInterrupt() returns the pin number or NOT_AN_INTERRUPT (-1).
 * GPIO0 (interrupt 0 - the BOOT/FLASH button on many boards) is valid; the
 * previous check "> 0" excluded it. ESP8266 GPIO16 has no interrupt.
 *
 * @return true when the pin has an interrupt.
 */
bool CInputPinController::canSendInterrupts() {
    return(m_nPin >= 0 && digitalPinToInterrupt(m_nPin) != NOT_AN_INTERRUPT);
}

/**
 * @brief Checks if the current GPIO level maps to logical ON.
 * @return true when the configured active level is currently present
 *         (false for a not configured pin).
 */
bool CInputPinController::isPinLogicalOn() {
    if(m_nPin < 0) return(false);
    int nStatus = digitalRead(m_nPin);
    return(m_bLowLevelIsOn ? (nStatus == LOW) : (nStatus == HIGH));
}

/**
 * @brief Checks if the current GPIO level maps to logical OFF.
 * @return true when isPinLogicalOn() is false.
 */
bool CInputPinController::isPinLogicalOff() {
    return(!isPinLogicalOn());
}
