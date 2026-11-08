#pragma once
#include <AESCryptor.h>


// Lifetime of a user token
#define TOKEN_TIME_ALIVE    (30*60*1000)       // 30 minutes...

// Length of the message authentication code (HMAC-SHA256, truncated) in bytes
#define TOKEN_MAC_SIZE      16


/**
 * @brief Access Token to be exchanged with clients, to ensure correct authentication.
 *
 * Token format (base64 of):
 *      {"IV":"<base64 IV>","Data":"<base64 ciphertext>","MAC":"<base64 MAC>"}
 *  - Data: AES-128-CBC encrypted {"TS":<millis>,"IP":"<client ip>","T":"<token key>"}
 *  - MAC : HMAC-SHA256 over IV || ciphertext, truncated to TOKEN_MAC_SIZE bytes
 *          ("encrypt-then-MAC"). AES-CBC alone only hides the content - without
 *          the MAC a client could flip bits in the IV and change the first
 *          plaintext block (i.e. extend the timestamp) without knowing the key.
 *          The MAC is checked BEFORE decrypting; tokens without valid MAC are rejected.
 *  - Keys: AES key = per-boot random secret (or APPL_SECURITY_TOKEN_PASS),
 *          MAC key = SHA256(AES key || "LSC-AccessToken-MAC") - separate keys
 *          for encryption and authentication.
 * @todo enhance with application specific meta information
 */
class CAccessToken {
    CAESCryptor     m_oAESCryptor;
    bool            m_bStructureIsValid = false;
    char            m_szTokenKey[30]  = {0};   // With buffer to ensure no overflow
    char            m_szIPAddress[30] = {0};   // With buffer to ensure no overflow
    unsigned long   m_ulTimeStamp     = 0;
    unsigned char   m_tMacKey[32]     = {0};   // HMAC key, derived from the AES key
    char            m_szMacAsBase64[(TOKEN_MAC_SIZE + 2) / 3 * 4 + 4] = {0};

    // Elements in the token - representation of states...
    // {"K":"0123456789012345","IP":"192.168.134.122","TS":123456678}
    char            m_szDataElement[(sizeof(m_szTokenKey) + sizeof(m_szIPAddress) + sizeof(m_ulTimeStamp)) + 30] = {0};
    char            m_szDataElementAsBase64[(sizeof(m_szDataElement)/ 3 * 4) + 40 ] = {0};

    // JSON wrapper (IV + Data + MAC) as base64
    char            m_szTokenAsBase64[((sizeof(m_szDataElementAsBase64) + sizeof(m_szMacAsBase64) + sizeof(m_szIPAddress))/3 * 4) + 40] = {0};

    /// @brief Load the passphrase used for tokens (per-boot secret or APPL_SECURITY_TOKEN_PASS).
    void loadPassphrase();
    protected:
        /// @brief Verify the MAC, then decode and load the base64-encoded token payload.
        bool loadBase64DataElement(const char * pszData, const char * pszMacBase64);
        /// @brief Compute the (truncated) HMAC over IV || ciphertext.
        void computeMac(const unsigned char *pCipher, size_t nCipherLen, unsigned char *pMacOut);
        /// @brief Return the base64-encoded payload, optionally refreshing timestamp.
        const char * getBase64DataElement(bool bRefreshTimeStamp = true);

    public:
        /// @brief Create an empty invalid access token.
        CAccessToken();
        /// @brief Create a token for the given IP address and token key.
        CAccessToken(const char * pszIPAddress, const char * pszTokenKey, const char * pszIV = nullptr);
        /// @brief Create a token by decoding an existing base64 token string.
        CAccessToken(const char * pszBase64EncodedString);

        /// @brief Return the complete access token encoded as base64.
        const char * getTokenAsBase64(bool bRandomIV = true);
        /// @brief Validate token structure, IP address, key and lifetime.
        bool isAuthValid(const char * pszIPAddress, const char * pszTokenKey);
};
