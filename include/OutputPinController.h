#pragma once
/**
 * OutputPinController.h 
 * 
 * (c) 2024 LSC labs
 * 
 * OutputPinController class
 * 
 * Can be initiated with the correkt pin and level  in the constructor
 * or via the setup phase of the project.
 * 
 * Respecting the pin logic (logic high = on or off) when asking for the pin status.
 * 
 */


#include <Runtime.h>
#include <PinController.h>

#define DEVICE_STATE_UNKNOWN -1
#define DEVICE_STATE_ACTIVE   1
#define DEVICE_STATE_INACTIVE 0


/// @brief Base class to controll a pin as in or output with helper functions
class COutputPinController : public CPinController {
    protected:
        int m_nLevelInPercent = 100;        // Active (PWM) Level in percent. 0 = off, 100 = on (Default is 100)
        int m_nMaxOutputLevel = 255;        // Max value to write for Level (current ESP8266 is 255)
        int m_nState = -1;                  // Current State (on/off/undefined: -1)

    public:
        /// @brief Create an unconfigured output pin controller.
        COutputPinController();
        /// @brief Create and configure an output pin controller.
        COutputPinController(int nSwitchPin, bool bLowLevelIsOn = false);
        /// @brief Configure the output pin and active-level logic.
        virtual void setup(int SwitchPin, bool bLowLevelIsOn = false);
        /// @brief Switch the controlled output off.
        virtual void switchOff();
        /// @brief Switch the controlled output on.
        virtual void switchOn();
        /// @brief Toggle the output and return the new on/off state.
        virtual bool toggleSwitch();
        /// @brief Return true if the output is currently on.
        virtual bool isOn();

        /// @brief Set the active PWM/output level in percent.
        virtual void setOutputLevelInPercent(int nLevelInPercent = 100);
        /// @brief Return the configured active PWM/output level in percent.
        virtual int  getOutputLevelInPercent();

    protected:
        /// @brief Convert a percent value to the hardware PWM range.
        virtual int getPinLevelValueForPWM(int nLevelInPercent);
};


