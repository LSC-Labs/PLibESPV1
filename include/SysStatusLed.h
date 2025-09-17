#pragma once

#include <RGBLed.h>
#include <EventHandler.h>

class CSysStatusLed : public CRGBLed, public IMsgEventReceiver {

    bool isRebootRequested = false;
    bool isButtonPressed = false;
    bool isInAccessPointMode = false;
    bool isInStationMode = false;
    bool isWiFiConnected = false;

    public:
        CSysStatusLed(int nRedPin, int nGreenPin, int nBluePin, bool bActiveLow = true );

        int receiveEvent(const void * pSender, int nMsgId, const void * pMessage, int nType);
        void updateLED();
};
