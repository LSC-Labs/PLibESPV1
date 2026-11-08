#include <AccessToken.h>
#include <DevelopmentHelper.h>
#include <JsonNode.h>
#include <Base64Data.h>
#ifndef DEBUG_LSC_SECURITY
    #undef DEBUGINFOS
#endif
#ifdef ESP32
    #include <mbedtls/md.h>     // HMAC / SHA256 (mbedTLS)
#else
    #include <bearssl/bearssl_hash.h>   // SHA256 (BearSSL)
    #include <bearssl/bearssl_hmac.h>   // HMAC   (BearSSL)
#endif

/*
 * Security notes
 * --------------
 * - Encryption (AES-CBC) keeps the token content (IP, key, timestamp) secret.
 * - Authentication (HMAC-SHA256, encrypt-then-MAC) ensures the token was created
 *   by this device and was not modified. Both are needed: CBC is malleable - flipping
 *   a bit in the IV flips the same bit in the first plaintext block.
 * - The MAC is verified before anything is decrypted, with a constant time compare.
 * - Encryption and MAC use different keys (MAC key derived from the AES key).
 */

namespace {
    #ifdef APPL_SECURITY_TOKEN_PASS_IS_DEFAULT
        /// @brief Random secret, created once per boot (thread safe static initialization).
        struct CSessionSecret {
            unsigned char Data[AES_CRYPTOR_BLOCK_SIZE];
            CSessionSecret() { CAESToken::fillRandom(Data, sizeof(Data)); }
        };
        const unsigned char * getSessionSecret() {
            static CSessionSecret s_oSecret;
            return(s_oSecret.Data);
        }
    #endif

    /// @brief SHA256 of a buffer (platform crypto library).
    void sha256(const unsigned char *pData, size_t nLen, unsigned char *pOut32) {
        #ifdef ESP32
            mbedtls_md(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), pData, nLen, pOut32);
        #else
            br_sha256_context oCtx;
            br_sha256_init(&oCtx);
            br_sha256_update(&oCtx, pData, nLen);
            br_sha256_out(&oCtx, pOut32);
        #endif
    }

    /// @brief HMAC-SHA256 over two concatenated buffers (pData1 || pData2).
    void hmacSha256(const unsigned char *pKey, size_t nKeyLen,
                    const unsigned char *pData1, size_t nLen1,
                    const unsigned char *pData2, size_t nLen2,
                    unsigned char *pOut32) {
        #ifdef ESP32
            mbedtls_md_context_t oCtx;
            mbedtls_md_init(&oCtx);
            if(mbedtls_md_setup(&oCtx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 1) == 0) {
                mbedtls_md_hmac_starts(&oCtx, pKey, nKeyLen);
                mbedtls_md_hmac_update(&oCtx, pData1, nLen1);
                mbedtls_md_hmac_update(&oCtx, pData2, nLen2);
                mbedtls_md_hmac_finish(&oCtx, pOut32);
            } else {
                memset(pOut32, 0, 32);      // no MAC possible => verification fails
            }
            mbedtls_md_free(&oCtx);
        #else
            br_hmac_key_context oKeyCtx;
            br_hmac_context     oCtx;
            br_hmac_key_init(&oKeyCtx, &br_sha256_vtable, pKey, nKeyLen);
            br_hmac_init(&oCtx, &oKeyCtx, 0);
            br_hmac_update(&oCtx, pData1, nLen1);
            br_hmac_update(&oCtx, pData2, nLen2);
            br_hmac_out(&oCtx, pOut32);
        #endif
    }

    /// @brief Byte compare with a runtime independent of the first difference.
    bool constantTimeEqualsBytes(const unsigned char *pA, const unsigned char *pB, size_t nLen) {
        unsigned char cDiff = 0;
        for(size_t nIdx = 0; nIdx < nLen; nIdx++) cDiff |= (unsigned char) (pA[nIdx] ^ pB[nIdx]);
        return(cDiff == 0);
    }

    /// @brief String compare with a runtime independent of the first difference.
    bool constantTimeEquals(const char *pszA, const char *pszB) {
        size_t nLenA = strlen(pszA);
        size_t nLenB = strlen(pszB);
        unsigned char cDiff = (unsigned char) (nLenA != nLenB);
        for(size_t nIdx = 0; nIdx < nLenA; nIdx++) {
            cDiff |= (unsigned char) (pszA[nIdx] ^ (nIdx < nLenB ? pszB[nIdx] : 0));
        }
        return(cDiff == 0);
    }
}

