#ifndef DEBUG_LSC_APPL
	#undef DEBUGINFOS
#endif

#include <SysStatus.h>
#include <FileSystem.h>
#include <DevelopmentHelper.h>
#if !defined(ARDUINO_ARCH_ESP32)
    extern "C" {
        #include <user_interface.h>     // struct rst_info, REASON_* (ESP8266 SDK)
    }
#endif
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

#pragma region Reset reason and crash record (RTC memory)

/**
 * Record of the last reset, kept in the RTC memory (survives a restart, not a power off).
 *
 * - CAppl::reboot() writes the source of a requested restart (markRequestedRestart()).
 * - ESP8266: the crash callback writes the crash information (abort/panic, exception,
 *   watchdog). The SDK reports an abort()/panic() (i.e. out of memory with exceptions
 *   enabled => std::bad_alloc) only as "Software/System restart" - without this record
 *   it can not be distinguished from a requested restart.
 */
namespace {
    const uint32_t RESET_RECORD_MAGIC = 0x4c534332;     // "LSC2" (v2: excvaddr + stack addresses)
    enum : uint32_t { RESET_RECORD_NONE = 0, RESET_RECORD_REQUESTED = 1, RESET_RECORD_CRASH = 2 };
    // Number of stack code addresses stored in a crash record (RTC memory is limited)
    const uint32_t RESET_RECORD_STACK_ADDRS = 8;

    struct ResetRecord {
        uint32_t ulMagic;
        uint32_t ulType;            // RESET_RECORD_*
        uint32_t ulReason;          // crash: rst_info.reason (254 = abort/panic)
        uint32_t ulExcCause;        // crash: exception cause
        uint32_t ulEpc1;            // crash: exception address
        uint32_t ulFailAllocAddr;   // crash: caller of the last failed allocation (0 = none)
        uint32_t ulFailAllocSize;   // crash: size of the last failed allocation
        uint32_t ulMinFreeHeap;     // minimum free heap before the reset
        uint32_t ulUptimeSec;       // uptime at the reset
        char     szSource[28];      // requested: source of the restart
        uint32_t ulExcVaddr;        // crash: memory address that caused the exception (i.e. 0 = NULL pointer)
        uint32_t ulStackAddrCount;  // crash: number of valid entries in ulStackAddr
        uint32_t ulStackAddr[RESET_RECORD_STACK_ADDRS]; // crash: code addresses found on the stack (call chain candidates)
    };

    // Minimum free heap since start - read by the crash callback (no function calls there)
    volatile uint32_t s_ulMinFreeHeap = 0xFFFFFFFF;

    #if defined(ARDUINO_ARCH_ESP32)
        RTC_NOINIT_ATTR ResetRecord s_oRtcRecord;
        void writeResetRecord(ResetRecord &oRecord) { s_oRtcRecord = oRecord; }
        bool readResetRecord(ResetRecord &oRecord)  { oRecord = s_oRtcRecord; return(true); }
    #else
        // Last 128 bytes of the 512 bytes RTC user memory (blocks of 4 bytes).
        // The start of the user memory is used by the OTA updater (eboot command).
        const uint32_t RESET_RECORD_RTC_OFFSET = 96;
        static_assert(RESET_RECORD_RTC_OFFSET * 4 + sizeof(ResetRecord) <= 512, "ResetRecord too large for RTC memory");
        void writeResetRecord(ResetRecord &oRecord) {
            ESP.rtcUserMemoryWrite(RESET_RECORD_RTC_OFFSET, (uint32_t *) &oRecord, sizeof(oRecord));
        }
        bool readResetRecord(ResetRecord &oRecord) {
            return(ESP.rtcUserMemoryRead(RESET_RECORD_RTC_OFFSET, (uint32_t *) &oRecord, sizeof(oRecord)));
        }
    #endif

    void clearResetRecord() {
        ResetRecord oRecord;
        memset(&oRecord, 0, sizeof(oRecord));
        writeResetRecord(oRecord);
    }
}

