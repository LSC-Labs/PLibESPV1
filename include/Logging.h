#pragma once

#include <stdarg.h>
#include <EventHandler.h>
#include <Msgs.h>

#define logInfo(s,...)      log("I",s, ##__VA_ARGS__)
#define logVerbose(s,...)   log("V",s, ##__VA_ARGS__)
#define logWarning(s,...)   log("W",s, ##__VA_ARGS__)
#define logError(s,...)     log("E",s, ##__VA_ARGS__)
#define logTrace(s,...)     log("T",s, ##__VA_ARGS__)
#define logException(s,...) log("X",s, ##__VA_ARGS__)
#define logDebug(s,...)     log("D",s, ##__VA_ARGS__)

// Definition of the Keys in a string for quick access...
// Index is defined Level
#define LOG_CLASS_LEVEL_MASK  ".IV..TWEDX"
#define LOG_CLASS_TYPE_ALWAYS "WEX"  


class CEventLogger {
    private:
        CEventHandler * pEventHandler;
    public:
        CEventLogger();
        CEventLogger(CEventHandler * pEventHandler);

        static int  getLogClassNumberFrom(const char cClass);
        static char getClassCharFromLogClass(int nClassNo);

        void log(const char* pszType, const __FlashStringHelper* pszMessage, ...);
        void log(const char* pszType, const char *pszMessage, ...);
        void log(const char* strType, JsonDocument *pDoc);
};


/// @brief Class to handle the logging for a module
///      This class is used to write log entries for a specific module
class CLogWriter : public IMsgEventReceiver {
    
    private:
        void *pSender;
        CEventHandler *pEventHandler;
        const char* m_strModuleName;
        char m_nLogLevel = 3; 
        bool isTypeToWriteAlways(const char cType);
        bool isLogLevelToWrite(const char cType);
    public:
        CLogWriter(){};
        int setLogLevel(int nLogLevel);
        int getLogLevel();
        virtual int  receiveEvent(const void * pSender, int nMsgType, const void * pMessage, int nClass) override;   

        virtual void writeLogEntry(const char *strType, const char *strMessage);
        virtual void writeLogEntry(const char *strType, JsonDocument *oDoc);
};

class CStreamLogWriter : public CLogWriter {
    private:
        Stream *pStream;
    public:
        CStreamLogWriter(Stream *pStream);
        void writeLogEntry(const char *strType, const char *strMessage) override;
        void writeLogEntry(const char *strType, JsonDocument *pDoc) override;
};


class CSerialLogWriter : public CStreamLogWriter {
    public:
        CSerialLogWriter() : CStreamLogWriter(&Serial){}
};
