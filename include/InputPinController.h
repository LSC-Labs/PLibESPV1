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



#include <Runtime.h>
#include <PinController.h>



/**
 * @brief Input pin with logical on/off (active low or active high).
 *
 * Internal pull resistors (bWithPullUpOrDown):
 *  - active low  => pullup   (all pins, except ESP8266 GPIO16 and ESP32 GPIO34..39)
 *  - active high => pulldown (ESP32: all pins except GPIO34..39; ESP8266: GPIO16 only -
 *                   other ESP8266 pins need an external pulldown resistor)
 */
class CInputPinController : public CPinController {

    protected:
        bool m_bWithPullUpDown = false;
        int  m_nMode = INPUT;

    public:
        CInputPinController() {};
        CInputPinController(int nPin, bool bLowLevelIsOn = true, bool bWithPullUpOrDown = false);

        virtual void setup(int SwitchPin, bool bLowLevelIsOn = true, bool bWithPullUpOrDown = false);
        virtual bool isPinLogicalOn();
        virtual bool isPinLogicalOff();
        virtual bool canSendInterrupts();
};

