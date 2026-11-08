#ifndef DEBUG_LSC_LOGGING
    #undef DEBUGINFOS
#endif

#include <Arduino.h>
#include <JsonNode.h>
#include <Logging.h>
#include <Msgs.h>
#include <DevelopmentHelper.h>

#include <ctype.h>

#pragma region Implementation of CEventLogger

/**
 * @brief Creates a logger without an event handler.
 *
 * Set the event handler (setEventHandler()) before writing log messages,
 * otherwise log calls are ignored.
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
 * @param cClass Log class character such as 'I', 'W' or 'E' (case insensitive).
 * @return Index in LOG_CLASS_LEVEL_MASK, or 0 when not found.
 */
int CEventLogger::getLogClassNumberFrom(const char cClass) {
    int nResult = 0;
    const char cUpper = (char) toupper((unsigned char) cClass);
    const char *strLevelMask = LOG_CLASS_LEVEL_MASK; 
    for(size_t nIdx = 0; strLevelMask[nIdx]; nIdx++) {
        if(cUpper != '.' && strLevelMask[nIdx] == cUpper) {
            nResult = (int) nIdx; break;
        }
    }
    return(nResult);
}

/**
 * @brief Converts a numeric log class back to its character.
 * @param nClassNo Index in LOG_CLASS_LEVEL_MASK.
 * @return Log class character, 'I' for unknown/unused indexes.
 */
char CEventLogger::getClassCharFromLogClass(int nClassNo) {
    const char *strLevelMask = LOG_CLASS_LEVEL_MASK;
    if(nClassNo < 0 || nClassNo >= (int) strlen(strLevelMask) || strLevelMask[nClassNo] == '.') return('I');
    return(strLevelMask[nClassNo]);
}

/**
 * @brief Sends one entry to the bus - queued, if called from another context.
 */
void CEventLogger::publish(char cType, const char *pszText) {
    if(!pEventHandler || !pszText) return;
    if(CEventHandler::isLoopContext()) {
        pEventHandler->sendEvent(this, MSG_LOG_ENTRY, pszText, cType);
    } else {
        pEventHandler->postEventText(this, MSG_LOG_ENTRY, pszText, cType);
    }
}

/// @brief Marks a truncated entry with "..." at the end.
static void markTruncated(char *pszBuffer, size_t nBufferSize, int nNeeded) {
    if(nNeeded >= (int) nBufferSize && nBufferSize > 4) {
        strcpy(&pszBuffer[nBufferSize - 4], "...");
    }
}

/**
 * @brief Formats a flash-string log message and publishes it as an event.
 * @param pszType Log class string. The first character is used.
 * @param pszMessage printf-style flash string.
 */
void CEventLogger::log(const char * pszType, const __FlashStringHelper* pszMessage,...) {
    if(!pEventHandler || !pszMessage) return;
    char strBuffer[LOG_ENTRY_BUFFER_SIZE];
    va_list args;
    va_start(args, pszMessage);
    int nNeeded = vsnprintf_P(strBuffer, sizeof(strBuffer), (const char *) pszMessage, args);
    va_end(args);
    markTruncated(strBuffer, sizeof(strBuffer), nNeeded);
    publish(pszType == nullptr ? 'I' : pszType[0], strBuffer);
}

/**
 * @brief Formats a RAM-string log message and publishes it as an event.
 * @param pszType Log class string. The first character is used.
 * @param pszMessage printf-style string.
 */
void CEventLogger::log(const char * pszType, const char *pszMessage, ...) {
    if(!pEventHandler || !pszMessage) return;
    char tBuffer[LOG_ENTRY_BUFFER_SIZE];
    va_list args;
    va_start(args, pszMessage);
    // vsnprintf_P: on ESP8266 the format may also be in flash (PSTR) - works for RAM too
    int nNeeded = vsnprintf_P(tBuffer, sizeof(tBuffer), pszMessage, args);
    va_end(args);
    markTruncated(tBuffer, sizeof(tBuffer), nNeeded);
    publish(pszType == nullptr ? 'I' : pszType[0], tBuffer);
}

/**
 * @brief Publishes a JSON log document as an event.
 *
 * From another context the document is copied (queued) - the caller's node
 * does not exist anymore when the entry is written.
 *
 * @param pszType Log class string. The first character is used.
 * @param pDoc JSON document/node to forward to log writers.
 */
void CEventLogger::log(const char * pszType, JsonNode *pDoc) {
    if(!pEventHandler || !pDoc) return;
    char cType = pszType == nullptr ? 'I' : pszType[0];
    if(CEventHandler::isLoopContext()) {
        pEventHandler->sendEvent(this, MSG_LOG_ENTRY_JSON, pDoc, cType);
    } else {
        pEventHandler->postEventJson(this, MSG_LOG_ENTRY_JSON, *pDoc, cType);
    }
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
    if(nLogLevel < 0) nLogLevel = 0;
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
    const char cUpper = (char) toupper((unsigned char) cType);
    return(cUpper != 0 && strchr(LOG_CLASS_TYPE_ALWAYS, cUpper) != nullptr);
}

/**
 * @brief Checks if a log class should be written at the current log level.
 * @param cType Log class character.
 * @return true when the class priority is within the configured threshold.
 */
bool CLogWriter::isLogLevelToWrite(const char cType) {
    int nClass = CEventLogger::getLogClassNumberFrom(cType);
    return(nClass <= m_nLogLevel);
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
    (void) pSender;
    if((nMsgType == MSG_LOG_ENTRY || nMsgType == MSG_LOG_ENTRY_JSON) && pMessage) {
        char cLogClass = (char) nClass;
        if(isLogLevelToWrite(cLogClass) || isTypeToWriteAlways(cLogClass)) {
            char szType[2] = { cLogClass, '\0' };
            if(nMsgType == MSG_LOG_ENTRY) {
                writeLogEntry(szType,(const char *) pMessage);
            } else {
                writeLogEntry(szType,(JsonNode *)(pMessage));
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
    if(pStream && pszMessage) {
        pStream->printf("[%s] %s\n", pszType ? pszType : "-", pszMessage);
    }
}

/**
 * @brief Writes a serialized JSON log entry to the stream.
 * @param pszType Log class string.
 * @param pDoc JSON node to serialize.
 */
void CStreamLogWriter::writeLogEntry(const char *pszType, JsonNode *pDoc) {
    if(pStream && pDoc) {
        const char *pszJson = pDoc->getAsJsonText();
        pStream->printf("[%s] %s\n", pszType ? pszType : "-", pszJson ? pszJson : "");
    }
}


