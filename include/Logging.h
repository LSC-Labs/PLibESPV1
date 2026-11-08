#pragma once

#include <stdarg.h>
#include <EventHandler.h>
#include <Msgs.h>

#ifndef LOG_ENTRY_BUFFER_SIZE
    #define LOG_ENTRY_BUFFER_SIZE   512     ///< max. length of a formatted log entry
#endif

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


/**
 * @brief Logger facade that sends formatted log events through CEventHandler.
 *
 * Log calls from other contexts (AsyncTCP task on ESP32, SYS context on
 * ESP8266 - i.e. web server, WebSocket, MQTT callbacks) are queued on the
 * message bus (postEventText()) and written in the loop - the log writers are
 * not thread safe.
 */
class CEventLogger {
    private:
        CEventHandler * pEventHandler = nullptr;

        /// @brief Send (or queue) one formatted entry.
        void publish(char cType, const char *pszText);

    public:
        /// @brief Create a logger without an attached event handler.
        CEventLogger();
        /// @brief Create a logger that publishes to the given event handler.
        CEventLogger(CEventHandler * pEventHandler);
        /// @brief Set the event handler (message bus) the entries are sent to.
        void setEventHandler(CEventHandler * pEventHandler) { this->pEventHandler = pEventHandler; }

        /// @brief Convert a log class character into its numeric message class.
        static int  getLogClassNumberFrom(const char cClass);
        /// @brief Convert a numeric message class back to its log class character.
        static char getClassCharFromLogClass(int nClassNo);

        /// @brief Format and publish a flash-string log message.
        void log(const char* pszType, const __FlashStringHelper* pszMessage, ...);
        /// @brief Format and publish a C-string log message.
        void log(const char* pszType, const char *pszMessage, ...);
        /// @brief Publish a JSON document as log payload.
        void log(const char* strType, JsonNode *pDoc);
};


/// @brief Class to handle the logging for a module
///      This class is used to write log entries for a specific module
class CLogWriter : public IMsgEventReceiver {
    
    private:
        int  m_nLogLevel = 3;       // index in LOG_CLASS_LEVEL_MASK: I=1, V=2 ... (W/E/X always)
        bool isTypeToWriteAlways(const char cType);
        bool isLogLevelToWrite(const char cType);
    public:
        CLogWriter(){};
        /// @brief Set the maximum log level written by this writer.
        int setLogLevel(int nLogLevel);
        /// @brief Return the currently configured maximum log level.
        int getLogLevel();
        /// @brief Receive log events from the message bus and write matching entries.
        virtual int  receiveEvent(const void * pSender, int nMsgType, const void * pMessage, int nClass) override;   

        /// @brief Write a text log entry to the concrete output.
        virtual void writeLogEntry(const char *strType, const char *strMessage);
        /// @brief Write a JSON log entry to the concrete output.
        virtual void writeLogEntry(const char *strType, JsonNode *oDoc);
};

/// @brief Log writer that prints log entries to an Arduino Stream.
class CStreamLogWriter : public CLogWriter {
    private:
        Stream *pStream;
    public:
        /// @brief Create a log writer for the given stream.
        CStreamLogWriter(Stream *pStream);
        /// @brief Write a text log entry to the configured stream.
        void writeLogEntry(const char *strType, const char *strMessage) override;
        /// @brief Write a JSON log entry to the configured stream.
        void writeLogEntry(const char *strType, JsonNode *pDoc) override;
};


/// @brief Stream log writer preconfigured for Serial.
class CSerialLogWriter : public CStreamLogWriter {
    public:
        CSerialLogWriter() : CStreamLogWriter(&Serial){}
};
