#ifndef DEBUG_LSC_LOGGING
    #undef DEBUGINFOS
#endif

#include <Arduino.h>
#include <ArduinoJson.h>
#include <Logging.h>
#include <Msgs.h>
#include <DevelopmentHelper.h>

#define LOG_ENTRY_BUFFER_SIZE  512

#pragma region Implementation of CEventLogger

CEventLogger::CEventLogger() { pEventHandler = nullptr;}
CEventLogger::CEventLogger(CEventHandler *oEventHandler) {
    pEventHandler = oEventHandler;
}

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

void CEventLogger::log(const char * pszType, const __FlashStringHelper* pszMessage,...) {
    va_list args;
    va_start(args, (const char*)pszMessage);
    char strBuffer[LOG_ENTRY_BUFFER_SIZE];
    vsnprintf_P(strBuffer, sizeof(strBuffer), (const char *) pszMessage, args);
    va_end(args);
    char cType = pszType == nullptr ? 'I' : pszType[0];
    pEventHandler->sendEvent(this, MSG_LOG_ENTRY, strBuffer,cType );
}

void CEventLogger::log(const char * pszType, const char *pszMessage, ...) {
    va_list args;
    va_start(args, pszMessage);
    char tBuffer[LOG_ENTRY_BUFFER_SIZE];
    vsnprintf_P(tBuffer, sizeof(tBuffer), pszMessage, args);
    va_end(args);
    char cType = pszType == nullptr ? 'I' : pszType[0];
    pEventHandler->sendEvent(this, MSG_LOG_ENTRY, tBuffer, cType );
}

void CEventLogger::log(const char * pszType, JsonDocument *pDoc) {
    char cType = pszType == nullptr ? 'I' : pszType[0];
    pEventHandler->sendEvent(this, MSG_LOG_ENTRY_JSON,pDoc, cType );
}
#pragma endregion

#pragma region CLogWriter, Base Class for all Log Writers

int CLogWriter::getLogLevel() {
    return(m_nLogLevel);
}

int CLogWriter::setLogLevel(int nLogLevel) {
    int nOldLevel = m_nLogLevel;
    m_nLogLevel = nLogLevel;
    return(nOldLevel);
}

bool CLogWriter::isTypeToWriteAlways(const char cType) {
    bool bResult = false;
    for(size_t nIdx = 0; nIdx < strlen(LOG_CLASS_TYPE_ALWAYS); nIdx++) {
        if(cType == LOG_CLASS_TYPE_ALWAYS[nIdx]) {
            bResult = true; break;
        }
    }
    return(bResult);
}

bool CLogWriter::isLogLevelToWrite(const char cType) {
    bool bResult = false;
    int nClass = CEventLogger::getLogClassNumberFrom(cType);
    if(nClass >= 0 && nClass <= m_nLogLevel) {
        bResult = true;
    }
    return(bResult);
}
void CLogWriter::writeLogEntry(const char *pszType, JsonDocument *pDoc) {
}

/// @brief Class to handle the logging for a module
///      This class is used to write log entries for a specific module
void CLogWriter::writeLogEntry(const char *pszType, const char *pszMessage) {
}

/// @brief Receive an event, and prepare the LOG entries.
///        
/// @param pSender  Sender of the message 
/// @param nMsgType MSG_LOG_ENTRY or MSG_LOG_ENTRY_JSON are interesting
/// @param pMessage Either a string of a JsonObject/JsonDocument
/// @param nClass   Class Level -> representing LogLevel like 'I' or 'W' as number
int CLogWriter::receiveEvent(const void *pSender, int nMsgType, const void *pMessage, int nClass) {
    if(nMsgType == MSG_LOG_ENTRY || nMsgType == MSG_LOG_ENTRY_JSON) {
        String strType = "";
        char cLogClass = nClass;
        if(isLogLevelToWrite(cLogClass) || isTypeToWriteAlways(cLogClass)) {
            strType = "[";
            strType += cLogClass;
            strType += "] ";
            if(nMsgType == MSG_LOG_ENTRY) {
                writeLogEntry(strType.c_str(),(const char *) pMessage);
            } 
            else if (nMsgType == MSG_LOG_ENTRY_JSON) {
                writeLogEntry(strType.c_str(),(JsonDocument *)(pMessage));
            }
        }
    }
    return(EVENT_MSG_RESULT_OK);
}

#pragma endregion

CStreamLogWriter::CStreamLogWriter(Stream *pStream) {
    this->pStream = pStream;
}

void CStreamLogWriter::writeLogEntry(const char *pszType, const char *pszMessage) {
    if(pStream) {
        pStream->printf("[%s] %s\n", pszType, pszMessage);
    }
}

void CStreamLogWriter::writeLogEntry(const char *pszType, JsonDocument *pDoc) {
    if(pStream) {
        serializeJsonPretty(*pDoc,*pStream);
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