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
/// @brief Shared filesystem initialization state.
struct LSC_FS {
    /// @brief true once the configured filesystem has been mounted/opened.
    bool isFSInitializedAndOpen = false;
};

/**
 * @brief Filesystem facade used by modules to avoid direct LittleFS/SPIFFS use.
 */
 class CFS : public IStatusHandler {
    protected:
        static LSC_FS m_oFS;
    public:
        /// @brief Create a filesystem helper and initialize the base filesystem if needed.
        CFS();

        /// @brief Return true if the file exists.
        bool    fileExists(const char *pszFileName);
        /// @brief String overload for file existence checks.
        bool    fileExists(String &strFileName);
        /// @brief Delete a file when it exists.
        void    deleteFile(const char *pszFileName);
        /// @brief String overload for deleting a file.
        void    deleteFile(String &strFileName);
        /// @brief Delete all files below the given path.
        void    deleteAllFilesOnPath(const char *pszPath);
        /// @brief Return the file size in bytes, or 0 if it cannot be read.
        size_t  getFileSize(const char *pszFileName);
        /// @brief String overload for file size lookup.
        size_t  getFileSize(String &strFileName);
        /// @brief Return the configured base filesystem object.
        FS      getBaseFS();
    
    
        /// @brief Load a file into an owned zero-terminated buffer.
        size_t  loadFileToBuffer(const char* strFileName, std::unique_ptr<char[]> &pData);
        /// @brief Load a file into a String.
        bool    loadFileToString(const char* strFileName, String &strResult);
        /// @brief Parse JSON content from a file into a JsonNode.
        bool    loadJsonContentFromFile(const char *strFileName,   JsonNode &oDoc);
        /// @brief Serialize and save a JsonNode to a file.
        bool    saveJsonContentToFile(const char* strFileName,     JsonNode &oDoc);
        
        /// @brief Return total bytes of the mounted filesystem.
        size_t  getTotalBytes();
        /// @brief Return used bytes of the mounted filesystem.
        size_t  getUsedBytes();
        /// @brief Write a directory listing into a JSON node.
        bool    getFileList(JsonNode &oDirDoc, const char *pszPath = "/");
        /// @brief Return a directory listing serialized as text.
        String  getFileList(const char* pszPath = "/");

        /// @brief Write filesystem capacity/status into a JSON node.
        void writeStatusTo(JsonNode &oStatus, int nLevel = STATUS_LEVEL_INFO) override;
    
 };