/**
 * @brief Loads the token passphrase (AES key) and derives the MAC key.
 *
 * With the public default APPL_SECURITY_TOKEN_PASS a random secret is used,
 * created once per boot from the hardware RNG - tokens cannot be forged with
 * the source code, and they become invalid on reboot (millis() restarts too).
 * A project specific APPL_SECURITY_TOKEN_PASS is used as before.
 *
 * MAC key = SHA256(AES key || "LSC-AccessToken-MAC"): a separate key for the
 * authentication, without storing a second secret.
 */
void CAccessToken::loadPassphrase() {
    #ifdef APPL_SECURITY_TOKEN_PASS_IS_DEFAULT
        memcpy(m_oAESCryptor.Passphrase.Data, getSessionSecret(), AES_CRYPTOR_BLOCK_SIZE);
    #else
        m_oAESCryptor.Passphrase.loadFromString(APPL_SECURITY_TOKEN_PASS);
    #endif
    static const char szLabel[] = "LSC-AccessToken-MAC";
    unsigned char tKeyMaterial[AES_CRYPTOR_BLOCK_SIZE + sizeof(szLabel)];
    memcpy(tKeyMaterial, m_oAESCryptor.Passphrase.Data, AES_CRYPTOR_BLOCK_SIZE);
    memcpy(tKeyMaterial + AES_CRYPTOR_BLOCK_SIZE, szLabel, sizeof(szLabel));
    sha256(tKeyMaterial, sizeof(tKeyMaterial), m_tMacKey);
    memset(tKeyMaterial, 0, sizeof(tKeyMaterial));     // do not leave key material on the stack
}

/**
 * @brief Computes the token MAC: HMAC-SHA256(MAC key, IV || ciphertext), truncated.
 *
 * The IV is part of the MAC - otherwise the first plaintext block could be
 * changed by modifying the IV. Truncation to TOKEN_MAC_SIZE (128 bit) keeps
 * the token short and is still far beyond brute force.
 *
 * @param pCipher    Encrypted data element (ciphertext).
 * @param nCipherLen Length of the ciphertext.
 * @param pMacOut    Receives TOKEN_MAC_SIZE bytes.
 */
void CAccessToken::computeMac(const unsigned char *pCipher, size_t nCipherLen, unsigned char *pMacOut) {
    unsigned char tFullMac[32];
    hmacSha256(m_tMacKey, sizeof(m_tMacKey),
               m_oAESCryptor.IV.Data, AES_CRYPTOR_BLOCK_SIZE,
               pCipher, nCipherLen,
               tFullMac);
    memcpy(pMacOut, tFullMac, TOKEN_MAC_SIZE);
}

/**
 * @brief Creates an empty, invalid access token.
 */
CAccessToken::CAccessToken() {
    loadPassphrase();
}

/**
 * @brief Creates a token structure from client data.
 *
 * The constructor copies IP address and token key into fixed buffers, optionally
 * loads an IV, and stamps the token with the current millis() value.
 *
 * @param pszIPAddress Client IP address to bind the token to.
 * @param pszTokenKey Application token key to bind the token to.
 * @param pszIV Optional initialization vector for encryption/decryption.
 */
CAccessToken::CAccessToken(const char * pszIPAddress, const char * pszTokenKey, const char * pszIV) {
    loadPassphrase();
    if(pszIV) m_oAESCryptor.IV.loadFromString(pszIV);
    m_ulTimeStamp = millis();
    strncpy(m_szIPAddress,pszIPAddress ? pszIPAddress : "",sizeof(m_szIPAddress) - 1);
    strncpy(m_szTokenKey,pszTokenKey ? pszTokenKey : "",sizeof(m_szTokenKey) - 1);
    m_bStructureIsValid = pszIPAddress && pszTokenKey;
}

