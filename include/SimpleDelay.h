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
        unsigned long m_ulStartMillis = 0;
        unsigned long m_ulDelayMillis = 0;
        bool m_bIsExpired = false;  // First start Flag..
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
        CSimpleDelay * start(unsigned long ulDelayMillis, bool bStartNow = true ) {
            if(ulDelayMillis > 0) this->m_ulDelayMillis = ulDelayMillis;
            m_ulStartMillis = bStartNow ? millis() : 0;
            m_bIsExpired = false;
            return(this);
        }
        void stop() {
            m_ulStartMillis = 0;
            m_bIsExpired = false;
        }
        /**
         * @brief Resets the delay (stops it).
         */
        void reset() { m_ulStartMillis = 0; m_ulDelayMillis = 0; m_bIsExpired = false;}
       
        bool isActive() { return(m_ulStartMillis > 0); }
        /**
         * @brief Restarts the delay with the previous set delay time.
        */
        unsigned long restart() { 
            m_bIsExpired = false;
            m_ulStartMillis = millis();  
            return(m_ulStartMillis); 
        }

        void setExpired() {
            m_bIsExpired = true;
        }

        /**
         * @brief Checks if the delay time has elapsed.
         * 
         */
        bool isDone() {
            if(m_bIsExpired) return(true);
            if(m_ulStartMillis == 0) return(false);
            return( (m_ulStartMillis + m_ulDelayMillis) < millis());
        }

       
        /**
         * @brief Get the Elapsed time since start in milliseconds.
         */
        unsigned long getElapsed() { return(millis() - m_ulStartMillis); }

        /**
         * @brief Get the Remaining time until the delay is done in milliseconds.
         */
        unsigned long getRemaining() { 
            if(m_ulStartMillis == 0) return(0);
            unsigned long nEnd = m_ulStartMillis + m_ulDelayMillis;
            if(nEnd > millis()) return(nEnd - millis());
            return(0); 
        }

        void printDiag() {
            Serial.printf("Delay: start=%lu, delay=%lu, elapsed=%lu, remaining=%lu\n",
                m_ulStartMillis,m_ulDelayMillis,getElapsed(),getRemaining());
        }
};
