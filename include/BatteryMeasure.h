#pragma once
#include <ConfigHandler.h>
#include <StatusHandler.h>

class CBatteryMeasure : public IConfigHandler, public IStatusHandler {
    private:
        int   m_nPin;
        int   m_nBatteryAvailableThreshold = 100;
        float m_fBatteryCalcFactor = 0.0;

    public:
        /// @brief Battery measure constructor
        /// @param nPin Pin to monitor
        /// @param fCalcFactor factor to calculate the battery voltage (i.E. 4.4 for 3.7 LiOn)
        CBatteryMeasure(int nPin, float fCalcFactor = 0.0);

        bool  isBatteryAvailable();
        int   getRawMeasureData();
        float getVoltage(int nDigits = -1);
        
        // Implementation of the Interfaces
        void  writeStatusTo( JsonObject &oStatusObj)                     override;
        void  writeConfigTo( JsonObject &oConfigObj, bool bHideCritical) override;
        void  readConfigFrom(JsonObject &oConfigObj)                     override;
};

