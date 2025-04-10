#pragma once
/**
 * InputPinController.h 
 * 
 * (c) 2024 LSC labs
 * 
 * InputPinController class
 * 
 * Can be initiated with the correkt pin, level and pullup/down in the constructor
 * or via the setup phase of the project.
 * 
 * Respecting the pin logic (logic high = on or off) when asking for the pin status.
 * 
 */



#include <Arduino.h>
#include <PinController.h>



class CInputPinController : public CPinController {

    protected:
        bool m_bWithPullUpDown = false;

    public:
        CInputPinController() {};
        CInputPinController(int nPin, bool bLowLevelIsOff = false, bool bWithPullUpOrDown = false);

        virtual void setup(int SwitchPin, bool bLowLevelIsOff = true, bool bWithPullUpOrDown = false);
        virtual bool isPinLogicalON();
        virtual bool isPinLogicalOFF();
        virtual bool canSendInterrupts();
};