#if !defined(ARDUINO_ARCH_ESP32) && !defined(LSC_NO_CRASH_CALLBACK)
extern "C" {
    extern void *umm_last_fail_alloc_addr;
    extern int   umm_last_fail_alloc_size;

    /**
     * @brief Called by the ESP8266 core after a crash (exception, watchdog, abort/panic),
     *        before the restart. Stores the crash information in the RTC memory.
     *
     * Runs in the crash context: no allocation, no logging, no file system.
     * Define LSC_NO_CRASH_CALLBACK, if the application has its own callback.
     */
    void custom_crash_callback(struct rst_info *pRstInfo, uint32_t ulStack, uint32_t ulStackEnd) {
        ResetRecord oRecord;
        memset(&oRecord, 0, sizeof(oRecord));
        oRecord.ulMagic         = RESET_RECORD_MAGIC;
        oRecord.ulType          = RESET_RECORD_CRASH;
        oRecord.ulReason        = pRstInfo ? pRstInfo->reason   : 0;
        oRecord.ulExcCause      = pRstInfo ? pRstInfo->exccause : 0;
        oRecord.ulEpc1          = pRstInfo ? pRstInfo->epc1     : 0;
        oRecord.ulFailAllocAddr = (uint32_t) umm_last_fail_alloc_addr;
        oRecord.ulFailAllocSize = (uint32_t) umm_last_fail_alloc_size;
        oRecord.ulMinFreeHeap   = s_ulMinFreeHeap;
        oRecord.ulUptimeSec     = millis() / 1000;
        oRecord.ulExcVaddr      = pRstInfo ? pRstInfo->excvaddr : 0;
        // Scan the stack for code addresses (flash 0x402xxxxx / IRAM 0x401xxxxx) - the
        // same candidates the exception decoder shows. Decode with addr2line and the
        // firmware.elf of the flashed build. The first entries are the nearest callers.
        if(ulStack && ulStackEnd > ulStack && (ulStack & 3) == 0) {
            for(uint32_t ulPos = ulStack; ulPos + 4 <= ulStackEnd
                    && oRecord.ulStackAddrCount < RESET_RECORD_STACK_ADDRS; ulPos += 4) {
                uint32_t ulValue = *(volatile uint32_t *) ulPos;
                bool bIsCode = (ulValue >= 0x40200000 && ulValue < 0x40300000) ||
                               (ulValue >= 0x40100000 && ulValue < 0x40108000);
                if(bIsCode) oRecord.ulStackAddr[oRecord.ulStackAddrCount++] = ulValue;
            }
        }
        writeResetRecord(oRecord);
    }
}
#endif

/**
 * @brief Remember the source of a requested restart in the RTC memory.
 * @param pszSource Short text, i.e. "Button" or "Firmware update" (truncated to 27 chars).
 */
void CSysStatus::markRequestedRestart(const char *pszSource) {
    ResetRecord oRecord;
    memset(&oRecord, 0, sizeof(oRecord));
    oRecord.ulMagic       = RESET_RECORD_MAGIC;
    oRecord.ulType        = RESET_RECORD_REQUESTED;
    oRecord.ulMinFreeHeap = s_ulMinFreeHeap;
    oRecord.ulUptimeSec   = millis() / 1000;
    snprintf(oRecord.szSource, sizeof(oRecord.szSource), "%s", pszSource && *pszSource ? pszSource : "unknown");
    writeResetRecord(oRecord);
}

/**
 * @brief Tracks the minimum free heap (cheap - call it in the loop).
 */
void CSysStatus::updateMinFreeHeap() {
    uint32_t ulFree = ESP.getFreeHeap();
    if(ulFree < s_ulMinFreeHeap) s_ulMinFreeHeap = ulFree;
}

/**
 * @brief Minimum free heap since start (0 if not tracked yet).
 */
uint32_t CSysStatus::getMinFreeHeap() {
    return(s_ulMinFreeHeap == 0xFFFFFFFF ? 0 : s_ulMinFreeHeap);
}

/**
 * @brief Gets the reason of the last reset (cached).
 *
 * The system reason is completed by the record in the RTC memory:
 * - requested restart : "Software restart (Button)"
 * - crash (ESP8266)   : "Abort/Panic - last failed alloc 0x4020abcd (1234 B), ..."
 * The record is cleared after reading, so it is only reported once.
 */
