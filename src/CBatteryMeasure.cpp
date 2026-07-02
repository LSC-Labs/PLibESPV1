
#include <BatteryMeasure.h>
#include <LSCUtils.h>

/**
 * @brief Creates a battery measurement helper for one analog input pin.
 * @param nPin Analog input pin that receives the divided battery voltage.
 * @param fCalcFactor Factor used to convert the ADC ratio into the original
 *        battery voltage. This usually includes the resistor divider ratio.
 */
CBatteryMeasure::CBatteryMeasure(int nPin, float fCalcFactor ) 
{
    m_nPin = nPin;
    m_fBatteryCalcFactor = fCalcFactor;
}

/**
 * @brief Checks whether the analog input is above the battery-present threshold.
 * @return true when the measured raw value indicates that a battery is present.
 */
bool CBatteryMeasure::isBatteryAvailable() {
    return(analogRead(m_nPin) > m_nBatteryAvailableThreshold);
}

/**
 * @brief Reads the raw ADC value from the configured pin.
 * @return Raw analogRead() result.
 */
int CBatteryMeasure::getRawMeasureData() {
    return(analogRead(m_nPin));
}

/**
 * @brief Calculates the battery voltage from the raw ADC reading.
 *
 * If no battery is detected, the method returns negative zero to signal "not
 * available" while still keeping the return type numeric. When nDigits is >= 0,
 * the result is rounded by formatting and parsing the value again.
 *
 * @param nDigits Number of decimal digits to keep. Use -1 to skip rounding.
 * @return Calculated voltage, or -0.0 when no battery is detected.
 */
float CBatteryMeasure::getVoltage(int nDigits) {
    float fResult = -1.0;
    if(isBatteryAvailable()) {
        fResult = (getRawMeasureData()/1024.0) * m_fBatteryCalcFactor;
    } else {
        fResult = -0.0;
    }
    if(nDigits > -1) {
        int nCharsNeeded = 1 + snprintf(NULL, 0, "%.*f", nDigits, fResult);
        char *pszBuffer = (char *) malloc(nCharsNeeded);
        snprintf(pszBuffer, nCharsNeeded, "%.*f", nDigits, fResult);
        fResult = atof(pszBuffer);
        free(pszBuffer);
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
    oStatusObj["volt"]      = getVoltage(2);
    oStatusObj["available"] = isBatteryAvailable();
    if(nLevel >= STATUS_LEVEL_INFO) {
        oStatusObj["raw"]       = getRawMeasureData();
    }
}

/**
 * @brief Writes configurable calibration values into a JSON node.
 * @param oConfigData Target configuration node.
 * @param bHideCritical Unused here; no battery configuration value is critical.
 */
void CBatteryMeasure::writeConfigTo(JsonNode &oConfigData, bool bHideCritical) {
    oConfigData["calcFactor"] = this->m_fBatteryCalcFactor;
}

/**
 * @brief Reads calibration values from a JSON node.
 * @param oConfigData Source configuration node. Missing values keep defaults.
 */
void CBatteryMeasure::readConfigFrom(JsonNode &oConfigData) {
    oConfigData.storeValueIf("calcFactor", &m_fBatteryCalcFactor);
}

#pragma endregion
