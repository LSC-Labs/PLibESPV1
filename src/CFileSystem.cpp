
#ifndef DEBUG_LSC_FILESYSTEM
    #undef DEBUGINFOS
#endif
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#include <JsonNode.h>
#include <FileSystem.h>
#include <LittleFS.h>
#include <DevelopmentHelper.h>
#include <Appl.h>

namespace LSC {
    namespace FS {
        bool isInitializedAndOpen = false;
    }
}

 
/**
 * @brief Initializes LittleFS, formatting once if the initial mount fails.
 */
CFS::CFS() {
    if(!LSC::FS::isInitializedAndOpen) {
        LSC::FS::isInitializedAndOpen = LittleFS.begin();
    }
    if(!LSC::FS::isInitializedAndOpen) {
        LittleFS.format();
        LSC::FS::isInitializedAndOpen = LittleFS.begin();
    } 
}

/**
 * @brief Gets the underlying filesystem instance.
 * @return LittleFS object.
 */
FS CFS::getBaseFS() { return(LittleFS); }

/**
 * @brief Writes filesystem capacity information into a status node.
 * @param oStatus Target status node.
 * @param nLevel Status verbosity; capacity values are written from INFO level.
 */
void CFS::writeStatusTo(JsonNode &oStatus, int nLevel) {
    if(LSC::FS::isInitializedAndOpen) {
        if(nLevel >= STATUS_LEVEL_INFO) {
            oStatus["fs_total"]          = getTotalBytes();
            oStatus["fs_used"]           = getUsedBytes();
        }
    }
}

/**
 * @brief Gets total filesystem capacity in bytes.
 * @return Total LittleFS capacity.
 */
size_t CFS::getTotalBytes() { 
    #if defined(ARDUINO_ARCH_ESP32)
        return LittleFS.totalBytes();
    #else
        FSInfo oFsInfo;
        LittleFS.info(oFsInfo);
        return oFsInfo.totalBytes;
    #endif
}

/**
 * @brief Gets used filesystem capacity in bytes.
 * @return Used LittleFS capacity.
 */
size_t CFS::getUsedBytes()  { 
    #if defined(ARDUINO_ARCH_ESP32)
        return LittleFS.usedBytes();
    #else
        FSInfo oFsInfo;
        LittleFS.info(oFsInfo);
        return oFsInfo.usedBytes;
    #endif
}

/**
 * @brief Checks if a file exists by C string path.
 * @param pszFileName File path.
 * @return true when the path exists in LittleFS.
 */
bool CFS::fileExists(const char *pszFileName) {
    bool bResult = false;
    if(pszFileName) bResult = LittleFS.exists(pszFileName);
    return(bResult);
}

/**
 * @brief Checks if a file exists by String path.
 * @param strFileName File path.
 * @return true when the path exists in LittleFS.
 */
bool CFS::fileExists(String &strFileName) {
    bool bResult = false;
    if(strFileName) bResult = LittleFS.exists(strFileName);
    return(bResult);
}

/**
 * @brief Deletes a file by C string path if it exists.
 * @param pszFileName File path.
 */
void CFS::deleteFile(const char *pszFileName) {
    if(fileExists(pszFileName)) LittleFS.remove(pszFileName);
}

/**
 * @brief Deletes a file by String path if it exists.
 * @param strFileName File path.
 */
void CFS::deleteFile(String &strFileName) {
    if(fileExists(strFileName)) LittleFS.remove(strFileName);
}

/**
 * @brief Deletes all files directly contained in a directory.
 * @param pszPath Directory path. nullptr means root.
 */
