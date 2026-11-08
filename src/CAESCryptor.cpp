/**
 * AES encrypt and decrypt classes
 * 
 * usage:
 * 
 * CAESCryptor oAES;
 * 
 */
#include <AESCryptor.h>
#include <DevelopmentHelper.h>
#include <Base64Data.h>
#ifdef ESP32
    #include <mbedtls/aes.h>
    #include <esp_system.h>         // esp_random()
#else
    #include <bearssl/bearssl_block.h>
#endif

/*
 * Notes
 * -----
 * - AES-128-CBC with PKCS#7 padding. CBC only provides confidentiality - use a
 *   MAC (see CAccessToken) if the data must be protected against modification.
 * - Passphrase.loadFromString() uses the first 16 bytes of the string directly
 *   as key (no key derivation) - use 16 random characters for a fixed passphrase.
 * - Random values (IV, default passphrase) come from the hardware RNG.
 */

#ifndef DEBUG_LSC_AESCRYPT
    #undef DEBUGINFOS
#endif


#pragma region AES Token

    /**
     * @brief Creates a random 16-byte AES token.
     */
    CAESToken::CAESToken() {
            createRandom();
        }

    /**
     * @brief Creates a token from a string, padded or truncated to block size.
     */
    CAESToken::CAESToken(const char * pszTokenString) {
            loadFromString(pszTokenString);
        }

    /**
     * @brief Gets the token bytes as a null-terminated string buffer.
     * @return Pointer to an internal buffer.
     */
    const char * CAESToken::getAsString() {
            memset(m_szAsString,'\0',sizeof(m_szAsString));
            memcpy(m_szAsString,(const char *) Data,AES_CRYPTOR_BLOCK_SIZE);
            return(m_szAsString);
        }

    /**
     * @brief Gets the token bytes encoded as base64.
     * @return Pointer to an internal base64 buffer.
     */
    const char * CAESToken::getAsBase64() {
        CBase64Data::base64EncodeData((const char *) Data,sizeof(Data),m_szAsBase64,sizeof(m_szAsBase64),true);
        return(m_szAsBase64);
    }

    /**
     * @brief Loads token bytes from a base64 string.
     * @param pszBase64String Base64 data representing exactly one AES block.
     */
    bool CAESToken::loadFromBase64(const char * pszBase64String) {
        // One block = 24 base64 characters - longer input is invalid (no stack buffer
        // sized by the input, the string may come from a client)
        size_t nInputLen = pszBase64String ? strlen(pszBase64String) : 0;
        if(nInputLen == 0 || nInputLen > 32) return(false);
        char szBuffer[sizeof(Data) + 8] = {0};
        int nSize = CBase64Data::base64DecodeData(pszBase64String, nInputLen, szBuffer, sizeof(szBuffer));
        // Expecting exactly one block (-1 = error)
        if(nSize != (int) sizeof(Data)) {
            DEBUG_INFOS("## ERROR - size of token is not as expected : %d - %d",nSize,(int) sizeof(Data));
            return(false);
        }
        memcpy(Data,szBuffer,sizeof(Data));
        return(true);
    }

    /**
     * @brief Loads token bytes from a plain string.
     *
     * Short strings are padded with zero bytes. Long strings are truncated to
     * AES_CRYPTOR_BLOCK_SIZE.
     */
    void CAESToken::loadFromString(const char * pszTokenString) {
        unsigned int nIdx = 0;
        while(pszTokenString && *pszTokenString && nIdx < sizeof(Data)) {
            Data[nIdx++] = *(pszTokenString++);
        }
        // If string was not long enough, fill the rest with 0...
        while(nIdx < sizeof(Data)) {
            Data[nIdx++] = '\0';
        }
    }

    /**
     * @brief Gets a mutable copy of the token bytes.
     *
     * BearSSL CBC functions modify the IV buffer, so encryption/decryption uses
     * this copy to keep the stored token unchanged.
     */
    void * CAESToken::copyOf() {
            memcpy(m_copyOf,Data,AES_CRYPTOR_BLOCK_SIZE);
            return(m_copyOf);
        }

    /**
     * @brief Fills the token with random bytes.
     *
     * Previously Arduino random() was used: seeded with millis() (predictable)
     * and limited to 42 printable values per byte. Now the full byte range of
     * the hardware RNG is used.
     */
    void CAESToken::createRandom() {
        fillRandom(Data, sizeof(Data));
    }

    /**
     * @brief Fills a buffer with bytes of the hardware random number generator.
     *
     * ESP32: esp_random(), ESP8266: RANDOM_REG32. Both are true random numbers
     * while the radio (WiFi) is active - before that they are pseudo random.
     */
    void CAESToken::fillRandom(unsigned char *pBuffer, size_t nLen) {
        if(!pBuffer) return;
        for(size_t nIdx = 0; nIdx < nLen; nIdx += 4) {
            #ifdef ESP32
                uint32_t ulRandom = esp_random();
            #else
                uint32_t ulRandom = RANDOM_REG32;
            #endif
            for(size_t nByte = 0; nByte < 4 && nIdx + nByte < nLen; nByte++) {
                pBuffer[nIdx + nByte] = (unsigned char) (ulRandom >> (8 * nByte));
            }
        }
    }

#pragma endregion


#pragma region AES Cryptor
/**
 * @brief Decrypts AES-CBC data in place.
 *
 * The IV is copied before the BearSSL call, so the stored IV remains reusable.
 * PKCS#7 padding bytes are replaced with '\0' and nDataLen is reduced to the
 * clear-text length.
 *
 * @param pData Buffer containing encrypted data. It is replaced with clear text.
 * @param nDataLen Encrypted data length. Must be aligned to AES block size.
 * @param pszPassphrase Optional passphrase to load before decrypting.
 * @return Clear-text length after padding removal.
 */
