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
        int  m_nPin           = -1;         // The pin to control (-1 = not configured)
        bool m_bLowLevelIsOn  = false;      // true: LOW means "on" (active low), false: HIGH means "on" (active high)
    };