void CFS::deleteAllFilesOnPath(const char *pszPath) {
    String strPath = pszPath ? pszPath : "/";
    #if defined(ARDUINO_ARCH_ESP32)
        File oDirEntry = LittleFS.open(strPath);
        if(oDirEntry.isDirectory()) {
            File oFile = oDirEntry.openNextFile();
            while (oFile) {
                if (!oFile.isDirectory()) {
                    // name() has no path on ESP32 - use the full path, close before removing
                    String strFile = oFile.path();
                    oFile.close();
                    DEBUG_INFOS(" - deleting file %s",strFile.c_str());
                    LittleFS.remove(strFile);
                }
                oFile = oDirEntry.openNextFile();
            }
        }
    #else
        Dir oDirEntry = LittleFS.openDir(strPath);
        while (oDirEntry.next()) {
            if (oDirEntry.isFile()) {
                DEBUG_INFOS(" - deleting file %s",oDirEntry.fileName().c_str());
                LittleFS.remove(oDirEntry.fileName());
            }
        }
    #endif
}


/**
 * @brief Gets a file size by C string path.
 * @param pszFileName File path.
 * @return File size in bytes, or (size_t)-1 when the file does not exist.
 */
size_t CFS::getFileSize(const char *pszFileName) {
    size_t nSize = -1;
    if(fileExists(pszFileName)) {
        File oFP = LittleFS.open(pszFileName,"r");
        if(oFP) {
            nSize = oFP.size();
            oFP.close();
        }
    }
    return(nSize);
}

/**
 * @brief Gets a file size by String path.
 * @param strFileName File path.
 * @return File size in bytes, or (size_t)-1 when the file does not exist.
 */
size_t CFS::getFileSize(String &strFileName) {
    return(getFileSize(strFileName.c_str()));
}

/**
 * @brief Writes the directory listing JSON into a JsonNode.
 * @param oDirDoc Target node that receives the parsed file list.
 * @param pszPath Directory path. nullptr means root.
 * @return true when parsing the generated list succeeds.
 */
bool CFS::getFileList(JsonNode &oDirDoc, const char *pszPath) {
    return(oDirDoc.parse(getFileList(pszPath).c_str()));
}

/**
 * @brief Builds a JSON array string with directory entries.
 *
 * Each entry contains type ("t"), size ("s") and name ("n"). The implementation
 * handles the different ESP32 and ESP8266 LittleFS directory APIs.
 *
 * @param pszPath Directory path. nullptr means root.
 * @return JSON array string with file entries.
 */
static String jsonEscapedName(const String &strName) {
    String strResult;
    for (size_t nIdx = 0; nIdx < strName.length(); nIdx++) {
        char c = strName[nIdx];
        if (c == '"' || c == '\\') strResult += '\\';
        if ((unsigned char) c >= 0x20) strResult += c;
    }
    return(strResult);
}

String CFS::getFileList(const char* pszPath) {
    String strResult = "[";
    const char *pszDir = pszPath ?  pszPath : "/";
    #if defined(ARDUINO_ARCH_ESP32)
        File oDirEntry = LittleFS.open(pszDir);
        if(oDirEntry.isDirectory()) {
            File oFile = oDirEntry.openNextFile();
            while (oFile) {
                    if (strResult.length() > 5) strResult += ",";
                    strResult += "{ \"t\": ";{
                    strResult += oFile.isDirectory() ? "\"D\"" : "\"F\"";
                    strResult += ", \"s\": ";
                    strResult += oFile.size();
                    strResult += ", \"n\": \"" + jsonEscapedName(String(oFile.name())) + "\"";
                    strResult += "}";
                }
                oFile = oDirEntry.openNextFile();
            }
        }
    #else
        Dir oDirEntry = LittleFS.openDir(pszDir);
        while (oDirEntry.next()) {
            if (strResult.length() > 5) strResult += ",";
            strResult += "{ \"t\": ";
            if(oDirEntry.isFile()) strResult += "\"F\"";
            else if(oDirEntry.isDirectory()) strResult += "\"D\"";
            else strResult += "\"-\"";
            strResult += ", \"s\": ",
            strResult += oDirEntry.fileSize();
            strResult += ", \"n\": \"" + jsonEscapedName(oDirEntry.fileName()) + "\"";
            strResult += "}";
        }
    #endif
    strResult += "]";
    return(strResult);
}

/**
 * @brief Loads a file into an owned character buffer.
 *
 * LittleFS must already be initialized. Do not pass flash-string F() values as
 * file names; LittleFS expects a normal C string.
 *
 * @param strFileName File path to read.
 * @param pData Output buffer receiving file content.
 * @return Number of bytes read, or 0 when the file could not be opened.
 */
