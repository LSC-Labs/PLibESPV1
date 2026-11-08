#pragma once
#include <Runtime.h>

/**
 * @brief Simple delay class to avoid DOS calls to functions.
 *    Should avoid to call services too often (e.g. web requests).
 *
 *       Usage:
 *       CSimpleDelay oDelay(5000); // 5 seconds delay
 *       if(oDelay.isDone()) { ... ; oDelay.restart(); }
 *
 *     As an alternative, you can use start(nDelayMillis) to set a new delay time.
 *     Use stop() to stop the delay, reset() to stop it and clear the delay time.
 *
 * All time calculations use the elapsed time (millis() - start), so they stay
 * correct when millis() overflows (every ~49.7 days).
 */
class CSimpleDelay {
    public:
        /// @brief Time source (default: millis()) - replaceable for tests.
        typedef unsigned long (*TTimeSource)();

    private:
        unsigned long m_ulStartMillis = 0;
        unsigned long m_ulDelayMillis = 0;
        bool m_bIsActive  = false;  // running (separate flag - millis() may be 0 at start)
        bool m_bIsExpired = false;  // set by setExpired()

        static unsigned long defaultTimeSource() { return(millis()); }
        static TTimeSource & timeSource() {
            static TTimeSource s_fnTimeSource = &CSimpleDelay::defaultTimeSource;
            return(s_fnTimeSource);
        }
        static unsigned long now() { return(timeSource()()); }

    public:
        CSimpleDelay() {};
        CSimpleDelay(unsigned long nDelayMillis) { start(nDelayMillis); };
        ~CSimpleDelay() {};

        /// @brief Replace the time source (tests). nullptr restores millis().
        static void setTimeSource(TTimeSource fnTimeSource) {
            timeSource() = fnTimeSource ? fnTimeSource : &CSimpleDelay::defaultTimeSource;
        }

        /**
         * @brief Starts the delay countdown. If ulDelayMillis is 0, the previous value is used.
         * @param ulDelayMillis New delay time in milliseconds (if 0, the previous value is used).
         * @param bStartNow true (default): the delay runs from now.
         *                  false: the delay time is only set, the delay is not running
         *                  (isDone() is false until restart() - or setExpired() for an
         *                  immediate first run).
         */
        CSimpleDelay * start(unsigned long ulDelayMillis, bool bStartNow = true ) {
            if(ulDelayMillis > 0) this->m_ulDelayMillis = ulDelayMillis;
            m_ulStartMillis = bStartNow ? now() : 0;
            m_bIsActive  = bStartNow;
            m_bIsExpired = false;
            return(this);
        }

        /**
         * @brief Sets the delay time exactly (also 0 - unlike start()), the running state is kept.
         */
        void setDelay(unsigned long ulDelayMillis) { m_ulDelayMillis = ulDelayMillis; }
        /// @brief Gets the delay time in milliseconds.
        unsigned long getDelay() { return(m_ulDelayMillis); }

        /**
         * @brief Sets the delay time exactly and (re)starts the delay now.
         * @return End time (millis() value, may wrap around).
         */
        unsigned long restartWith(unsigned long ulDelayMillis) { m_ulDelayMillis = ulDelayMillis; return(restart()); }

        /**
         * @brief Stops the delay (the delay time is kept for restart()).
         */
        void stop() {
            m_ulStartMillis = 0;
            m_bIsActive  = false;
            m_bIsExpired = false;
        }

        /**
         * @brief Resets the delay (stops it and clears the delay time).
         */
        void reset() { stop(); m_ulDelayMillis = 0; }

        /**
         * @brief Checks if the delay is active (running).
         */
        bool isActive() { return(m_bIsActive); }

        /**
         * @brief Restarts the delay with the previous set delay time.
         * @return End time (millis() value, may wrap around).
         */
        unsigned long restart() {
            m_bIsExpired = false;
            m_bIsActive  = true;
            m_ulStartMillis = now();
            return(m_ulStartMillis + m_ulDelayMillis);
        }

        /**
         * @brief Advances to the next delay period without resetting the start time.
         *
         * Keeps a constant interval even if the check is late: the start time is
         * moved by whole periods, so the next end time lies in the future.
         * Missed periods are skipped (not caught up).
         * @return End time of the current period (millis() value, may wrap around).
         */
        unsigned long next() {
            m_bIsExpired = false;
            if(m_bIsActive && m_ulDelayMillis > 0) {
                unsigned long ulElapsed = now() - m_ulStartMillis;
                if(ulElapsed >= m_ulDelayMillis) {
                    // whole periods - no loop (delay 0 would loop forever)
                    m_ulStartMillis += (ulElapsed / m_ulDelayMillis) * m_ulDelayMillis;
                }
            }
            return(m_ulStartMillis + m_ulDelayMillis);
        }

        /**
         * Set the delay to expired (isDone() returns true, without
         * reseting the start time). This enables to getRemaining()
         * and other time dependend infos
         */
        void setExpired() {
            m_bIsExpired = true;
        }

        /**
         * checks if the delay is expired
         * (stopped by an explicit setExpired() - not, if the delay isDone())
         */
        bool isExpired() { return(m_bIsExpired); }

        /**
         * @brief Checks if the delay time has elapsed,
         * or the user has stopped the delay with setExpired()
         */
        bool isDone() {
            if(m_bIsExpired) return(true);
            if(!m_bIsActive) return(false);
            return(now() - m_ulStartMillis >= m_ulDelayMillis);
        }

        /**
         * @brief Get the Elapsed time since start in milliseconds (0 if not active).
         */
        unsigned long getElapsed() { return(m_bIsActive ? now() - m_ulStartMillis : 0); }

        /**
         * @brief Get the Remaining time until the delay is done in milliseconds.
         */
        unsigned long getRemaining() {
            if(!m_bIsActive) return(0);
            unsigned long ulElapsed = now() - m_ulStartMillis;
            return(ulElapsed < m_ulDelayMillis ? m_ulDelayMillis - ulElapsed : 0);
        }

        void printDiag() {
            Serial.printf("Delay: start=%lu, delay=%lu, elapsed=%lu, remaining=%lu\n",
                m_ulStartMillis,m_ulDelayMillis,getElapsed(),getRemaining());
        }
};
