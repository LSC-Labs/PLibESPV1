
#include <BatteryMeasure.h>
#include <LSCUtils.h>
#include <math.h>

/**
 * @brief Creates a battery measurement helper for one analog input pin.
 * @param nPin Analog input pin that receives the divided battery voltage.
 * @param fCalcFactor Voltage corresponding to the max. ADC value. This usually
 *        includes the resistor divider ratio.
 */
CBatteryMeasure::CBatteryMeasure(int nPin, float fCalcFactor )
{
    m_nPin = nPin;
    m_fBatteryCalcFactor = fCalcFactor;
    m_oMeasureDelay.start(BATTERY_MEASURE_INTERVAL);
}

/**
 * @brief Reads the ADC (averaged) and stores the result as cached value.
 *
 * Only call from the application loop context.
 * @return Averaged raw ADC value.
 */
int CBatteryMeasure::measure() {
    long nSum = 0;
    for(int nIdx = 0; nIdx < BATTERY_MEASURE_SAMPLES; nIdx++) {
        nSum += analogRead(m_nPin);
    }
    m_nRawValue = (int) (nSum / BATTERY_MEASURE_SAMPLES);
    return(m_nRawValue);
}

/**
 * @brief Measures periodically in the application loop.
 */
void CBatteryMeasure::dispatch() {
    if(m_nRawValue < 0 || m_oMeasureDelay.isDone()) {
        measure();
        m_oMeasureDelay.restart();
    }
}

/**
 * @brief Returns the last measured raw ADC value.
 *
 * If no measurement exists yet, one is taken immediately.
 * @return Raw analogRead() result (averaged).
 */
int CBatteryMeasure::getRawMeasureData() {
    int nRaw = m_nRawValue;
    return(nRaw < 0 ? measure() : nRaw);
}

/**
 * @brief Checks whether the analog input is above the battery-present threshold.
 * @return true when the measured raw value indicates that a battery is present.
 */
bool CBatteryMeasure::isBatteryAvailable() {
    return(getRawMeasureData() > m_nBatteryAvailableThreshold);
}

/**
 * @brief Calculates the battery voltage from the raw ADC reading.
 *
 * If no battery is detected, the method returns 0.0.
 * When nDigits is >= 0, the result is rounded to nDigits decimals.
 *
 * @param nDigits Number of decimal digits to keep. Use -1 to skip rounding.
 * @return Calculated voltage, or 0.0 when no battery is detected.
 */
float CBatteryMeasure::getVoltage(int nDigits) {
    int nRaw = getRawMeasureData();     // one reading for check and calculation
    float fResult = 0.0f;
    if(nRaw > m_nBatteryAvailableThreshold) {
        fResult = ((float) nRaw / (float) (BATTERY_ADC_MAX_VALUE + 1)) * m_fBatteryCalcFactor;
    }
    if(nDigits > -1) {
        if(nDigits > 6) nDigits = 6;
        float fScale = powf(10.0f, (float) nDigits);
        fResult = roundf(fResult * fScale) / fScale;
    }
    return(fResult);
}

#pragma region Interfaces

/**
 * @brief Writes current battery state into a status JSON node.
 * @param oStatusObj Target node receiving voltage and availability values.
 * @param nLevel Status verbosity. Raw ADC data is included from INFO level on.
 */
void CBatteryMeasure::writeStatusTo(JsonNode &oStatusObj, int nLevel) {
    int nRaw = getRawMeasureData();
    oStatusObj["volt"]      = getVoltage(2);
    oStatusObj["available"] = nRaw > m_nBatteryAvailableThreshold;
    if(nLevel >= STATUS_LEVEL_INFO) {
        oStatusObj["raw"]       = nRaw;
    }
}

/**
 * @brief Writes configurable calibration values into a JSON node.
 * @param oConfigData Target configuration node.
 * @param bHideCritical Unused here; no battery configuration value is critical.
 */
void CBatteryMeasure::writeConfigTo(JsonNode &oConfigData, bool bHideCritical) {
    (void) bHideCritical;
    oConfigData["calcFactor"] = this->m_fBatteryCalcFactor;
}

/**
 * @brief Reads calibration values from a JSON node.
 *
 * Invalid factors (<= 0, NaN, infinite) are ignored, the current value is kept.
 * @param oConfigData Source configuration node. Missing values keep defaults.
 */
void CBatteryMeasure::readConfigFrom(JsonNode &oConfigData) {
    float fFactor = m_fBatteryCalcFactor;
    if(oConfigData.storeValueIf("calcFactor", &fFactor)) {
        if(isfinite(fFactor) && fFactor > 0.0f) m_fBatteryCalcFactor = fFactor;
    }
}

/**
 * @brief Adds the battery voltage sensor to the Home Assistant discovery payload.
 */
void CBatteryMeasure::insertComponentDiscovery(const char * pszComponentName, JsonNode & oComponentArea, CMQTTController * pController) {
    (void) pController;
    String strCompID = Appl.getDeviceID();
    strCompID += "-bat-volt";
    char szValueTemplate[256];
    snprintf(szValueTemplate,sizeof(szValueTemplate),"{{ value_json.%s.volt }}",pszComponentName ? pszComponentName : "");
    JsonNode * pSensor = oComponentArea.createObject("bat-volt");
    if(!pSensor) return;
    pSensor->setValue("p",                   "sensor");
    pSensor->setValue("device_class",        "voltage");
    pSensor->setValue("unit_of_measurement", "V");
    pSensor->setValue("suggested_display_precision", 2);
    pSensor->setValue("name",                "Battery Voltage");
    pSensor->setValue("value_template" ,     szValueTemplate);
    pSensor->setValue("unique_id",           strCompID.c_str());
}


#pragma endregion
