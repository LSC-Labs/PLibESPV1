#ifndef DEBUG_LSC_LOGGING
    #undef DEBUGINFOS
#endif

#include <Arduino.h>
#include <JsonNode.h>
#include <Logging.h>
#include <Msgs.h>
#include <DevelopmentHelper.h>

#define LOG_ENTRY_BUFFER_SIZE  512

#pragma region Implementation of CEventLogger

/**
 * @brief Creates a logger without an event handler.
 *
 * Set pEventHandler before writing log messages, otherwise log calls cannot be
 * delivered.
 */
CEventLogger::CEventLogger() { pEventHandler = nullptr;}

/**
 * @brief Creates a logger that publishes log events to an event handler.
 * @param oEventHandler Message bus that receives MSG_LOG_ENTRY events.
 */
CEventLogger::CEventLogger(CEventHandler *oEventHandler) {
    pEventHandler = oEventHandler;
}

/**
 * @brief Converts a log class character to its numeric priority.
 * @param cClass Log class character such as 'I', 'W' or 'E'.
 * @return Index in LOG_CLASS_LEVEL_MASK, or 0 when not found.
 */
int CEventLogger::getLogClassNumberFrom(const char cClass) {
    int nResult = 0;
    const char *strLevelMask = LOG_CLASS_LEVEL_MASK; 
    for(size_t nIdx = 0; nIdx < strlen(strLevelMask); nIdx++) {
        if(strLevelMask[nIdx] == cClass) {
            nResult = nIdx; break;
        }
    }
    return(nResult);
}

/**
 * @brief Formats a flash-string log message and publishes it as an event.
 * @param pszType Log class string. The first character is used.
 * @param pszMessage printf-style flash string.
 */
void CEventLogger::log(const char * pszType, const __FlashStringHelper* pszMessage,...) {
    va_list args;
    va_start(args, (const char*)pszMessage);
    char strBuffer[LOG_ENTRY_BUFFER_SIZE];
    vsnprintf_P(strBuffer, sizeof(strBuffer), (const char *) pszMessage, args);
    va_end(args);
    char cType = pszType == nullptr ? 'I' : pszType[0];
    pEventHandler->sendEvent(this, MSG_LOG_ENTRY, strBuffer,cType );
}

/**
 * @brief Formats a RAM-string log message and publishes it as an event.
 * @param pszType Log class string. The first character is used.
 * @param pszMessage printf-style string.
 */
void CEventLogger::log(const char * pszType, const char *pszMessage, ...) {
    va_list args;
    va_start(args, pszMessage);
    char tBuffer[LOG_ENTRY_BUFFER_SIZE];
    vsnprintf_P(tBuffer, sizeof(tBuffer), pszMessage, args);
    va_end(args);
    char cType = pszType == nullptr ? 'I' : pszType[0];
    pEventHandler->sendEvent(this, MSG_LOG_ENTRY, tBuffer, cType );
}

/**
 * @brief Publishes a JSON log document as an event.
 * @param pszType Log class string. The first character is used.
 * @param pDoc JSON document/node to forward to log writers.
 */
void CEventLogger::log(const char * pszType, JsonNode *pDoc) {
    char cType = pszType == nullptr ? 'I' : pszType[0];
    pEventHandler->sendEvent(this, MSG_LOG_ENTRY_JSON,pDoc, cType );
}
#pragma endregion

#pragma region CLogWriter, Base Class for all Log Writers

/**
 * @brief Gets the current maximum log level.
 * @return Numeric log level threshold.
 */
int CLogWriter::getLogLevel() {
    return(m_nLogLevel);
}

/**
 * @brief Sets the current maximum log level.
 * @param nLogLevel New numeric log level threshold.
 * @return Previous log level.
 */
int CLogWriter::setLogLevel(int nLogLevel) {
    int nOldLevel = m_nLogLevel;
    m_nLogLevel = nLogLevel;
    return(nOldLevel);
}

/**
 * @brief Checks if a log class bypasses the normal log-level threshold.
 * @param cType Log class character.
 * @return true for classes listed in LOG_CLASS_TYPE_ALWAYS.
 */
