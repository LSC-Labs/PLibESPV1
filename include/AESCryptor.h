#pragma once

#include <Arduino.h>
#include <Network.h>

// ****************************************************
// The Application security token password
// ... only the first 16 bytes are used (!)
// ****************************************************
#ifndef APPL_SECURITY_TOKEN_PASS
    #define APPL_SECURITY_TOKEN_PASS "Kis8%$vvQ@ä+qw12"
#endif

// Blocksize is 16, as using the bearer implementation 
#define AES_CRYPTOR_BLOCK_SIZE br_aes_big_BLOCK_SIZE 


/**
 * @brief AES Token for IV and Passphrase, used by CAESCryptor
 * 
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
        /// @brief Load token data from a base64-encoded string.
        void loadFromBase64(const char * pszBase64String);
        /// @brief Return token data encoded as base64.
        const char * getAsBase64();
        /// @brief Return token data as a zero-terminated string buffer.
        const char * getAsString();
        /// @brief Return a stable copy of the raw token data.
        void * copyOf();
        /// @brief Fill the token with random bytes.
        void createRandom();
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
    size_t decrypt(void * pData, size_t nDataLen, const char * pszPassphrase  = nullptr );
    /// @brief Encrypt data in place and return the padded/encrypted length.
    size_t encrypt(void * pData, size_t nDataLen, const char * pszPassphrase = nullptr);

    /// @brief Add PKCS#7 padding to a buffer and return the padded length.
    size_t insertPkcs7Padding(unsigned char * pData, size_t nDataLength, size_t nBlockSize = AES_CRYPTOR_BLOCK_SIZE);
  
    /// @brief Remove PKCS#7 padding and update the data length in place.
    void removePkcs7Padding(unsigned char * pData, size_t& nDataLength, size_t nBlockSize = AES_CRYPTOR_BLOCK_SIZE);

};