const char * CSysStatus::getResetReason() {
    if(m_szResetReason[0] != '\0') return(m_szResetReason);

    ResetRecord oRecord;
    bool bHasRecord = readResetRecord(oRecord) && oRecord.ulMagic == RESET_RECORD_MAGIC;
    if(bHasRecord) oRecord.szSource[sizeof(oRecord.szSource) - 1] = '\0';
    clearResetRecord();

    #if defined(ARDUINO_ARCH_ESP32)
        const char *pszReason = "Unknown";
        esp_reset_reason_t eReason = esp_reset_reason();
        switch(eReason) {
            case ESP_RST_POWERON:   pszReason = "Power on";             break;
            case ESP_RST_EXT:       pszReason = "External reset";       break;
            case ESP_RST_SW:        pszReason = "Software restart";     break;
            case ESP_RST_PANIC:     pszReason = "Exception/panic";      break;
            case ESP_RST_INT_WDT:   pszReason = "Interrupt watchdog";   break;
            case ESP_RST_TASK_WDT:  pszReason = "Task watchdog";        break;
            case ESP_RST_WDT:       pszReason = "Other watchdog";       break;
            case ESP_RST_DEEPSLEEP: pszReason = "Deep sleep wakeup";    break;
            case ESP_RST_BROWNOUT:  pszReason = "Brownout";             break;
            case ESP_RST_SDIO:      pszReason = "SDIO reset";           break;
            default: break;
        }
        bool bSoftRestart = eReason == ESP_RST_SW;
        // The RTC_NOINIT memory is random after power on
        if(eReason == ESP_RST_POWERON || eReason == ESP_RST_BROWNOUT) bHasRecord = false;
    #else
        String strSysReason = ESP.getResetReason();
        const char *pszReason = strSysReason.c_str();
        rst_info *pInfo = ESP.getResetInfoPtr();
        bool bSoftRestart = pInfo && pInfo->reason == REASON_SOFT_RESTART;
    #endif

    if(bHasRecord && oRecord.ulType == RESET_RECORD_CRASH) {
        const char *pszCrash;
        switch(oRecord.ulReason) {
            case 254: pszCrash = "Abort/Panic";        break;   // REASON_USER_SWEXCEPTION_RST: abort(), panic(), assert, C++ exception
            case 2:   pszCrash = "Exception";          break;   // REASON_EXCEPTION_RST
            case 3:   pszCrash = "Software watchdog";  break;   // REASON_SOFT_WDT_RST
            case 1:   pszCrash = "Hardware watchdog";  break;   // REASON_WDT_RST
            case 255: pszCrash = "Stack smashing";     break;   // REASON_USER_STACK_SMASH
            default:  pszCrash = "Crash";              break;
        }
        int nLen = snprintf(m_szResetReason, sizeof(m_szResetReason), "%s", pszCrash);
        if(oRecord.ulReason == 2 && nLen < (int) sizeof(m_szResetReason)) {
            nLen += snprintf(&m_szResetReason[nLen], sizeof(m_szResetReason) - nLen,
                             " (%u) at 0x%08x, addr 0x%08x", (unsigned) oRecord.ulExcCause,
                             (unsigned) oRecord.ulEpc1, (unsigned) oRecord.ulExcVaddr);
        }
        if(oRecord.ulFailAllocAddr && nLen < (int) sizeof(m_szResetReason)) {
            nLen += snprintf(&m_szResetReason[nLen], sizeof(m_szResetReason) - nLen,
                             " - out of memory, failed alloc %u B at 0x%08x",
                             (unsigned) oRecord.ulFailAllocSize, (unsigned) oRecord.ulFailAllocAddr);
        }
        if(nLen < (int) sizeof(m_szResetReason)) {
            // 0xFFFFFFFF: the heap was never measured (crash before the first loop / web request)
            if(oRecord.ulMinFreeHeap == 0xFFFFFFFF) {
                nLen += snprintf(&m_szResetReason[nLen], sizeof(m_szResetReason) - nLen,
                                 " - min. heap n/a, after %lus", (unsigned long) oRecord.ulUptimeSec);
            } else {
                nLen += snprintf(&m_szResetReason[nLen], sizeof(m_szResetReason) - nLen,
                                 " - min. heap %u B, after %lus",
                                 (unsigned) oRecord.ulMinFreeHeap, (unsigned long) oRecord.ulUptimeSec);
            }
        }
        // Call chain candidates - decode with:
        // xtensa-lx106-elf-addr2line -pfiaC -e .pio/build/<env>/firmware.elf <addresses>
        uint32_t ulCount = oRecord.ulStackAddrCount <= RESET_RECORD_STACK_ADDRS ? oRecord.ulStackAddrCount : 0;
        if(ulCount > 0 && nLen < (int) sizeof(m_szResetReason)) {
            nLen += snprintf(&m_szResetReason[nLen], sizeof(m_szResetReason) - nLen, " - stack:");
            for(uint32_t nIdx = 0; nIdx < ulCount && nLen < (int) sizeof(m_szResetReason); nIdx++) {
                nLen += snprintf(&m_szResetReason[nLen], sizeof(m_szResetReason) - nLen,
                                 " 0x%08x", (unsigned) oRecord.ulStackAddr[nIdx]);
            }
        }
    } else if(bHasRecord && oRecord.ulType == RESET_RECORD_REQUESTED && bSoftRestart) {
        snprintf(m_szResetReason, sizeof(m_szResetReason), "%s (%s, after %lus)",
                 pszReason, oRecord.szSource, (unsigned long) oRecord.ulUptimeSec);
    } else if(bSoftRestart) {
        // Restart without record: not by CAppl::reboot() - i.e. ESP.restart() of a library
        snprintf(m_szResetReason, sizeof(m_szResetReason), "%s (unknown source)", pszReason);
    } else {
        snprintf(m_szResetReason, sizeof(m_szResetReason), "%s", pszReason);
    }
    return(m_szResetReason);
}

#pragma endregion

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
    oStatusObj["reset_reason"]      = getResetReason();
    oStatusObj["cpu_clock"]         = getCpuFrequencyMhz();
    oStatusObj["flash_size"]        = getFlashChipSize();
    oStatusObj["flash_real_size"]   = getFlashChipRealSize();    
    oStatusObj["heap_free"]         = getFreeHeap();
    oStatusObj["heap_max"]          = getHeapSize();
    oStatusObj["heap_max_block"]    = getMaxAllocHeap();
    oStatusObj["heap_frag"]         = (unsigned int) getHeapFragmentation();
    oStatusObj["heap_min"]          = getMinFreeHeap();           // minimum free heap since start
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
