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


 #include <Arduino.h>
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
        COutputPinController();
        COutputPinController(int nSwitchPin, bool bLowLevelIsOff = true);
        virtual void setup(int SwitchPin, bool bLowLevelIsOff = true);
        virtual void switchOff();
        virtual void switchOn();
        virtual void toggleSwitch();
        virtual bool isOn();

        virtual void setOutputLevelInPercent(int nLevelInPercent = 100);
        virtual int  getOutputLevelInPercent();

    protected:
        virtual int getPinLevelValueForPWM(int nLevelInPercent);
};



