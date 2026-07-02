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
        CBase64Data::base64EncodeData((const char *) Data,16,m_szAsBase64,sizeof(m_szAsBase64),true);
        return(m_szAsBase64);
    }

    /**
     * @brief Loads token bytes from a base64 string.
     * @param pszBase64String Base64 data representing exactly one AES block.
     */
    void CAESToken::loadFromBase64(const char * pszBase64String) {
        if(pszBase64String) {
            char szBuffer[strlen(pszBase64String) + 40];
            size_t nSize = CBase64Data::base64DecodeData(pszBase64String,strlen(pszBase64String), szBuffer,sizeof(szBuffer));
            // Expecting the right size (!)
            if(nSize == sizeof(Data)) {
                memcpy(Data,szBuffer,sizeof(Data));
            } else {
                DEBUG_INFOS("## ERROR - size of token is not as expected : %d - %d",nSize,sizeof(Data));
            }
        }
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
     * @brief Fills the token with random printable-ish bytes.
     */
    void CAESToken::createRandom() {
            // The initialization should be done in the main object
            // normaly this is done already in the Appl instance
            // initializeRandomNumbers(false);
            for(unsigned int nIdx = 0; nIdx < sizeof(Data); nIdx++) {
                Data[nIdx] = random(0x30l,0x5al);
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

        br_aes_big_cbcdec_keys decCtx;
        br_aes_big_cbcdec_init(&decCtx, Passphrase.Data,AES_CRYPTOR_BLOCK_SIZE);
        br_aes_big_cbcdec_run( &decCtx, IV.copyOf(), pData, nDataLen);

        removePkcs7Padding((unsigned char *) pData,nDataLen);
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

        size_t nTotalSize = insertPkcs7Padding((unsigned char *) pData,nDataLen);
        br_aes_big_cbcenc_keys encCtx;
        br_aes_big_cbcenc_init(&encCtx, Passphrase.Data , 16);
        br_aes_big_cbcenc_run( &encCtx, IV.copyOf(), pData, nTotalSize);

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
     * @brief Removes PKCS#7 padding from a decrypted buffer.
     *
     * Padding bytes are overwritten with '\0' to make decrypted text buffers
     * usable as C strings when the original payload was textual.
     *
     * @param pData Decrypted data buffer.
     * @param nDataLength In/out length. Reduced by the padding length.
     * @param nBlockSize AES block size, normally 16.
     */
    void CAESCryptor::removePkcs7Padding(unsigned char * pData, size_t& nDataLength, size_t nBlockSize) {
        size_t nPaddingLen = pData[nDataLength - 1];
        if (nPaddingLen <= nBlockSize) {
            // TODO: check if the padding is consistent over whole area (!)
            for (size_t nIdx = 0; nIdx < nPaddingLen; nIdx++) {
                pData[nDataLength -1 - nIdx] = '\0';
            }
            nDataLength -= nPaddingLen;
        }
        
    }




#pragma endregion
