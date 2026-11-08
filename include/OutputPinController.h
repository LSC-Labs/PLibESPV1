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


/**
 * @brief Base class to control a pin as output (on/off and PWM level).
 *
 * @attention For OUTPUT pins the flag bLowLevelIsOn works INVERTED to its name
 *            (kept for compatibility with existing projects):
 *            - bLowLevelIsOn = true : ON writes HIGH, OFF writes LOW  (active high,
 *              i.e. LED from GPIO to GND, common cathode RGB LED)
 *            - bLowLevelIsOn = false: ON writes LOW,  OFF writes HIGH (active low,
 *              i.e. LED from VCC to GPIO, common anode RGB LED, ESP8266 built-in LED)
 *            PWM levels follow the same rule. Input pins (CInputPinController)
 *            use the flag as named: true = LOW means "on".
 */
class COutputPinController : public CPinController {
    protected:
        int m_nLevelInPercent = 100;        // Active (PWM) Level in percent. 0 = off, 100 = on (Default is 100)
        int m_nMaxOutputLevel = 255;        // Max PWM value (analogWrite range: ESP8266 core 3.x and ESP32 = 255)
        int m_nState = -1;                  // Current State (on/off/undefined: -1)
        bool m_bPwmActive = false;          // ESP32: pin is routed to the PWM unit (LEDC) - digitalWrite has no effect

    public:
        /// @brief Create an unconfigured output pin controller.
        COutputPinController();
        /// @brief Create and configure an output pin controller.
        /// @param bLowLevelIsOn true = active HIGH, false = active LOW (inverted to the name - see class doc).
        COutputPinController(int nSwitchPin, bool bLowLevelIsOn = false);
        /// @brief Configure the output pin and active-level logic.
        /// @param bLowLevelIsOn true = active HIGH, false = active LOW (inverted to the name - see class doc).
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
        /// @brief Write an output level (0 = off, 100 = full on) - digital or PWM, see implementation.
        void writeOutputLevel(int nLevelInPercent);
};


