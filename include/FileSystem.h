#pragma once
#include <FS.h>
#include <Runtime.h>
#include <StatusHandler.h>
/**
 * FileSystem helper functions
 * 
 * This modules helps you to operate with the Filesystem of the ESP32.
 * To avoid programming against LittleFS or SPIFFS, this module provides
 * simple functions to operate with the filesystem, so you can easily switch 
 * between this systems.
 * 
 * Known issues: 
 *  Do not store filenames in F("") macros - LittleFS will throw an exception (3).
 */
struct LSC_FS {
    bool isFSInitializedAndOpen = false;
};

 class CFS : public IStatusHandler {
    protected:
        static LSC_FS m_oFS;
    public:
        CFS();

        bool    fileExists(const char *pszFileName);
        bool    fileExists(String &strFileName);
        void    deleteFile(const char *pszFileName);
        void    deleteFile(String &strFileName);
        void    deleteAllFilesOnPath(const char *pszPath);
        size_t  getFileSize(const char *pszFileName);
        size_t  getFileSize(String &strFileName);
        FS      getBaseFS();
    
    
        size_t  loadFileToBuffer(const char* strFileName, std::unique_ptr<char[]> &pData);
        bool    loadFileToString(const char* strFileName, String &strResult);
        bool    loadJsonContentFromFile(const char *strFileName,   JsonNode &oDoc);
        bool    saveJsonContentToFile(const char* strFileName,     JsonNode &oDoc);
        
        size_t  getTotalBytes();
        size_t  getUsedBytes();
        bool    getFileList(JsonNode &oDirDoc, const char *pszPath = "/");
        String  getFileList(const char* pszPath = "/");

        void writeStatusTo(JsonNode &oStatus, int nLevel = STATUS_LEVEL_INFO) override;
    
 };

