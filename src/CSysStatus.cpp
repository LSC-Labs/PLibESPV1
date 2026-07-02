#ifndef DEBUG_LSC_APPL
	#undef DEBUGINFOS
#endif

#include <SysStatus.h>
#include <FileSystem.h>
#include <DevelopmentHelper.h>
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
/** 
* https://github.com/Jason2866/ESP32_Show_Info/blob/main/src/Chip_info.ino#L68
*/

/**
 * @brief Creates a system status helper.
 */
CSysStatus::CSysStatus() {
    m_pszChipID = NULL;
}

/**
 * @brief Releases cached chip ID strings used on ESP8266.
 */
CSysStatus::~CSysStatus() {
    if(m_pszChipID) free((void *) m_pszChipID);
    m_pszChipID = NULL;
}

/**
 * @brief Gets the Arduino/SDK version string.
 */
const char * CSysStatus::getSdkVersion() {
    return(ESP.getSdkVersion());
}

/**
 * @brief Gets a stable chip identifier string.
 *
 * ESP32 returns the chip model. ESP8266 builds a lowercase hex chip id and
 * caches it because the returned pointer must stay valid after the function
 * returns.
 */
const char * CSysStatus::getChipID() {
    #if defined(ARDUINO_ARCH_ESP32)
        return(ESP.getChipModel());
    #else
        String strChipID = String(ESP.getChipId(), HEX);
        if(m_pszChipID) free((void *) m_pszChipID);
        m_pszChipID = strdup(strChipID.c_str());
        return(m_pszChipID);
    #endif

}

/**
 * @brief Gets the flash chip id with ESP32 byte order normalization.
 */
uint32_t CSysStatus::getFlashChipId(){
    #if defined(ARDUINO_ARCH_ESP32)
        uint32_t id = g_rom_flashchip.device_id;
        id = ((id & 0xff) << 16) | ((id >> 16) & 0xff) | (id & 0xff00);
    #else
        uint32_t id = ESP.getFlashChipId();
    #endif
    return id;
}

/**
 * @brief Calculates the physical flash chip size from the flash chip id.
 */
uint32_t CSysStatus::getFlashChipRealSize(){
    uint32_t id = (getFlashChipId() >> 16) & 0xFF;
    return 2 << (id - 1);
}

/**
 * @brief Gets the flash size reported by the runtime/board config.
 */
uint32_t CSysStatus::getFlashChipSize(){
    return(ESP.getFlashChipSize());
}

/**
 * @brief Gets the best available total heap size.
 *
 * ESP8266 does not expose the same total heap API as ESP32, so the maximum heap
 * value from getHeapStats() is used as approximation.
 */
uint32_t CSysStatus::getHeapSize() {
    #if defined(ARDUINO_ARCH_ESP32)
        return(ESP.getHeapSize());
    #else
        // TODO: find a correct way to get the heap size (not the max...)
        uint32_t nFreeHeap;
        uint32_t nMaxHeap;
        uint8_t nFragHeap;
        ESP.getHeapStats(&nFreeHeap,&nMaxHeap,&nFragHeap);
        return(nMaxHeap);
    #endif
}

/**
 * @brief Gets the currently free heap.
 */
uint32_t CSysStatus::getFreeHeap() {
    return(ESP.getFreeHeap());
}

/**
 * @brief Gets the current sketch size.
 */
uint32_t CSysStatus::getSketchSize() {
    return(ESP.getSketchSize());
}

/**
 * @brief Gets free OTA sketch space.
 */
uint32_t CSysStatus::getFreeSketchSpace() {
    return(ESP.getFreeSketchSpace());
}

/**
 * @brief Gets the CPU frequency in MHz.
 */
uint32_t CSysStatus::getCpuFrequencyMhz() {
    return(ESP.getCpuFreqMHz());
}

/**
 * @brief Writes system diagnostics into a status JSON node.
 * @param oStatusObj Target node.
 * @param nLevel Status verbosity, currently unused.
 */
void CSysStatus::writeStatusTo(JsonNode &oStatusObj,int nLevel) {
    DEBUG_FUNC_START_PARMS("%s",oStatusObj ? "OK" : "-null-");
    
    CFS oFS;   
    // uint32_t nFreeHeap;
    // uint32_t nMaxHeap;
    // uint8_t  nFragHeap;
    
    // ESP.getHeapStats(&nFreeHeap,&nMaxHeap,&nFragHeap);

    oStatusObj["full_ver"]          = getSdkVersion();
    oStatusObj["chip_id"]           = getChipID();
    oStatusObj["cpu_clock"]         = ESP.getCpuFreqMHz();
    // oStatusObj["core_ver"]          = ESP.getCoreVersion();
    oStatusObj["flash_size"]        = getFlashChipSize();
    oStatusObj["flash_real_size"]   = getFlashChipRealSize();    
    oStatusObj["heap_free"]         = getFreeHeap(); // ESP.getFreeHeap(); 
    oStatusObj["heap_max"]          = getHeapSize(); // nMaxHeap;
    oStatusObj["sketch_size"]       = getSketchSize();
    oStatusObj["sketch_free_size"]  = getFreeSketchSpace(); // availsize - should be able to cover a new sketch
    oStatusObj["fs_total"]          = oFS.getTotalBytes();
    oStatusObj["fs_used"]           = oFS.getUsedBytes();

    DEBUG_FUNC_END();
}
