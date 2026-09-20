#pragma once
#include <ApplModule.h>
#include <MQTTController.h>

class CBatteryMeasure : public ApplModule, public IHomeAssistantComponent {
    private:
        int   m_nPin;
        int   m_nBatteryAvailableThreshold = 100;
        float m_fBatteryCalcFactor = 0.0;

    public:
        /// @brief Battery measure constructor
        /// @param nPin Pin to monitor
        /// @param fCalcFactor factor to calculate the battery voltage (i.E. 4.4 for 3.7 LiOn)
        CBatteryMeasure(int nPin, float fCalcFactor = 4.2);

        bool  isBatteryAvailable();
        int   getRawMeasureData();
        float getVoltage(int nDigits = -1);
        
        // Implementation of the Interfaces
        void  writeStatusTo( JsonNode &oStatusObj, int nLevel)         override;
        void  writeConfigTo( JsonNode &oConfigObj, bool bHideCritical) override;
        void  readConfigFrom(JsonNode &oConfigObj)                     override;

        void insertComponentDiscovery(const char * pszComponentName, JsonNode & oComponentArea, CMQTTController * pController) override;    
};

