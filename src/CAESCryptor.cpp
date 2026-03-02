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

    CAESToken::CAESToken() {
            createRandom();
        }
    CAESToken::CAESToken(const char * pszTokenString) {
            loadFromString(pszTokenString);
        }

    const char * CAESToken::getAsString() {
            memset(m_szAsString,'\0',sizeof(m_szAsString));
            memcpy(m_szAsString,(const char *) Data,AES_CRYPTOR_BLOCK_SIZE);
            return(m_szAsString);
        }

    const char * CAESToken::getAsBase64() {
        CBase64Data::base64EncodeData((const char *) Data,16,m_szAsBase64,sizeof(m_szAsBase64),true);
        return(m_szAsBase64);
    }

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

    void * CAESToken::copyOf() {
            memcpy(m_copyOf,Data,AES_CRYPTOR_BLOCK_SIZE);
            return(m_copyOf);
        }
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
     * @brief decryption of data - with IV and Passphrase already in place
     * Padding will be removed, so the final data size is smaller than the input size.
     * In addition, the padding location will be replaced will '\0', 
     * IV will NOT be modified !
     * 
     * - You have to set IV and Passphrase before calling !
     * 
     * @param pData Pointer to the data to be decrypted
     * @param nDataLen Len of the data to be encrypted
     * @param pszPassphrase pointer to a passphrase to be used (will be aligned to AES_BLOCKSIZE)
     * @return the final data len, without padding structure.
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
     * @brief encryption of data - with IV and Passphrase already in place
     * Padding will be inserted, so ensure, pData has enouth size (!)
     * If data is exactly a multiple of block size (16) insert extra 16 bytes,
     * this is needed to insert the padding.
     * 
     * IV will NOT be modified !
     * 
     * - You have to set IV and Passphrase before calling !
     * 
     * @param pData Pointer to the data to be decrypted (data size needs to be a multiple of 16)
     * @param nDataLen Len of the data to be encrypted
     * @param pszPassphrase pointer to a passphrase to be used (will be aligned to AES_BLOCKSIZE)
     * @return the final data len, with padding structure ( aligned to block size).
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
     * @brief Insert the PKCS7 Padding 
     * - to fill up data until the block end
     * Ensure, enough memory is allocated (!)
     * -> if the datalength hits exact the boundary of the block, an extra block is needed (!)
     * Best is always to allocate one block extra to ensure (!)
     * @param pData Pointer to the data
     * @param nDataLength Length of the data.
     * @param nBlockSize Size of a block (default = 16)
     */
    size_t CAESCryptor::insertPkcs7Padding(unsigned char * pData, size_t nDataLength, size_t nBlockSize) {
        size_t nPaddingLen = nBlockSize - (nDataLength % nBlockSize);
        for (size_t i = nDataLength; i < nDataLength + nPaddingLen; i++) {
            pData[i] = nPaddingLen;
        }
        return(nDataLength + nPaddingLen);
    }

        /**
     * @brief remove the PKCS7 Padding and fill the padding with '\0'
     * @return the data len, without padding structure.
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