/**
 * @brief Decodes a serialized base64 token.
 *
 * The decoded token must contain the JSON fields "IV", "Data" and "MAC". The IV
 * is loaded first, then the MAC over IV || ciphertext is verified and only a
 * correctly authenticated data element is decrypted.
 *
 * @param pszBase64Data Base64-encoded token JSON wrapper.
 */
CAccessToken::CAccessToken(const char * pszBase64Data) {
    DEBUG_FUNC_START();
    loadPassphrase();
    m_bStructureIsValid = false;
    // The token comes from the client - limit the size (no large stack buffers)
    size_t nInputLen = pszBase64Data ? strlen(pszBase64Data) : 0;
    if(nInputLen > 0 && nInputLen < sizeof(m_szTokenAsBase64)) {
        // Decode the Base 64 string to IV/Data elements (decoded data is smaller than the input)
        char szBuffer[sizeof(m_szTokenAsBase64)] = {0};
        int nDecoded = CBase64Data::base64DecodeData(pszBase64Data,nInputLen,szBuffer,sizeof(szBuffer));
        if(nDecoded <= 0 || nDecoded >= (int) sizeof(szBuffer)) szBuffer[0] = '\0';
        CJsonNode oToken;
        oToken.parse(szBuffer);
        // Tokens without MAC (older format) are rejected
        if(oToken.exists("IV") && oToken.exists("Data") && oToken.exists("MAC")) { 
            if(m_oAESCryptor.IV.loadFromBase64(oToken.getValue("IV"))) {
                loadBase64DataElement(oToken.getValue("Data"), oToken.getValue("MAC"));
            }
        } else {
            DEBUG_INFO("## Error missing json elements..");
        }
    }
    DEBUG_FUNC_END();
}

/**
 * @brief Verifies the MAC, then decrypts and loads the token data element.
 *
 * Order (encrypt-then-MAC):
 *  1. base64-decode ciphertext and MAC
 *  2. verify HMAC(IV || ciphertext) with a constant time compare
 *  3. only then decrypt and parse {"TS","IP","T"}
 * A modified token is rejected before the decryption (no padding oracle, no
 * parsing of attacker controlled plaintext).
 *
 * @param pszBase64Data Base64-encoded encrypted data element.
 * @param pszMacBase64  Base64-encoded MAC of the token.
 * @return true when the MAC is valid and all required fields were parsed.
 */
bool CAccessToken::loadBase64DataElement(const char * pszBase64Data, const char * pszMacBase64) {
    bool bSuccess = false;
    if(pszBase64Data && pszMacBase64) {
        // Decode the base64 string... and decrypt the block
        // + one AES block: the encrypted data contains the PKCS#7 padding
        char szBuffer[sizeof(m_szDataElement) + AES_CRYPTOR_BLOCK_SIZE + 1] = {0};
        int nDecoded = CBase64Data::base64DecodeData(pszBase64Data,strlen(pszBase64Data),szBuffer,sizeof(szBuffer));
        // -1 (error), too large for the buffer (nothing copied) or no complete AES blocks => invalid.
        // Without this check decrypt() would run over the end of the stack buffer.
        if(nDecoded <= 0 || nDecoded >= (int) sizeof(szBuffer) || (nDecoded % AES_CRYPTOR_BLOCK_SIZE) != 0) {
            DEBUG_INFO("## Error invalid data element");
            return(false);
        }
        // Verify the MAC before decrypting
        char szMacBuffer[TOKEN_MAC_SIZE + 8] = {0};
        int nMacLen = CBase64Data::base64DecodeData(pszMacBase64,strlen(pszMacBase64),szMacBuffer,sizeof(szMacBuffer));
        unsigned char tExpectedMac[TOKEN_MAC_SIZE];
        computeMac((const unsigned char *) szBuffer, (size_t) nDecoded, tExpectedMac);
        if(nMacLen != TOKEN_MAC_SIZE ||
           !constantTimeEqualsBytes((const unsigned char *) szMacBuffer, tExpectedMac, TOKEN_MAC_SIZE)) {
            DEBUG_INFO("## Error token MAC invalid - token modified or not created by this device");
            return(false);
        }
        size_t nDataLen = m_oAESCryptor.decrypt(szBuffer,(size_t) nDecoded);     // passphrase loaded in the constructor
        if(nDataLen == 0) return(false);
        szBuffer[nDataLen] = '\0';
        // Store the clear text strings into my object members...
        strncpy(m_szDataElementAsBase64,pszBase64Data,sizeof(m_szDataElementAsBase64) - 1);
        m_szDataElementAsBase64[sizeof(m_szDataElementAsBase64) - 1] = '\0';
        strncpy(m_szDataElement,szBuffer,sizeof(m_szDataElement) - 1);
        m_szDataElement[sizeof(m_szDataElement) - 1] = '\0';
        DEBUG_INFOS("--> parsing %s",m_szDataElement);
        CJsonNode oToken;
        oToken.parse(m_szDataElement);
        if( oToken.exists("TS") && oToken.exists("IP") && oToken.exists("T") ) {
            m_ulTimeStamp = strtoul(oToken.getValue("TS"),0,10);
            strncpy(m_szIPAddress,oToken.getValue("IP",""),sizeof(m_szIPAddress) - 1);
            m_szIPAddress[sizeof(m_szIPAddress) - 1] = '\0';
            strncpy(m_szTokenKey,oToken.getValue("T",""),sizeof(m_szTokenKey) - 1);
            m_szTokenKey[sizeof(m_szTokenKey) - 1] = '\0';
            m_bStructureIsValid = true;
            bSuccess = true;
        } else {
            DEBUG_INFO("## Error reading json, missing elements..");
        }
    }
    return(bSuccess);       
}