size_t CAESCryptor::decrypt(void * pData, size_t nDataLen, const char * pszPassphrase ) {
        if(pszPassphrase) Passphrase.loadFromString(pszPassphrase);
        // CBC needs complete blocks - anything else is invalid input
        if(!pData || nDataLen == 0 || (nDataLen % AES_CRYPTOR_BLOCK_SIZE) != 0) return(0);

        #ifdef ESP32
            mbedtls_aes_context oCtx;
            mbedtls_aes_init(&oCtx);
            int nRC = mbedtls_aes_setkey_dec(&oCtx, Passphrase.Data, AES_CRYPTOR_BLOCK_SIZE * 8);
            if(nRC == 0) nRC = mbedtls_aes_crypt_cbc(&oCtx, MBEDTLS_AES_DECRYPT, nDataLen,
                                        (unsigned char *) IV.copyOf(),
                                        (const unsigned char *) pData, (unsigned char *) pData);
            mbedtls_aes_free(&oCtx);
            if(nRC != 0) return(0);
        #else
            br_aes_big_cbcdec_keys decCtx;
            br_aes_big_cbcdec_init(&decCtx, Passphrase.Data,AES_CRYPTOR_BLOCK_SIZE);
            br_aes_big_cbcdec_run( &decCtx, IV.copyOf(), pData, nDataLen);
            memset(&decCtx, 0, sizeof(decCtx));     // do not leave the key schedule on the stack
        #endif

        // Invalid padding = wrong key or corrupted data
        if(!removePkcs7Padding((unsigned char *) pData,nDataLen)) return(0);
        return(nDataLen);
    }

/**
 * @brief Encrypts AES-CBC data in place.
 *
 * PKCS#7 padding is inserted before encryption. The caller must allocate enough
 * space for at least one additional block because exact block-size input still
 * receives a full padding block.
 *
 * @param pData Buffer containing clear text. It is replaced with encrypted data.
 * @param nDataLen Clear-text length before padding.
 * @param pszPassphrase Optional passphrase to load before encrypting.
 * @return Encrypted length including padding.
 */
    size_t CAESCryptor::encrypt(void * pData, size_t nDataLen, const char * pszPassphrase) {

        if(pszPassphrase) Passphrase.loadFromString(pszPassphrase);

        if(!pData) return(0);
        size_t nTotalSize = insertPkcs7Padding((unsigned char *) pData,nDataLen);
        #ifdef ESP32
            mbedtls_aes_context oCtx;
            mbedtls_aes_init(&oCtx);
            int nRC = mbedtls_aes_setkey_enc(&oCtx, Passphrase.Data, AES_CRYPTOR_BLOCK_SIZE * 8);
            if(nRC == 0) nRC = mbedtls_aes_crypt_cbc(&oCtx, MBEDTLS_AES_ENCRYPT, nTotalSize,
                                        (unsigned char *) IV.copyOf(),
                                        (const unsigned char *) pData, (unsigned char *) pData);
            mbedtls_aes_free(&oCtx);
            if(nRC != 0) return(0);
        #else
            br_aes_big_cbcenc_keys encCtx;
            br_aes_big_cbcenc_init(&encCtx, Passphrase.Data , AES_CRYPTOR_BLOCK_SIZE);
            br_aes_big_cbcenc_run( &encCtx, IV.copyOf(), pData, nTotalSize);
            memset(&encCtx, 0, sizeof(encCtx));     // do not leave the key schedule on the stack
        #endif

        return(nTotalSize);
    }


    /**
     * @brief Inserts PKCS#7 padding into a buffer.
     * @param pData Buffer with enough free space for padding.
     * @param nDataLength Current data length.
     * @param nBlockSize AES block size, normally 16.
     * @return New length including padding.
     */
    size_t CAESCryptor::insertPkcs7Padding(unsigned char * pData, size_t nDataLength, size_t nBlockSize) {
        size_t nPaddingLen = nBlockSize - (nDataLength % nBlockSize);
        for (size_t i = nDataLength; i < nDataLength + nPaddingLen; i++) {
            pData[i] = nPaddingLen;
        }
        return(nDataLength + nPaddingLen);
    }

    /**
     * @brief Checks and removes PKCS#7 padding from a decrypted buffer.
     *
     * All padding bytes must have the padding length as value (1..nBlockSize).
     * Valid padding bytes are overwritten with '\0' to make decrypted text buffers
     * usable as C strings when the original payload was textual.
     *
     * @param pData Decrypted data buffer.
     * @param nDataLength In/out length. Reduced by the padding length.
     * @param nBlockSize AES block size, normally 16.
     * @return false for invalid padding (wrong key / corrupted data); data and length unchanged.
     */
    bool CAESCryptor::removePkcs7Padding(unsigned char * pData, size_t& nDataLength, size_t nBlockSize) {
        if (!pData || nDataLength == 0) return(false);
        size_t nPaddingLen = pData[nDataLength - 1];
        if (nPaddingLen == 0 || nPaddingLen > nBlockSize || nPaddingLen > nDataLength) return(false);
        // Check every padding byte (without early exit)
        unsigned char cDiff = 0;
        for (size_t nIdx = 0; nIdx < nPaddingLen; nIdx++) {
            cDiff |= (unsigned char) (pData[nDataLength - 1 - nIdx] ^ (unsigned char) nPaddingLen);
        }
        if (cDiff != 0) return(false);
        for (size_t nIdx = 0; nIdx < nPaddingLen; nIdx++) {
            pData[nDataLength - 1 - nIdx] = '\0';
        }
        nDataLength -= nPaddingLen;
        return(true);
    }




#pragma endregion
