
#ifndef DEBUG_LSC_FILESYSTEM
    #undef DEBUGINFOS
#endif
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#include <JsonNode.h>
#include <FileSystem.h>
#include <LittleFS.h>
#include <DevelopmentHelper.h>

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
                    DEBUG_INFOS(" - deleting file %s",oDirEntry.fileName().c_str());
                    LittleFS.remove(oFile.name());
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
                    strResult += ", \"n\": \"" + String(oFile.name()) + "\"";
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
            strResult += ", \"n\": \"" + oDirEntry.fileName() + "\"";
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
        pData = std::unique_ptr<char[]>(new char[oFile.size()]);
        // std::unique_ptr<char[]> ptrBuffer(new char [oFile.size()]);
        nDataLen = oFile.readBytes(pData.get(),oFile.size());
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
        oFile.write(oDoc.getAsJsonText());
        // serializeJson(oDoc,oFile);
        oFile.close();
        bResult = true;
    }
    return(bResult);
}

/**
 * @brief Loads a JSON file and parses it into a JsonNode.
 * @param strFileName File path to read.
 * @param oDoc Target JSON node.
 * @return true when bytes were loaded from the file.
 */
bool CFS::loadJsonContentFromFile(const char *strFileName,JsonNode &oDoc) {
    DEBUG_FUNC_START_PARMS("%s,...",strFileName);
    int nSize = -1;
    std::unique_ptr<char[]> ptrBuffer;
    nSize = loadFileToBuffer(strFileName,ptrBuffer);
    // (void) nSize;
    DEBUG_INFOS(" --- loaded %d bytes %s",nSize,ptrBuffer ? ptrBuffer.get() : "-nullptr-");
    if(ptrBuffer) {
        oDoc.parse(ptrBuffer.get());
        /*
	    auto error = deserializeJson(oDoc, ptrBuffer.get());
        if (error) {
            DEBUG_INFOS(" --- deserializeJson() failed: %s", error.c_str());
        } 
            */
    }
    bool bResult = nSize > 0? true: false;
    DEBUG_FUNC_END_PARMS("%s",bResult ? "OK" : "NOT Loaded");
    return(bResult);
}
