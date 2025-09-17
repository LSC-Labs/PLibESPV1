#pragma once
/**
 * PinController.h
 * 
 * (c) 2024 LSC Labs
 * 
 * Base class for Input / Output pins. 
 * Defines the pin and if it is activ low or activ high. 
 * Input and Output Controllers should reflect this bas information.
 */

class CPinController {
    protected:
        int m_nPin           = -1;         // The pin to control
        int m_bLowLevelIsOn  = false;      // If true, the pin is "active" on high, false, the pin is active on low
    };