/**
 * @brief Builds the encrypted data element and returns it as base64.
 * @param bRefreshTimeStamp true to update the token timestamp before encoding.
 * @return Pointer to the internal base64 data-element buffer.
 */
const char * CAccessToken::getBase64DataElement(bool bRefreshTimeStamp) {
    // Build the json structure by string...   
    if(bRefreshTimeStamp) m_ulTimeStamp = millis();
    snprintf(m_szDataElement, sizeof(m_szDataElement),
                "{\"TS\":%lu,"
                "\"IP\":\"%s\","
                "\"T\":\"%s\"}",
                m_ulTimeStamp,
                m_szIPAddress,
                m_szTokenKey);
    // make a copy of the data, to encrypt this block (room for the padding block)
    char szDataBlockBuffer[sizeof(m_szDataElement) + AES_CRYPTOR_BLOCK_SIZE] = {0};
    size_t nDataLen = strlen(m_szDataElement);
    memcpy(szDataBlockBuffer,m_szDataElement,nDataLen);
    int nFinalDataLen = m_oAESCryptor.encrypt(szDataBlockBuffer,nDataLen);     // passphrase loaded in the constructor
    // MAC over IV || ciphertext (encrypt-then-MAC) - the IV must be set before this call
    unsigned char tMac[TOKEN_MAC_SIZE];
    computeMac((const unsigned char *) szDataBlockBuffer, (size_t) nFinalDataLen, tMac);
    CBase64Data::base64EncodeData((const char *) tMac, TOKEN_MAC_SIZE, m_szMacAsBase64, sizeof(m_szMacAsBase64), true);
    // Convert the data block to Base64
    CBase64Data::base64EncodeData(szDataBlockBuffer,nFinalDataLen,m_szDataElementAsBase64,sizeof(m_szDataElementAsBase64),true);
    return(m_szDataElementAsBase64);
}

/**
 * @brief Builds the full access token as a base64 string.
 *
 * The full token is a JSON wrapper containing the IV, the encrypted data element
 * and the MAC over both, then encoded as base64 for transport.
 *
 * @param bRandomIV true to create a new IV before encoding.
 * @return Pointer to the internal base64 token buffer.
 */
