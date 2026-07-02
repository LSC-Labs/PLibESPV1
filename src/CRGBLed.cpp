#ifndef DEBUG_LSC_RGBLED
    #undef DEBUGINFOS
#endif
#pragma GCC diagnostic ignored "-Wunknown-pragmas"

#include <RGBLed.h>
#include <Appl.h>
#include <LightSwitch.h>

/**
 * @brief Creates an unconfigured RGB LED.
 *
 * Call setup() before using the LED channels.
 */
CRGBLed::CRGBLed() {}

/**
 * @brief Creates and configures all three color channels.
 * @param nRedPin GPIO pin for the red channel.
 * @param nGreenPin GPIO pin for the green channel.
 * @param nBluePin GPIO pin for the blue channel.
 * @param bActiveLow true when the LED channels are active-low.
 */
CRGBLed::CRGBLed(int nRedPin, int nGreenPin, int nBluePin, bool bActiveLow) :
    RedLED(nRedPin, bActiveLow),
    GreenLED(nGreenPin,bActiveLow),
    BlueLED(nBluePin,bActiveLow) { 
        switchOff();
}

/**
 * @brief Configures all three color channels after default construction.
 */
void CRGBLed::setup(int nRedPin, int nGreenPin, int nBluePin, bool bActiveLow) {
    RedLED.setup(nRedPin,bActiveLow);
    GreenLED.setup(nGreenPin,bActiveLow);
    BlueLED.setup(nBluePin,bActiveLow);
    switchOff();
}

/**
 * @brief Switches on the channels represented by the current color bit mask.
 */
void CRGBLed::switchOn() {
    (m_nCurrentColor & COLOR_RED_BIT_MASK)   ? RedLED.switchOn()   : RedLED.switchOff();
    (m_nCurrentColor & COLOR_GREEN_BIT_MASK) ? GreenLED.switchOn() : GreenLED.switchOff();
    (m_nCurrentColor & COLOR_BLUE_BIT_MASK)  ? BlueLED.switchOn()  : BlueLED.switchOff();
}

/**
 * @brief Switches all color channels off.
 */
void CRGBLed::switchOff() {
    RedLED.switchOff();
    GreenLED.switchOff();
    BlueLED.switchOff();
}

/**
 * @brief Sets the current color using the RGB_COLOR enum.
 * @param eColor Color enum value.
 * @param bSwitchLeds true to apply the color immediately.
 */
void CRGBLed::setColor(RGB_COLOR eColor, bool bSwitchLeds) 
{
    this->setColor(eColor & 7,bSwitchLeds);
}

/**
 * @brief Sets the current color using a three-bit RGB mask.
 * @param nColorBits Bit mask using COLOR_RED/GREEN/BLUE bits.
 * @param bSwitchLeds true to apply the color immediately.
 */
void CRGBLed::setColor(int nColorBits, bool bSwitchLeds) 
{
    m_nCurrentColor = nColorBits;
    if(bSwitchLeds) switchOn();
    /*
    digitalWrite(m_nRedPin  ,nColorBits & COLOR_RED_BIT_MASK   ? m_nRGB_LED_ON : m_nRGB_LED_OFF);
    digitalWrite(m_nGreenPin,nColorBits & COLOR_GREEN_BIT_MASK ? m_nRGB_LED_ON : m_nRGB_LED_OFF);
    digitalWrite(m_nBluePin ,nColorBits & COLOR_BLUE_BIT_MASK  ? m_nRGB_LED_ON : m_nRGB_LED_OFF);
    */
}

/**
 * @brief Sets brightness for a color selected by RGB_COLOR.
 */
void CRGBLed::setBrightnessInPercent(RGB_COLOR eColor, int nBrightnessPercent) {
     this->setBrightnessInPercent(eColor & 7,nBrightnessPercent);
}

/**
 * @brief Sets PWM brightness for all channels present in the color mask.
 * @param nColorBits Three-bit RGB channel mask.
 * @param nBrightnessPercent Brightness level in percent.
 */
