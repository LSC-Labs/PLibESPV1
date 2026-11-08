#pragma once
#include <ApplModule.h>
#include <MQTTController.h>
#include <SimpleDelay.h>

// Max. raw value of analogRead(): ESP8266 10 bit, ESP32 12 bit (default resolution)
#ifndef BATTERY_ADC_MAX_VALUE
    #ifdef ESP32
        #define BATTERY_ADC_MAX_VALUE 4095
    #else
        #define BATTERY_ADC_MAX_VALUE 1023
    #endif
#endif

// Interval between two measurements in the application loop (ms)
#ifndef BATTERY_MEASURE_INTERVAL
    #define BATTERY_MEASURE_INTERVAL 5000
#endif

// Number of ADC samples averaged per measurement
#ifndef BATTERY_MEASURE_SAMPLES
    #define BATTERY_MEASURE_SAMPLES 4
#endif

/**
 * @brief Measures a battery voltage on an analog pin.
 *
 * The ADC is only read in the application loop (dispatch()). Status, config
 * and Home Assistant callbacks - which may run in the async web/socket context -
 * only use the cached value. This also avoids frequent ADC reads on the ESP8266,
 * which disturb the WiFi connection.
 */
class CBatteryMeasure : public ApplModule, public IHomeAssistantComponent {
    private:
        int   m_nPin;
        // Threshold relative to the ADC range (~10% of 1023 on ESP8266, scaled on ESP32)
        int   m_nBatteryAvailableThreshold = (BATTERY_ADC_MAX_VALUE + 1) / 10;
        float m_fBatteryCalcFactor = 0.0;
        volatile int  m_nRawValue  = -1;     // last measured raw value, -1 = not measured yet
        CSimpleDelay  m_oMeasureDelay;

        int   measure();

    public:
        /// @brief Battery measure constructor
        /// @param nPin Pin to monitor
        /// @param fCalcFactor Voltage that corresponds to the max. ADC value
        ///        (includes the resistor divider, i.E. 4.2 for a 3.7V LiIon cell)
        CBatteryMeasure(int nPin, float fCalcFactor = 4.2);

        bool  isBatteryAvailable();
        int   getRawMeasureData();
        float getVoltage(int nDigits = -1);

        /// @brief Measures periodically in the application loop.
        void  dispatch() override;

        // Implementation of the Interfaces
        void  writeStatusTo( JsonNode &oStatusObj, int nLevel)         override;
        void  writeConfigTo( JsonNode &oConfigObj, bool bHideCritical) override;
        void  readConfigFrom(JsonNode &oConfigObj)                     override;

        void insertComponentDiscovery(const char * pszComponentName, JsonNode & oComponentArea, CMQTTController * pController) override;
};
