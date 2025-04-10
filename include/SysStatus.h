#pragma once
/**
 * SysStatus.h
 * 
 * (c) 2025 LSC Labs
 * 
 * Aliases functions to sys information
 * Helps to port information between devices and implements the ISatusHandler interface
 * Not all information are available - still porting...
 * 
 * Thanks to the implementation of Jason2866 which gives a lot of basic ideas.
 * 
 * @see https://github.com/Jason2866/ESP32_Show_Info/blob/main/src/Chip_info.ino#L68
 * 
 */
#include <StatusHandler.h>
#include "Arduino.h"
#include "Esp.h"
#include <memory>


#if defined(ARDUINO_ARCH_ESP32)


  #include <soc/efuse_reg.h>


  #include "esp_sleep.h"

  #include <soc/soc.h>
  #include <esp_partition.h>

  extern "C" {
      #include "esp_ota_ops.h"
      #include "esp_image_format.h"
  }

  #include "soc/spi_reg.h"
  #include "esp_system.h"
  #include "esp_chip_info.h"
  #include "esp_mac.h"
  #include "esp_flash.h"

  #ifdef ESP_IDF_VERSION_MAJOR  // IDF 4+
      #if CONFIG_IDF_TARGET_ESP32   // ESP32/PICO-D4
          #include "esp32/rom/spi_flash.h"
          #include "soc/efuse_reg.h"
      #elif CONFIG_IDF_TARGET_ESP32S2
          #include "esp32s2/rom/spi_flash.h"
          #include "soc/efuse_reg.h"
      #elif CONFIG_IDF_TARGET_ESP32S3
          #include "esp32s3/rom/spi_flash.h"
      #elif CONFIG_IDF_TARGET_ESP32C2
          #include "esp32c2/rom/spi_flash.h"
      #elif CONFIG_IDF_TARGET_ESP32C3
          #include "esp32c3/rom/spi_flash.h"
      #elif CONFIG_IDF_TARGET_ESP32C6
          #include "esp32c6/rom/spi_flash.h"
      #elif CONFIG_IDF_TARGET_ESP32H2
          #include "esp32h2/rom/spi_flash.h"
      #else
          #error Target CONFIG_IDF_TARGET is not supported
      #endif
  #else  // ESP32 Before IDF 4.0
      #include "rom/spi_flash.h"
  #endif


  #if CONFIG_IDF_TARGET_ESP32      // ESP32/PICO-D4
    #include "esp32/rom/rtc.h"
  #elif CONFIG_IDF_TARGET_ESP32S2  // ESP32-S2
    #include "esp32s2/rom/rtc.h"
  #elif CONFIG_IDF_TARGET_ESP32S3  // ESP32-S3
    #include "esp32s3/rom/rtc.h"
  #elif CONFIG_IDF_TARGET_ESP32C2  // ESP32-C2
    #include "esp32c2/rom/rtc.h"
  #elif CONFIG_IDF_TARGET_ESP32C3  // ESP32-C3
    #include "esp32c3/rom/rtc.h"
  #elif CONFIG_IDF_TARGET_ESP32C6  // ESP32-C6
    #include "esp32c6/rom/rtc.h"
  #elif CONFIG_IDF_TARGET_ESP32H2  // ESP32-H2
    #include "esp32h2/rom/rtc.h"
  #else
    #error Target CONFIG_IDF_TARGET is not supported
  #endif

  #if CONFIG_IDF_TARGET_ESP32S3 || CONFIG_IDF_TARGET_ESP32C3
    #undef REG_SPI_BASE
    #define REG_SPI_BASE(i)     (DR_REG_SPI1_BASE + (((i)>1) ? (((i)* 0x1000) + 0x20000) : (((~(i)) & 1)* 0x1000 )))
  #endif // TARGET
#else
  // ESP8266
#endif

class CSysStatus : public IStatusHandler {
    private :
        const char *m_pszChipID; // Used as buffer for the chip Name/ID

    public:
      CSysStatus();
      ~CSysStatus();
        const char * getSdkVersion();
        const char * getChipModel();
        uint32_t getFlashChipId();
        uint32_t getFlashChipRealSize();
        uint32_t getFlashChipSize();
        uint32_t getSketchSize();
        uint32_t getFreeSketchSpace();
        uint32_t getHeapSize();
        uint32_t getFreeHeap();
        uint32_t getMaxAllocHeap();
        uint32_t getFreePsram();
        uint32_t getCpuFrequencyMhz();

        void writeStatusTo(JsonObject &oStatusObj) override;

};