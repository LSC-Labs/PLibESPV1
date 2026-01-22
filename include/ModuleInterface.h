#pragma once
/**
 * @file ModuleInterface.h
 * @author LSC-Labs - P.Liebl
 * 
 * @brief Full Interface for a module and an abstract basic module implementation
 */



#include <ConfigHandler.h>
#include <StatusHandler.h>
#include <EventHandler.h>
#include <Msgs.h>

/**
 * @brief Full Interface for modules
 * - Configuration Handling
 * - Status Handling
 * - Event Handling
 */
class IModule : public IConfigHandler, public IStatusHandler, public IMsgEventReceiver {
  
};

/**
 * @brief Abstract Basic Module Implementation
 * You have to override the dispatch method to implement your module logic
 * Implement the configuration and status handling
 */
class AModule: public IModule {

    public:
        virtual void dispatch();
        virtual int receiveEvent(const void * pSender, int nMsg, const void * pMessage, int nClass) override {
            switch(nMsg) {
                case MSG_APPL_LOOP: dispatch(); break;
            }
            return EVENT_MSG_RESULT_OK;
        };
};
