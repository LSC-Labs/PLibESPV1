#pragma once

#include <Arduino.h>
#include <Network.h>

// ****************************************************
// The Application security token password
// ... only the first 16 bytes are used (!)
// ****************************************************
#ifndef APPL_SECURITY_TOKEN_PASS
    #define APPL_SECURITY_TOKEN_PASS "Kis8%$vvQ@ä+qw12"
    // The default is public (source code) - access tokens use a random per-boot
    // secret instead (see CAccessToken). Define your own value to use a fixed one.
    #define APPL_SECURITY_TOKEN_PASS_IS_DEFAULT
#endif

// AES block size is always 16 bytes (BearSSL on ESP8266, mbedTLS on ESP32)
#define AES_CRYPTOR_BLOCK_SIZE 16


/**
 * @brief AES Token for IV and Passphrase, used by CAESCryptor
 *
 * Holds one AES block (16 bytes). Random values come from the hardware
 * random number generator (esp_random() / RANDOM_REG32).
 */
class CAESToken {
        
    
    unsigned char  m_copyOf[AES_CRYPTOR_BLOCK_SIZE];
    char m_szAsString[AES_CRYPTOR_BLOCK_SIZE + 2] = {0};
    char m_szAsBase64[AES_CRYPTOR_BLOCK_SIZE * 4] = {0};
    
    public:
        /// @brief Raw 16-byte token data used as AES IV/passphrase material.
        unsigned char  Data[AES_CRYPTOR_BLOCK_SIZE];
        
        /// @brief Create a token initialized with empty/default data.
        CAESToken();

        /// @brief Create a token from a plain string.
        CAESToken(const char * pszTokenString);

        /// @brief Load token data from a plain string, truncated/padded to block size.
        void loadFromString(const char * pszTokenString);
        /// @brief Load token data from a base64-encoded string (exactly one block).
        /// @return false if the string is invalid - Data is unchanged then.
        bool loadFromBase64(const char * pszBase64String);
        /// @brief Return token data encoded as base64.
        const char * getAsBase64();
        /// @brief Return token data as a zero-terminated string buffer.
        const char * getAsString();
        /// @brief Return a stable copy of the raw token data.
        void * copyOf();
        /// @brief Fill the token with random bytes (hardware RNG, full byte range).
        void createRandom();

        /// @brief Fill a buffer with bytes of the hardware random number generator.
        static void fillRandom(unsigned char *pBuffer, size_t nLen);
};

/**
 * @brief Helper class for AES encryption/decryption with PKCS#7 padding.
 */
class CAESCryptor {
    public:
    /// @brief Initialization vector used by encrypt/decrypt.
    CAESToken IV;
    /// @brief Default passphrase used when no explicit passphrase is supplied.
    CAESToken Passphrase;

    /// @brief Create a cryptor with default IV/passphrase tokens.
    CAESCryptor() {}

    /// @brief Decrypt data in place and return the resulting data length.
    /// @return Clear-text length, 0 on error (no complete blocks, invalid padding).
    size_t decrypt(void * pData, size_t nDataLen, const char * pszPassphrase  = nullptr );
    /// @brief Encrypt data in place and return the padded/encrypted length.
    /// @attention pData must have room for nDataLen + AES_CRYPTOR_BLOCK_SIZE bytes (padding).
    size_t encrypt(void * pData, size_t nDataLen, const char * pszPassphrase = nullptr);

    /// @brief Add PKCS#7 padding to a buffer and return the padded length.
    size_t insertPkcs7Padding(unsigned char * pData, size_t nDataLength, size_t nBlockSize = AES_CRYPTOR_BLOCK_SIZE);
  
    /// @brief Check and remove PKCS#7 padding, update the data length in place.
    /// @return false if the padding is invalid (wrong key / corrupted data) - data unchanged then.
    bool removePkcs7Padding(unsigned char * pData, size_t& nDataLength, size_t nBlockSize = AES_CRYPTOR_BLOCK_SIZE);

};
