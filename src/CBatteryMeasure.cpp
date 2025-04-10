#include <BatteryMeasure.h>
#include <LSCUtils.h>

/// @brief Constructor
/// @param nPin Pin to monitor
/// @param fCalcFactor factor to calculate the battery voltage from raw data.
CBatteryMeasure::CBatteryMeasure(int nPin, float fCalcFactor ) 
{
    m_nPin = nPin;
    m_fBatteryCalcFactor = fCalcFactor;
}

/// @brief is Batter available ? Needs at least some voltage on the inputpin
/// @return false => seems no battery inserted... true => battery in place
bool CBatteryMeasure::isBatteryAvailable() {
    return(analogRead(m_nPin) > m_nBatteryAvailableThreshold);
}

int CBatteryMeasure::getRawMeasureData() {
    return(analogRead(m_nPin));
}

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

void CBatteryMeasure::writeStatusTo(JsonObject &oStatusObj) {
    oStatusObj["power"]     = getVoltage(2);
    oStatusObj["available"] = isBatteryAvailable();
    oStatusObj["raw"]       = getRawMeasureData();
}

void CBatteryMeasure::writeConfigTo(JsonObject &oConfigData, bool bHideCritical) {
    oConfigData["calcFactor"] = this->m_fBatteryCalcFactor;
}
void CBatteryMeasure::readConfigFrom(JsonObject &oConfigData) {
    LSC::setValue( &m_fBatteryCalcFactor,oConfigData["calcFactor"]);
}

#pragma endregion