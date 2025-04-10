#pragma once
#include <Arduino.h>
#include <LightSwitch.h>

// Names of colors to be used with the COLOR_* Bitmasks
enum RGB_COLOR {
  BLACK  = 0,
  RED    = 1,
  GREEN  = 2,
  YELLOW = 3,
  BLUE   = 4,
  PURPLE = 5,
  CYAN   = 6,
  WHITE  = 7
};


/**
 * @brief Class to handle the RGB LED
 * Initialize the RGB LED with the pins for the colors and with the
 * Information COMMON_ANODE (Active Low) or COMMON_CATHODE (Active High)
 * 
 * The Setup function must be called to initialize the pins
 */
class CRGBLed
{  
    private:
        // int m_nRedPin;
        // int m_nGreenPin;
        // int m_nBluePin;
        // bool m_bActiveLow; // Active Low (common anode) or Active High (common cathode)?
        // int m_nRGB_LED_ON  = LOW;  // Active Low
        // int m_nRGB_LED_OFF = HIGH; // Active High
        // int m_nBrightnessMaxLevel = 255; //  255 PWM adjustment for the brightness (output only 255 - not 1024)

        // bool m_bBlinkMode = false;
        int  m_nCurrentColor = RGB_COLOR::BLACK;

        // Bitmasks for the RGB LED to choose the color the easy way
        const int COLOR_RED_BIT_MASK   = 1;
        const int COLOR_GREEN_BIT_MASK = 2;
        const int COLOR_BLUE_BIT_MASK  = 4;

    public:
        CLightSwitch RedLED;
        CLightSwitch GreenLED;
        CLightSwitch BlueLED;
    
    public:
        CRGBLed();
        /**
         * @brief Constructor
         * @param nRedPin   Pin number for the red LED
         * @param nGreenPin Pin number for the green LED
         * @param nBluePin  Pin number for the blue LED
         * @param bActiveLow If true, the LED is active low (Common Cathode)
         */
        CRGBLed(int nRedPin, int nGreenPin, int nBluePin, bool bActiveLow = true);
        /**
         * Start the RGB LED and initilaize the pins
         */
        void setup(int nRedPin, int nGreenPin, int nBluePin, bool bActiveLow = true);

        void switchOn();

        /**
         * Switch off the RGB LED
         */
        void switchOff();

        /**
         * Set the color of the RGB LED
         * @param eColor Color to be set
         * @param bSwitchLeds If true, the leds will be switched to the desired states (brightness & on/off)
         */
        void setColor(RGB_COLOR eColor, bool bSwitchLeds = true);

        /**
         * Set the color of the RGB LED - follow the Bitmasks in RGB_COLOR !
         * @param nColorBits (0x1 = RED, 0x2 = GREEN, 0x4 = BLUE  / Color to be set
         * @param bSwitchLeds If true, the leds will be switched to the desired states (brightness & on/off)
         */
        void setColor(int nColorBits, bool bSwitchLeds = true);

        void blink(RGB_COLOR eColor, unsigned long nOnMillis = 500, unsigned long nOffMillis = 500);
        void blink(int nColorBits,   unsigned long nOnMillis = 500, unsigned long nOffMillis = 500);

        void wave(RGB_COLOR eColor, unsigned long ulFadeInMillis=500, unsigned long ulFadeOutMillis = 500, unsigned long ulOnTime = 1000, unsigned long ulOffTime = 2000, int nMaxLevelInPercent = -1);
        void wave(int nColorBits,   unsigned long ulFadeInMillis=500, unsigned long ulFadeOutMillis = 500, unsigned long ulOnTime = 1000, unsigned long ulOffTime = 2000, int nMaxLevelInPercent = -1);

        void setBrightnessInPercent(RGB_COLOR eColor, int nBrightnessPercent);
        void setBrightnessInPercent(int nColorBits,   int nBrightnessPercent);
        /**
         * Run the tests for the RGB LED
         */
        void runTests();
        void showStartupFlashLight(int nDelayBetweenFlashes);
    private:
        void runSingleColorTests();
        void runColorBrightnessTest();
        
};
