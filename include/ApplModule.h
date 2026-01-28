#pragma once
#include <Appl.h>

/**
 * @brief Abstract Basic Module Implementation
 * You have to implement the dispatch method to reflect your module logic
 * Implement the configuration and status handling
 */
class ApplModule: public IModule {

    public:
        /**
         * Default dispatch function by doing nothing.
         * Will bi called by default when Appl.dispatch() is used in main loop
         * Implement your logic by overwriting. 
         */
        virtual void dispatch();

        /**
         * Default Event Receiver,
         * is calling the dispatch function on Application loop.
         */
        virtual int receiveEvent(const void * pSender, int nMsg, const void * pMessage, int nClass) override {
            switch(nMsg) {
                case MSG_APPL_LOOP: dispatch(); break;
            }
            return EVENT_MSG_RESULT_OK;
        };

};