bool CLogWriter::isTypeToWriteAlways(const char cType) {
    bool bResult = false;
    for(size_t nIdx = 0; nIdx < strlen(LOG_CLASS_TYPE_ALWAYS); nIdx++) {
        if(cType == LOG_CLASS_TYPE_ALWAYS[nIdx]) {
            bResult = true; break;
        }
    }
    return(bResult);
}

/**
 * @brief Checks if a log class should be written at the current log level.
 * @param cType Log class character.
 * @return true when the class priority is within the configured threshold.
 */
bool CLogWriter::isLogLevelToWrite(const char cType) {
    bool bResult = false;
    int nClass = CEventLogger::getLogClassNumberFrom(cType);
    if(nClass >= 0 && nClass <= m_nLogLevel) {
        bResult = true;
    }
    return(bResult);
}

/**
 * @brief Hook for writing JSON log entries.
 *
 * The base implementation intentionally does nothing. Derived writers override
 * this when they support structured log output.
 */
void CLogWriter::writeLogEntry(const char *pszType, JsonNode *pDoc) {
}

/**
 * @brief Hook for writing text log entries.
 *
 * The base implementation intentionally does nothing. Derived writers override
 * this to write to Serial, streams, files, or other sinks.
 */
void CLogWriter::writeLogEntry(const char *pszType, const char *pszMessage) {
}

/**
 * @brief Receives log events and forwards accepted entries to writer hooks.
 * @param pSender Sender of the event.
 * @param nMsgType MSG_LOG_ENTRY or MSG_LOG_ENTRY_JSON.
 * @param pMessage Text pointer or JsonNode pointer depending on nMsgType.
 * @param nClass Log class character passed as an int.
 * @return EVENT_MSG_RESULT_OK after processing.
 */
int CLogWriter::receiveEvent(const void *pSender, int nMsgType, const void *pMessage, int nClass) {
    if(nMsgType == MSG_LOG_ENTRY || nMsgType == MSG_LOG_ENTRY_JSON) {
        String strType = "";
        char cLogClass = nClass;
        if(isLogLevelToWrite(cLogClass) || isTypeToWriteAlways(cLogClass)) {
            strType = cLogClass;
            if(nMsgType == MSG_LOG_ENTRY) {
                writeLogEntry(strType.c_str(),(const char *) pMessage);
            } 
            else if (nMsgType == MSG_LOG_ENTRY_JSON) {
                writeLogEntry(strType.c_str(),(JsonNode *)(pMessage));
            }
        }
    }
    return(EVENT_MSG_RESULT_OK);
}

#pragma endregion

/**
 * @brief Creates a log writer for an Arduino Stream.
 * @param pStream Stream destination, for example Serial.
 */
CStreamLogWriter::CStreamLogWriter(Stream *pStream) {
    this->pStream = pStream;
}

/**
 * @brief Writes a formatted text log line to the stream.
 * @param pszType Log class string.
 * @param pszMessage Message text.
 */
void CStreamLogWriter::writeLogEntry(const char *pszType, const char *pszMessage) {
    if(pStream) {
        pStream->printf("[%s] %s\n", pszType, pszMessage);
    }
}

/**
 * @brief Writes a serialized JSON log entry to the stream.
 * @param pszType Log class string.
 * @param pDoc JSON node to serialize.
 */
void CStreamLogWriter::writeLogEntry(const char *pszType, JsonNode *pDoc) {
    if(pStream) {
        pStream->write(pDoc->getAsJsonText());
        pStream->println();
    }
}



#pragma region Serial Log Writer, Implementation of CLogWriter
/*
void CSerialLogWriter::writeLogEntry(const char *pszType, const char *pszMessage) {
    Serial.printf("%s%s\n", pszType, pszMessage);
}

void CSerialLogWriter::writeLogEntry(const char *strType, JsonDocument *pDoc) {
    DEBUG_FUNC_START();
    serializeJsonPretty(*pDoc,Serial);
}
*/
#pragma endregion
