#pragma once
#include <AESCryptor.h>


// Lifetime of a user token
#define TOKEN_TIME_ALIVE    (30*60*1000)       // 30 minutes...


/**
 * @brief Access Token to be exchanged with clients, to ensure correct authentication.
 * @todo enhance with application specific meta information
 * 
 */
class CAccessToken {
    CAESCryptor     m_oAESCryptor;
    bool            m_bStructureIsValid = false;
    char            m_szTokenKey[30];   // With buffer to ensure no overflow
    char            m_szIPAddress[30];  // With buffer to ensure no overflow
    unsigned long   m_ulTimeStamp;

    // Elements in the token - representation of states...
    // {"K":"0123456789012345","IP":"192.168.134.122","TS":123456678}
    char            m_szDataElement[(sizeof(m_szTokenKey) + sizeof(m_szIPAddress) + sizeof(m_ulTimeStamp)) + 30];
    char            m_szDataElementAsBase64[(sizeof(m_szDataElement)/ 3 * 4) + 40 ];

    char            m_szTokenAsBase64[((sizeof(m_szDataElementAsBase64) + sizeof(m_szIPAddress))/3 * 4) + 20];
    protected:
        /// @brief Decode and load the base64-encoded token payload.
        bool loadBase64DataElement(const char * pszData);
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
