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

CSysStatus::CSysStatus() {
    m_pszChipID = NULL;
}

CSysStatus::~CSysStatus() {
    if(m_pszChipID) free((void *) m_pszChipID);
    m_pszChipID = NULL;
}
const char * CSysStatus::getSdkVersion() {
    return(ESP.getSdkVersion());
}

const char * CSysStatus::getChipModel() {
    #if defined(ARDUINO_ARCH_ESP32)
        return(ESP.getChipModel());
    #else
        String strChipID = String(ESP.getChipId(), HEX);
        if(m_pszChipID) free((void *) m_pszChipID);
        m_pszChipID = strdup(strChipID.c_str());
        return(m_pszChipID);
    #endif

}

uint32_t CSysStatus::getFlashChipId(){
    #if defined(ARDUINO_ARCH_ESP32)
        uint32_t id = g_rom_flashchip.device_id;
        id = ((id & 0xff) << 16) | ((id >> 16) & 0xff) | (id & 0xff00);
    #else
        uint32_t id = ESP.getFlashChipId();
    #endif
    return id;
}

uint32_t CSysStatus::getFlashChipRealSize(){
    uint32_t id = (getFlashChipId() >> 16) & 0xFF;
    return 2 << (id - 1);
}

uint32_t CSysStatus::getFlashChipSize(){
    return(ESP.getFlashChipSize());
}

/// @brief RAM size (total heap size)
/// @return 
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
/// @brief free RAM
/// @return 
uint32_t CSysStatus::getFreeHeap() {
    return(ESP.getFreeHeap());
}

uint32_t CSysStatus::getSketchSize() {
    return(ESP.getSketchSize());
}

/// @brief Free space for sketch - should be able to cover a new sketch...
/// @return 
uint32_t CSysStatus::getFreeSketchSpace() {
    return(ESP.getFreeSketchSpace());
}

uint32_t CSysStatus::getCpuFrequencyMhz() {
    return(ESP.getCpuFreqMHz());
}

void CSysStatus::writeStatusTo(JsonObject &oStatusObj) {
    DEBUG_FUNC_START_PARMS("%s",oStatusObj ? "OK" : "-null-");
    
    CFS oFS;   
    // uint32_t nFreeHeap;
    // uint32_t nMaxHeap;
    // uint8_t  nFragHeap;
    
    // ESP.getHeapStats(&nFreeHeap,&nMaxHeap,&nFragHeap);

    oStatusObj["full_ver"]          = getSdkVersion();
    oStatusObj["chip_id"]           = getChipModel();
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