#pragma once
#include <Arduino.h>

/**
 * @brief Simple delay class to avoid DOS calls to functions.
 *    Should avoid to cal services too often (e.g. web requests).
 * 
 *       Usage:
 *       CDelay oDelay(5000); // 5 seconds delay
 *       if(oDelay.isDone()) { ... ; oDelay.restart(); }
 * 
 *     As an alternative, you can use start(nDelayMillis) to set a new delay time.
 *     Use reset() to stop the delay.
 * 
 */
class CSimpleDelay {
        unsigned long nStartMillis = 0;
        unsigned long nDelayMillis = 0;
    public:
        CSimpleDelay() {};
        CSimpleDelay(unsigned long nDelayMillis) { start(nDelayMillis); };
        ~CSimpleDelay() {};   

        /**
         * @brief Starts the delay countdown. If nDelayMillis is 0, the previous value is used.
         * @param nDelayMillis New delay time in milliseconds (if 0, the previous value is used).
         * @param bStartNow If true, the delay starts immediately. If false, the delay starts after nDelayMillis (default: true). 
         *                  This is useful to avoid immediate calls after start.
         */
        void start(unsigned long nDelayMillis, bool bStartNow = true ) {
            if(nDelayMillis > 0) this->nDelayMillis = nDelayMillis;
            nStartMillis = bStartNow ? 0 : millis();
        }
        /**
         * @brief Restarts the delay with the previous set delay time.
        */
        unsigned long restart() { nStartMillis = millis();  return(nStartMillis); }

        /**
         * @brief Checks if the delay time has elapsed.
         * 
         */
        bool isDone() {
            if(nStartMillis == 0) return(true);
            return( (nStartMillis + nDelayMillis) < millis());
        }

        /**
         * @brief Resets the delay (stops it).
         */
        void reset() { nStartMillis = 0; nDelayMillis = 0;}
       
        /**
         * @brief Get the Elapsed time since start in milliseconds.
         */
        unsigned long getElapsed() { return(millis() - nStartMillis); }

        /**
         * @brief Get the Remaining time until the delay is done in milliseconds.
         */
        unsigned long getRemaining() { 
            if(nStartMillis == 0) return(0);
            unsigned long nEnd = nStartMillis + nDelayMillis;
            if(nEnd > millis()) return(nEnd - millis());
            return(0); 
        }

        void printDiag() {
            Serial.printf("Delay: start=%lu, delay=%lu, elapsed=%lu, remaining=%lu\n",
                nStartMillis,nDelayMillis,getElapsed(),getRemaining());
        }
};
