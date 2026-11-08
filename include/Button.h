#pragma once

#include <InputPinController.h>
#include <EventHandler.h>
#include <SimpleDelay.h>

#define BUTTON_STATUS_ON  1
#define BUTTON_STATUS_OFF 0

/// Debouncing time of button... 
/// Default is 100 ms
#ifndef BUTTON_DEBOUNCING_TIME
    #define BUTTON_DEBOUNCING_TIME 30
#endif

/**
 * @brief Debounced button with MSG_BUTTON_ON / MSG_BUTTON_OFF events.
 *
 * startMonitoring() attaches an interrupt and registers the button on the
 * message bus. The interrupt handler only remembers that the pin changed -
 * no events, no logging, no memory allocation in the interrupt (not allowed).
 * In MSG_APPL_LOOP the pin is read again after the debouncing time; a stable
 * new state is sent as MSG_BUTTON_ON / MSG_BUTTON_OFF (nMsgInfo = pin).
 * Bouncing contacts therefore send one event per press/release.
 */
class CButton : CInputPinController, public IMsgEventReceiver {
    private:
        volatile bool m_bIrqChanged = false;                        // set in the interrupt
        int           m_nCurStatus  = BUTTON_STATUS_OFF;            // debounced state
        CSimpleDelay  m_oDebounce   = CSimpleDelay(BUTTON_DEBOUNCING_TIME);
        bool          m_bMonitoring = false;
        
        void IRAM_ATTR interruptHandler();
        /// @brief Read the pin after the debouncing time and publish a changed state.
        void processPinChange();
        
    public:
        CButton();   
        CButton(   int nPin, bool bLowLevelIsOn = true, bool bUsePullUpDown = true);
        ~CButton();
        void setup(int nPin, bool bLowLevelIsOn = true, bool bUsePullUpDown = true);

        void startMonitoring();
        void stopMonitoring();

        /// @brief Debounced state (polls the pin if no interrupt monitoring is running).
        bool isPressed();

        /// @brief Handles MSG_APPL_LOOP (debouncing and events of the interrupt path).
        int receiveEvent(const void * pSender, int nMsg, const void * pMessage, int nMsgInfo) override;
};