void CRGBLed::setBrightnessInPercent(int nColorBits, int nBrightnessPercent) {
    /*
    if(nBrightnessPercent < 0) nBrightnessPercent = 0;
    if(nBrightnessPercent > 100) nBrightnessPercent = 100;
    int nBrightnessNormal    = (nBrightnessPercent * m_nBrightnessMaxLevel) / 100;
    int nBrightnessInverted  = m_nBrightnessMaxLevel - nBrightnessNormal;
    int nBrightnessValue     = this->m_bActiveLow ? nBrightnessInverted : nBrightnessNormal;
    */
    // Serial.printf("Setting brightness to %2d == %d %d\n",nBrightness, nBrightnessValue, nBrightness *1024);   
    RedLED.setBrightness(  (nColorBits & COLOR_RED_BIT_MASK) ? nBrightnessPercent : 0);
    GreenLED.setBrightness((nColorBits & COLOR_RED_BIT_MASK) ? nBrightnessPercent : 0);
    BlueLED.setBrightness( (nColorBits & COLOR_RED_BIT_MASK) ? nBrightnessPercent : 0);
    /*
    if(nColorBits & COLOR_GREEN_BIT_MASK) analogWrite(m_nGreenPin, nBrightnessValue);
    if(nColorBits & COLOR_BLUE_BIT_MASK) analogWrite(m_nBluePin, nBrightnessValue);
    */
}


/**
 * @brief Blinks a color selected by RGB_COLOR.
 */
void CRGBLed::blink(RGB_COLOR eColor, unsigned long nOnMillis,unsigned long nOffMillis) {
    blink(eColor & 0x7,nOnMillis,nOffMillis);
}

/**
 * @brief Blinks all channels present in the color mask.
 */
void CRGBLed::blink(int nColorBits, unsigned long nOnMillis,unsigned long nOffMillis) {
    (nColorBits & COLOR_RED_BIT_MASK)   ? RedLED.blink(nOnMillis,nOffMillis)    : RedLED.switchOff();
    (nColorBits & COLOR_GREEN_BIT_MASK) ? GreenLED.blink(nOnMillis,nOffMillis)  : GreenLED.switchOff();
    (nColorBits & COLOR_BLUE_BIT_MASK)  ? BlueLED.blink(nOnMillis,nOffMillis)   : BlueLED.switchOff();
}

/**
 * @brief Runs a wave/fade pattern for a color selected by RGB_COLOR.
 */
void CRGBLed::wave(RGB_COLOR eColor, unsigned long ulFadeInMillis, 
                                     unsigned long ulFadeOutMillis,
                                     unsigned long ulOnMillis,
                                     unsigned long ulOffMillis,
                                     int nMaxLevelInPercent) {
    wave(eColor & 0x7,ulFadeInMillis,ulFadeOutMillis,ulOnMillis,ulOffMillis,nMaxLevelInPercent);
}

/**
 * @brief Runs a wave/fade pattern for all channels in the color mask.
 */
void CRGBLed::wave(int nColorBits, unsigned long ulFadeInMillis, unsigned long ulFadeOutMillis, unsigned long ulOnTime, unsigned long ulOffTime, int nMaxLevelInPercent) {
    (nColorBits & COLOR_RED_BIT_MASK)   ? RedLED.wave(ulFadeInMillis,ulFadeOutMillis,ulOnTime,ulOffTime,nMaxLevelInPercent)   : RedLED.switchOff();
    (nColorBits & COLOR_GREEN_BIT_MASK) ? GreenLED.wave(ulFadeInMillis,ulFadeOutMillis,ulOnTime,ulOffTime,nMaxLevelInPercent) : GreenLED.switchOff();
    (nColorBits & COLOR_BLUE_BIT_MASK)  ? BlueLED.wave(ulFadeInMillis,ulFadeOutMillis,ulOnTime,ulOffTime,nMaxLevelInPercent)  : BlueLED.switchOff();
}

/**
 * @brief Runs the built-in test pattern for each single channel.
 */
void CRGBLed::runSingleColorTests() 
{
    Serial.printf(" - color red...");
    RedLED.runTests();
    Serial.printf(" - color green...");
    GreenLED.runTests();
    Serial.printf(" - color blue...");
    BlueLED.runTests();
}

/**
 * @brief Shows a startup sequence through all non-zero RGB masks.
 * @param nDelayBetweenFlashes Delay between colors in milliseconds.
 */
void CRGBLed::showStartupFlashLight(int nDelayBetweenFlashes) {
    for(int nColorIdx = 0; nColorIdx < 7; nColorIdx++) {
        setColor(nColorIdx);
        delay(nDelayBetweenFlashes);
    }
    switchOff();
}

/**
 * @brief Runs all RGB LED diagnostic patterns.
 */
void CRGBLed::runTests() {
    runSingleColorTests();
    showStartupFlashLight(250);
    switchOff();
}
