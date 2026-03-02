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
        bool loadBase64DataElement(const char * pszData);
        const char * getBase64DataElement(bool bRefreshTimeStamp = true);

    public:
        CAccessToken();
        CAccessToken(const char * pszIPAddress, const char * pszTokenKey, const char * pszIV = nullptr);
        CAccessToken(const char * pszBase64EncodedString);

        const char * getTokenAsBase64(bool bRandomIV = true);
        bool isAuthValid(const char * pszIPAddress, const char * pszTokenKey);
        void dump();

};

