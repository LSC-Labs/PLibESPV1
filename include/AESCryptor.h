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
        unsigned char  Data[AES_CRYPTOR_BLOCK_SIZE];
        
        CAESToken();

        CAESToken(const char * pszTokenString);

        void loadFromString(const char * pszTokenString);
        void loadFromBase64(const char * pszBase64String);
        const char * getAsBase64();
        const char * getAsString();
        void * copyOf();
        void createRandom();
};

/**
 * @brief Helper class to use the aes encrypt and decrypt function
 */
class CAESCryptor {
    public:
    CAESToken IV;
    CAESToken Passphrase;

    CAESCryptor() {}

    
    size_t decrypt(void * pData, size_t nDataLen, const char * pszPassphrase  = nullptr );
    size_t encrypt(void * pData, size_t nDataLen, const char * pszPassphrase = nullptr);

    size_t insertPkcs7Padding(unsigned char * pData, size_t nDataLength, size_t nBlockSize = AES_CRYPTOR_BLOCK_SIZE);
  
    void removePkcs7Padding(unsigned char * pData, size_t& nDataLength, size_t nBlockSize = AES_CRYPTOR_BLOCK_SIZE);

};