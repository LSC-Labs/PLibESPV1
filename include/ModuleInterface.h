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

