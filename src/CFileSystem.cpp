
#ifndef DEBUG_LSC_FILESYSTEM
    #undef DEBUGINFOS
#endif
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#include <ArduinoJson.h>
#include <FileSystem.h>
#include <LittleFS.h>
#include <DevelopmentHelper.h>

namespace LSC {
    namespace FS {
        bool isInitializedAndOpen = false;
    }
}

 
CFS::CFS() {
    if(!LSC::FS::isInitializedAndOpen) {
        LSC::FS::isInitializedAndOpen = LittleFS.begin();
    }
    if(!LSC::FS::isInitializedAndOpen) {
        LittleFS.format();
        LSC::FS::isInitializedAndOpen = LittleFS.begin();
    } 
}

void CFS::writeStatusTo(JsonObject &oStatus) {
    if(LSC::FS::isInitializedAndOpen) {
        oStatus["fs_total"]          = getTotalBytes();
        oStatus["fs_used"]           = getUsedBytes();
    }
}

size_t CFS::getTotalBytes() { 
    #if defined(ARDUINO_ARCH_ESP32)
        return LittleFS.totalBytes();
    #else
        FSInfo oFsInfo;
        LittleFS.info(oFsInfo);
        return oFsInfo.totalBytes;
    #endif
}
size_t CFS::getUsedBytes()  { 
    #if defined(ARDUINO_ARCH_ESP32)
        return LittleFS.usedBytes();
    #else
        FSInfo oFsInfo;
        LittleFS.info(oFsInfo);
        return oFsInfo.usedBytes;
    #endif
}

bool CFS::fileExists(const char *pszFileName) {
    bool bResult = false;
    if(pszFileName) bResult = LittleFS.exists(pszFileName);
    return(bResult);
}

bool CFS::fileExists(String &strFileName) {
    bool bResult = false;
    if(strFileName) bResult = LittleFS.exists(strFileName);
    return(bResult);
}

void CFS::deleteFile(const char *pszFileName) {
    if(fileExists(pszFileName)) LittleFS.remove(pszFileName);
}

void CFS::deleteFile(String &strFileName) {
    if(fileExists(strFileName)) LittleFS.remove(strFileName);
}

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


/// @brief Get the file size, if the file exists
/// @param pszFileName the name of the file to be checked
/// @return -1 == file does not exist, otherwise the size of the file
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

size_t CFS::getFileSize(String &strFileName) {
    return(getFileSize(strFileName.c_str()));
}

bool CFS::getFileList(JsonDocument &oDirDoc, const char *pszPath) {
    return(oDirDoc.set(getFileList(pszPath)));
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

/// @brief Load a file into a buffer structure
///        Open the LittleFS with begin, before using this function (!)
///        Filename may NOT be stored in F("") - LittleFS will throw an exception (3).
/// @param strFileName The filename to be read.
/// @return nullptr - if it could not be loaded, otherwise a smart unique_ptr with the content of the requested file
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

bool loadFileToString(const char* strFileName, String &strResult) {
    bool bResult = false;
    File oFile = LittleFS.open(strFileName,"r");
    if(oFile) {
        strResult = oFile.readString();
        oFile.close();
        bResult = true;
    }
    return(bResult);
}

bool CFS::saveJsonContentToFile(const char* strFileName, JsonDocument &oDoc) {
    bool bResult = false;
    File oFile = LittleFS.open(strFileName, "w");
    if (oFile)
    {
        serializeJson(oDoc,oFile);
        oFile.close();
        bResult = true;
    }
    return(bResult);
}

bool CFS::saveJsonContentToFile(const char* pszFileName, JsonObject &oNode) {
    bool bResult = false;
    File oFile = LittleFS.open(pszFileName, "w");
    if (oFile)
    {
        serializeJson(oNode,oFile);
        oFile.close();
        bResult = true;
    }
    return(bResult);
}

bool CFS::loadJsonContentFromFile(const char *strFileName,JsonDocument &oDoc) {
    DEBUG_FUNC_START_PARMS("%s,...",strFileName);
    int nSize = -1;
    std::unique_ptr<char[]> ptrBuffer;
    nSize = loadFileToBuffer(strFileName,ptrBuffer);
    // (void) nSize;
    DEBUG_INFOS(" --- loaded %d bytes %s",nSize,ptrBuffer ? ptrBuffer.get() : "-nullptr-");
    if(ptrBuffer) {
	    auto error = deserializeJson(oDoc, ptrBuffer.get());
        if (error) {
            DEBUG_INFOS(" --- deserializeJson() failed: %s", error.c_str());
        } 
    }
    DEBUG_FUNC_END_PARMS("%s",bResult ? "OK" : "NOT Loaded");
    return(nSize > 0? true: false);
}
