#ifndef DEBUG_LSC_RGBLED
    #undef DEBUGINFOS
#endif
#pragma GCC diagnostic ignored "-Wunknown-pragmas"

#include <RGBLed.h>
#include <Appl.h>
#include <LightSwitch.h>

/// @brief Empty constructor - use setup is mandatory !
CRGBLed::CRGBLed() {}

/// @brief Constructor
/// @param nRedPin   Pin number for the red LED
/// @param nGreenPin Pin number for the green LED
/// @param nBluePin  Pin number for the blue LED
/// @param bActiveLow If true, the LED is active low (Common Cathode)
CRGBLed::CRGBLed(int nRedPin, int nGreenPin, int nBluePin, bool bActiveLow) : 
    RedLED(nRedPin,bActiveLow),
    GreenLED(nGreenPin,bActiveLow),
    BlueLED(nBluePin,bActiveLow) {
        switchOff();
}

void CRGBLed::setup(int nRedPin, int nGreenPin, int nBluePin, bool bActiveLow) {
    RedLED.setup(nRedPin,bActiveLow);
    GreenLED.setup(nGreenPin,bActiveLow);
    BlueLED.setup(nBluePin,bActiveLow);
    switchOff();
}

void CRGBLed::switchOn() {
    (m_nCurrentColor & COLOR_RED_BIT_MASK)   ? RedLED.switchOn()   : RedLED.switchOff();
    (m_nCurrentColor & COLOR_GREEN_BIT_MASK) ? GreenLED.switchOn() : GreenLED.switchOff();
    (m_nCurrentColor & COLOR_BLUE_BIT_MASK)  ? BlueLED.switchOn()  : BlueLED.switchOff();
}

void CRGBLed::switchOff() {
    RedLED.switchOff();
    GreenLED.switchOff();
    BlueLED.switchOff();
}

/// @brief set the colre by using the Color Tabel with masks
/// @param eColor 
void CRGBLed::setColor(RGB_COLOR eColor, bool bSwitchLeds) 
{
    this->setColor(eColor & 7,bSwitchLeds);
}

/// @brief set the RGB LED with a bit mask, where RGB is coded as 3 bits
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

void CRGBLed::setBrightnessInPercent(RGB_COLOR eColor, int nBrightnessPercent) {
     this->setBrightnessInPercent(eColor & 7,nBrightnessPercent);
}

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


void CRGBLed::blink(RGB_COLOR eColor, unsigned long nOnMillis,unsigned long nOffMillis) {
    blink(eColor & 0x7,nOnMillis,nOffMillis);
}

void CRGBLed::blink(int nColorBits, unsigned long nOnMillis,unsigned long nOffMillis) {
    (nColorBits & COLOR_RED_BIT_MASK)   ? RedLED.blink(nOnMillis,nOffMillis)    : RedLED.switchOff();
    (nColorBits & COLOR_GREEN_BIT_MASK) ? GreenLED.blink(nOnMillis,nOffMillis)  : GreenLED.switchOff();
    (nColorBits & COLOR_BLUE_BIT_MASK)  ? BlueLED.blink(nOnMillis,nOffMillis)   : BlueLED.switchOff();
}

void CRGBLed::wave(RGB_COLOR eColor, unsigned long ulFadeInMillis, 
                                     unsigned long ulFadeOutMillis,
                                     unsigned long ulOnMillis,
                                     unsigned long ulOffMillis,
                                     int nMaxLevelInPercent) {
    wave(eColor & 0x7,ulFadeInMillis,ulFadeOutMillis,ulOnMillis,ulOffMillis,nMaxLevelInPercent);
}

void CRGBLed::wave(int nColorBits, unsigned long ulFadeInMillis, unsigned long ulFadeOutMillis, unsigned long ulOnTime, unsigned long ulOffTime, int nMaxLevelInPercent) {
    (nColorBits & COLOR_RED_BIT_MASK)   ? RedLED.wave(ulFadeInMillis,ulFadeOutMillis,ulOnTime,ulOffTime,nMaxLevelInPercent)   : RedLED.switchOff();
    (nColorBits & COLOR_GREEN_BIT_MASK) ? GreenLED.wave(ulFadeInMillis,ulFadeOutMillis,ulOnTime,ulOffTime,nMaxLevelInPercent) : GreenLED.switchOff();
    (nColorBits & COLOR_BLUE_BIT_MASK)  ? BlueLED.wave(ulFadeInMillis,ulFadeOutMillis,ulOnTime,ulOffTime,nMaxLevelInPercent)  : BlueLED.switchOff();
}

void CRGBLed::runSingleColorTests() 
{
    Serial.printf(" - color red...");
    RedLED.runTests();
    Serial.printf(" - color green...");
    GreenLED.runTests();
    Serial.printf(" - color blue...");
    BlueLED.runTests();
}

void CRGBLed::showStartupFlashLight(int nDelayBetweenFlashes) {
    for(int nColorIdx = 0; nColorIdx < 7; nColorIdx++) {
        setColor(nColorIdx);
        delay(nDelayBetweenFlashes);
    }
    switchOff();
}

/// @brief Run the tests for the RGB LED
void CRGBLed::runTests() {
    runSingleColorTests();
    showStartupFlashLight(250);
    switchOff();
}