const char * CAccessToken::getTokenAsBase64(bool bRandomIV) {
    DEBUG_FUNC_START_PARMS("%d",bRandomIV);
    // IV from the hardware RNG (Arduino random() is predictable - seeded with millis())
    if(bRandomIV) m_oAESCryptor.IV.createRandom();
    const char * szData = getBase64DataElement();
    const char * szIV = m_oAESCryptor.IV.getAsBase64();
    char szTokenBuffer[sizeof(m_szDataElementAsBase64) + sizeof(m_szMacAsBase64) + 64];
    DEBUG_INFOS("-- IV             size == %d",strlen(szIV));
    DEBUG_INFOS("-- Data           size == %d",strlen(szData));
    // MAC (computed in getBase64DataElement()) authenticates IV and Data
    snprintf(szTokenBuffer, sizeof(szTokenBuffer),
            "{\"IV\":\"%s\",\"Data\":\"%s\",\"MAC\":\"%s\"}",
            szIV,
            szData,
            m_szMacAsBase64);
    DEBUG_INFOS("-- szTokenBuffer final == %d",strlen(szTokenBuffer));
    DEBUG_INFOS("-- szToken       size  == %d",sizeof(m_szTokenAsBase64));
    CBase64Data::base64EncodeData(szTokenBuffer,strlen(szTokenBuffer),m_szTokenAsBase64,sizeof(m_szTokenAsBase64),true);
    DEBUG_INFOS("-- szToken.      final == %d",strlen(m_szTokenAsBase64));
    DEBUG_FUNC_END_PARMS("%s",m_szTokenAsBase64);
    return(m_szTokenAsBase64);
}

/*
void CAccessToken::dump() {
    Serial.println("");
    Serial.println("Access Token:");
    Serial.println("=============");
    Serial.printf("Timestamp             >>%lu<<\n",m_ulTimeStamp);
    Serial.printf("IP Address     (%d/%d)>>%s<<\n",sizeof(m_szIPAddress),strlen(m_szIPAddress), m_szIPAddress);
    Serial.printf("Token key      (%d/%d)>>%s<<\n",sizeof(m_szTokenKey), strlen(m_szTokenKey),m_szTokenKey);
    Serial.printf("Data element   (%d/%d)>>%s<<\n",sizeof(m_szDataElement), strlen(m_szDataElement),m_szDataElement);
    Serial.printf("Data element64 (%d/%d)>>%s<<\n",sizeof(m_szDataElementAsBase64), strlen(m_szDataElementAsBase64),m_szDataElementAsBase64);
    Serial.printf("IV                    >>%s<<\n",m_oAESCryptor.IV.getAsString());
    Serial.printf("IV (Base64)           >>%s<<\n",m_oAESCryptor.IV.getAsBase64());
    Serial.printf("Token (Base64) (%d/%d)>>%s<<\n",sizeof(m_szTokenAsBase64),strlen(m_szTokenAsBase64),m_szTokenAsBase64);
    Serial.println("---------------------------------------------------------------");
}
*/

/**
 * @brief Checks whether this token is valid for a client and token key.
 *
 * A token is valid only when its structure decoded successfully, the IP address
 * matches, the token key matches, and the timestamp is still within
 * TOKEN_TIME_ALIVE.
 *
 * @param pszIPAddress Client IP address to validate.
 * @param pszTokenKey Application token key to validate.
 * @return true when all checks pass.
 */
bool CAccessToken::isAuthValid(const char * pszIPAddress, const char * pszTokenKey) {
    DEBUG_FUNC_START_PARMS("\"%s\",\"%s\"",NULL_POINTER_STRING(pszIPAddress),NULL_POINTER_STRING(pszTokenKey));
    bool bIsValid = false;
    DEBUG_INFOS("Structure is valid : %d\n",m_bStructureIsValid);
    if(m_bStructureIsValid && pszIPAddress && pszTokenKey ) {
        DEBUG_INFOS("TS %lu (age %lu ms)",m_ulTimeStamp, millis() - m_ulTimeStamp);
        DEBUG_INFOS("IP %s\t == %s",pszIPAddress, m_szIPAddress);
        DEBUG_INFOS("K  %s\t == %s",pszTokenKey, m_szTokenKey);
        // Elapsed time (overflow safe). A timestamp "in the future" (i.e. from
        // before a reboot) results in a huge elapsed time and is rejected.
        unsigned long ulAge = millis() - m_ulTimeStamp;
        bool bIPOk  = constantTimeEquals(pszIPAddress, m_szIPAddress);
        bool bKeyOk = constantTimeEquals(pszTokenKey,  m_szTokenKey);
        bIsValid = (ulAge <= (unsigned long) TOKEN_TIME_ALIVE) && bIPOk && bKeyOk;
    }
    DEBUG_FUNC_END_PARMS("%d",bIsValid);
    return(bIsValid);
}
