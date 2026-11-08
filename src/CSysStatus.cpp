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
CSysStatus::CSysStatus() {}

/**
 * @brief Nothing to release (fixed buffers).
 */
CSysStatus::~CSysStatus() {}

/**
 * @brief Gets the Arduino/SDK version string.
 */
const char * CSysStatus::getSdkVersion() {
    return(ESP.getSdkVersion());
}

/**
 * @brief Gets a unique chip identifier string.
 *
 * Lower 24 bit of the factory MAC address as hex - the value ESP.getChipId()
 * returns on ESP8266. On ESP32 the chip model was returned before, which is the
 * same for all devices of a type (not unique - Appl.getDeviceID() and the Home
 * Assistant unique ids use this value).
 */
const char * CSysStatus::getChipID() {
    if(m_szChipID[0] == '\0') {
        #if defined(ARDUINO_ARCH_ESP32)
            uint64_t ullMac = ESP.getEfuseMac();    // byte 0 of the MAC is the lowest byte
            uint32_t ulChipId = 0;
            for(int nIdx = 0; nIdx < 3; nIdx++) {
                ulChipId |= (uint32_t) ((ullMac >> (8 * (5 - nIdx))) & 0xFF) << (8 * nIdx);
            }
        #else
            uint32_t ulChipId = ESP.getChipId();
        #endif
        snprintf(m_szChipID, sizeof(m_szChipID), "%x", (unsigned int) ulChipId);
    }
    return(m_szChipID);
}

/**
 * @brief Gets the chip model.
 */
const char * CSysStatus::getChipModel() {
    #if defined(ARDUINO_ARCH_ESP32)
        return(ESP.getChipModel());
    #else
        return("ESP8266");
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
 * @brief Gets the physical flash chip size (read from the chip).
 *
 * ESP32: esp_flash_get_size() of the main flash, ESP8266: ESP.getFlashChipRealSize().
 * Both use the JEDEC id of the chip. The previous calculation 2 << (id - 1)
 * was undefined for unknown chips (id 0 => shift by -1).
 */
uint32_t CSysStatus::getFlashChipRealSize(){
    if(m_ulFlashRealSize == 0) {
        #if defined(ARDUINO_ARCH_ESP32)
            uint32_t ulSize = 0;
            if(esp_flash_get_size(NULL, &ulSize) == ESP_OK) m_ulFlashRealSize = ulSize;
        #else
            m_ulFlashRealSize = ESP.getFlashChipRealSize();
        #endif
    }
    return(m_ulFlashRealSize);
}

/**
 * @brief Gets the flash size reported by the runtime/board config.
 */
uint32_t CSysStatus::getFlashChipSize(){
    return(ESP.getFlashChipSize());
}

/**
 * @brief Gets the heap size.
 *
 * ESP32: total heap size. ESP8266 has no API for the total heap - the largest
 * free block is returned (approximation, value of "heap_max" in the status).
 */
uint32_t CSysStatus::getHeapSize() {
    #if defined(ARDUINO_ARCH_ESP32)
        return(ESP.getHeapSize());
    #else
        return(ESP.getMaxFreeBlockSize());
    #endif
}

/**
 * @brief Gets the currently free heap.
 */
uint32_t CSysStatus::getFreeHeap() {
    return(ESP.getFreeHeap());
}

/**
 * @brief Gets the largest block that can be allocated at once.
 */
uint32_t CSysStatus::getMaxAllocHeap() {
    #if defined(ARDUINO_ARCH_ESP32)
        return(ESP.getMaxAllocHeap());
    #else
        return(ESP.getMaxFreeBlockSize());
    #endif
}

/**
 * @brief Gets the heap fragmentation in percent (100 - largest block / free heap).
 */
uint8_t CSysStatus::getHeapFragmentation() {
    #if defined(ARDUINO_ARCH_ESP32)
        uint32_t ulFree = getFreeHeap();
        if(ulFree == 0) return(0);
        return((uint8_t) (100 - (uint64_t) getMaxAllocHeap() * 100 / ulFree));
    #else
        return(ESP.getHeapFragmentation());
    #endif
}

/**
 * @brief Gets the free PSRAM (0 if not available, always 0 on ESP8266).
 */
uint32_t CSysStatus::getFreePsram() {
    #if defined(ARDUINO_ARCH_ESP32)
        return(ESP.getFreePsram());
    #else
        return(0);
    #endif
}

/**
 * @brief Gets the current sketch size (cached - does not change at runtime).
 */
uint32_t CSysStatus::getSketchSize() {
    if(m_ulSketchSize == 0) m_ulSketchSize = ESP.getSketchSize();
    return(m_ulSketchSize);
}

/**
 * @brief Gets free OTA sketch space (cached - does not change at runtime).
 */
uint32_t CSysStatus::getFreeSketchSpace() {
    if(m_ulFreeSketchSpace == 0) m_ulFreeSketchSpace = ESP.getFreeSketchSpace();
    return(m_ulFreeSketchSpace);
}

/**
 * @brief Gets the CPU frequency in MHz.
 */
uint32_t CSysStatus::getCpuFrequencyMhz() {
    return(ESP.getCpuFreqMHz());
}

/**
 * @brief Writes system diagnostics into a status JSON node.
 *
 * Existing fields are unchanged ("heap_max": ESP32 total heap, ESP8266 largest
 * free block). New: "chip_model", "heap_max_block", "heap_frag" and - from
 * STATUS_LEVEL_VERBOSE - "flash_chip_id" and "psram_free".
 *
 * @param oStatusObj Target node.
 * @param nLevel Status verbosity.
 */
void CSysStatus::writeStatusTo(JsonNode &oStatusObj,int nLevel) {
    DEBUG_FUNC_START_PARMS("%d",nLevel);
    CFS oFS;   

    oStatusObj["full_ver"]          = getSdkVersion();
    oStatusObj["chip_id"]           = getChipID();
    oStatusObj["chip_model"]        = getChipModel();
    oStatusObj["cpu_clock"]         = getCpuFrequencyMhz();
    oStatusObj["flash_size"]        = getFlashChipSize();
    oStatusObj["flash_real_size"]   = getFlashChipRealSize();    
    oStatusObj["heap_free"]         = getFreeHeap();
    oStatusObj["heap_max"]          = getHeapSize();
    oStatusObj["heap_max_block"]    = getMaxAllocHeap();
    oStatusObj["heap_frag"]         = (unsigned int) getHeapFragmentation();
    oStatusObj["sketch_size"]       = getSketchSize();
    oStatusObj["sketch_free_size"]  = getFreeSketchSpace(); // availsize - should be able to cover a new sketch
    oStatusObj["fs_total"]          = oFS.getTotalBytes();
    oStatusObj["fs_used"]           = oFS.getUsedBytes();
    if(nLevel >= STATUS_LEVEL_VERBOSE) {
        char szFlashId[12];
        snprintf(szFlashId, sizeof(szFlashId), "%06x", (unsigned int) getFlashChipId());
        oStatusObj["flash_chip_id"] = szFlashId;
        oStatusObj["psram_free"]    = getFreePsram();
    }
    DEBUG_FUNC_END();
}
