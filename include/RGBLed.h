#pragma once
#include <Runtime.h>
#include <LightSwitch.h>

/// @brief Named RGB color bitmasks used by CRGBLed.
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
 * Initialize the RGB LED with the pins for the colors and the wiring:
 *  - bActiveLow = true  (default): common CATHODE LED (cathode to GND, a channel
 *                                  is ON with HIGH)
 *  - bActiveLow = false          : common ANODE LED (anode to VCC, a channel is
 *                                  ON with LOW)
 * @attention The name bActiveLow is inverted to the behavior (kept for
 *            compatibility, see COutputPinController): true means active HIGH.
 * 
 * The Setup function must be called to initialize the pins
 */
class CRGBLed
{  
    private:
        // int m_nRedPin;
        // int m_nGreenPin;
        // int m_nBluePin;

        // int m_nBrightnessMaxLevel = 255; //  255 PWM adjustment for the brightness (output only 255 - not 1024)

        // bool m_bBlinkMode = false;
        int  m_nCurrentColor = RGB_COLOR::BLACK;

        // Bitmasks for the RGB LED to choose the color the easy way
        const int COLOR_RED_BIT_MASK   = 1;
        const int COLOR_GREEN_BIT_MASK = 2;
        const int COLOR_BLUE_BIT_MASK  = 4;

    public:
        /// @brief Red channel light switch.
        CLightSwitch RedLED;
        /// @brief Green channel light switch.
        CLightSwitch GreenLED;
        /// @brief Blue channel light switch.
        CLightSwitch BlueLED;
    
    public:
        /// @brief Create an unconfigured RGB LED.
        CRGBLed();
        /**
         * @brief Constructor
         * @param nRedPin   Pin number for the red LED
         * @param nGreenPin Pin number for the green LED
         * @param nBluePin  Pin number for the blue LED
         * @param bActiveLow true (default): common cathode, channel ON = HIGH;
         *                   false: common anode, channel ON = LOW (name inverted - see class doc)
         */
        CRGBLed(int nRedPin, int nGreenPin, int nBluePin, bool bActiveLow = true);
        /**
         * @brief Configure the RGB LED pins and active-level mode.
         * @param bActiveLow true (default): common cathode (ON = HIGH), false: common anode (ON = LOW).
         */
        void setup(int nRedPin, int nGreenPin, int nBluePin, bool bActiveLow = true);

        /// @brief Switch all active color channels on.
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

        /// @brief Restart the blink cycle of all channels (keeps mixed colors in sync).
        void restartBlink() { RedLED.restartBlink(); GreenLED.restartBlink(); BlueLED.restartBlink(); }
        /// @brief Restart the wave cycle of all channels (keeps mixed colors in sync).
        void restartWave()  { RedLED.restartWave();  GreenLED.restartWave();  BlueLED.restartWave(); }
        /// @brief Blink the given named color.
        void blink(RGB_COLOR eColor, unsigned long nOnMillis = 500, unsigned long nOffMillis = 500);
        /// @brief Blink the given color bitmask.
        void blink(int nColorBits,   unsigned long nOnMillis = 500, unsigned long nOffMillis = 500);

        /// @brief Fade the given named color in/out repeatedly.
        void wave(RGB_COLOR eColor, unsigned long ulFadeInMillis=500, unsigned long ulFadeOutMillis = 500, unsigned long ulOnTime = 1000, unsigned long ulOffTime = 2000, int nMaxLevelInPercent = -1);
        /// @brief Fade the given color bitmask in/out repeatedly.
        void wave(int nColorBits,   unsigned long ulFadeInMillis=500, unsigned long ulFadeOutMillis = 500, unsigned long ulOnTime = 1000, unsigned long ulOffTime = 2000, int nMaxLevelInPercent = -1);

        /// @brief Set brightness for one named color channel/group.
        void setBrightnessInPercent(RGB_COLOR eColor, int nBrightnessPercent);
        /// @brief Set brightness for one color bitmask channel/group.
        void setBrightnessInPercent(int nColorBits,   int nBrightnessPercent);
        /**
         * Run the tests for the RGB LED
         */
        void runTests();
        /// @brief Show a short startup flash sequence.
        void showStartupFlashLight(int nDelayBetweenFlashes);
    private:
        /// @brief Run single-color diagnostics.
        void runSingleColorTests();
        /// @brief Run color brightness diagnostics.
        void runColorBrightnessTest();
        
};