size_t CFS::loadFileToBuffer(const char* strFileName, std::unique_ptr<char[]> &pData) {
    size_t nDataLen = 0;
    // std::unique_ptr<char[]> ptrBuffer = nullptr;
    File oFile = LittleFS.open(strFileName,"r");
    if(oFile) {
        size_t nFileSize = oFile.size();
        // +1: the buffer is used as C string (i.e. JSON parser) - it must be zero terminated,
        // otherwise the reader runs into the heap data behind the file content.
        pData = std::unique_ptr<char[]>(new char[nFileSize + 1]);
        nDataLen = oFile.readBytes(pData.get(),nFileSize);
        pData[nDataLen] = '\0';
        oFile.close();
    }
    return(nDataLen);
}

/**
 * @brief Loads a whole file into a String.
 * @param strFileName File path to read.
 * @param strResult Output string receiving file content.
 * @return true when the file was opened and read.
 */
bool CFS::loadFileToString(const char* strFileName, String &strResult) {
    bool bResult = false;
    File oFile = LittleFS.open(strFileName,"r");
    if(oFile) {
        strResult = oFile.readString();
        oFile.close();
        bResult = true;
    }
    return(bResult);
}

/**
 * @brief Serializes a JSON node and writes it to a file.
 * @param strFileName File path to write.
 * @param oDoc JSON node to serialize.
 * @return true when the file was opened and written.
 */
bool CFS::saveJsonContentToFile(const char* strFileName, JsonNode &oDoc) {
    bool bResult = false;
    File oFile = LittleFS.open(strFileName, "w");
    if (oFile)
    {
        const char *pszData = oDoc.getAsJsonText();
        size_t nLen = pszData ? strlen(pszData) : 0;
        bResult = oFile.write((const uint8_t *) pszData, nLen) == nLen;
        oFile.close();
    }
    return(bResult);
}

/**
 * @brief Loads a JSON file and parses it into a JsonNode.
 * @param strFileName File path to read.
 * @param oDoc Target JSON node.
 * @return true when the file was loaded and contains valid JSON.
 */
bool CFS::loadJsonContentFromFile(const char *strFileName,JsonNode &oDoc) {
    DEBUG_FUNC_START_PARMS("%s,...",strFileName);
    int nSize = -1;
    std::unique_ptr<char[]> ptrBuffer;
    nSize = loadFileToBuffer(strFileName,ptrBuffer);
    // (void) nSize;
    DEBUG_INFOS(" --- loaded %d bytes %s",nSize,ptrBuffer ? ptrBuffer.get() : "-nullptr-");
    bool bResult = false;
    if(ptrBuffer && nSize > 0) {
        // parse() keeps oDoc unchanged and returns nullptr on invalid JSON
        bResult = oDoc.parse(ptrBuffer.get()) != nullptr;
        if(!bResult) {
            // Show where the strict parser stopped (helps to find the invalid part)
            size_t nPos = JsonNode::getLastParseErrorOffset();
            if(nPos > (size_t) nSize) nPos = (size_t) nSize;
            size_t nFrom = nPos > 30 ? nPos - 30 : 0;
            ApplLogWarnWithParms(F("%s: invalid JSON at offset %u: ...%.*s<<<HERE"), strFileName,
                                 (unsigned) nPos, (int) (nPos - nFrom), ptrBuffer.get() + nFrom);
            // Files written by older versions are not always valid JSON - try the legacy mode.
            // The file is written in valid JSON with the next save.
            bResult = oDoc.parse(ptrBuffer.get(), true) != nullptr;
            if(bResult) {
                ApplLogWarnWithParms(F("%s is not valid JSON - loaded in legacy mode"), strFileName);
            } else {
                DEBUG_INFOS(" --- invalid JSON in %s", strFileName);
            }
        }
    }
    DEBUG_FUNC_END_PARMS("%s",bResult ? "OK" : "NOT Loaded");
    return(bResult);
}
