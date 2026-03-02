/**
 * Using the base implmentation of NibbleAndHalf (see base64.h)
 * - wrapping the base implemation to be able to change,
 *   as in the past, a lot of implementations had problems.  
 */
#pragma once
#include <base64.h>
#include <string.h>

class CBase64Data {
    protected:
     char           * m_pszEncodedData = nullptr;
     unsigned char  * m_pDecodedData = nullptr;
    public:
        virtual ~CBase64Data() {
            clear();
        }

        virtual void clear() {
            if(m_pszEncodedData) free(m_pszEncodedData);
            if(m_pDecodedData)   free(m_pDecodedData);
            m_pszEncodedData = nullptr;
            m_pDecodedData = nullptr;
        }

        const char * getBase64EncodedString(const char * pszInput, size_t nInputLen = 0) {
            if(m_pszEncodedData) { free(m_pszEncodedData); m_pszEncodedData = nullptr; }
            int nFinalLen = 0;
            if(nInputLen == 0 && pszInput) nInputLen = strlen(pszInput);
            m_pszEncodedData = base64((const void *) pszInput,nInputLen,&nFinalLen);
            return(m_pszEncodedData);
        }

        const unsigned char * getBase64DecodedData(const char *pszInput, int & nFinalLen) {
            if(m_pDecodedData) { free(m_pDecodedData); m_pDecodedData = nullptr; }
            int nInputLen = strlen(pszInput);
            nFinalLen = 0;
            m_pDecodedData = unbase64(pszInput,nInputLen,&nFinalLen);
            return(m_pDecodedData);
        }

    /**
     * @brief encode data (also bin) to an base 64 string
     * If output area is not large enough to hold the string, nothing will be changed (!)
     * @param pszInput input data to be encode
     * @param nInputLen len of the input data to be encoded
     * @param pszOutput output area, where the string can be written (needs to be large enough to hold the encoded string !)
     * @param nOutputLen size of the output area, to ensure no memory violation.
     * @param bNoCRLF No CRLF inside the generated string
     * @return the final data length, or -1 if output area is too small...
     */
    static int base64EncodeData(const char * pszInput, size_t nInputLen, char * pszOutput, size_t nOutputLen, bool bRemoveCRLF = true) {
        int nFinalLen = -1;
        // Writes a null terminated string... return is the data len...
        const char * pszEncodedData = base64((const void *) pszInput,nInputLen,&nFinalLen);
        if(pszEncodedData && nFinalLen > 0 && nFinalLen < (int) nOutputLen) {
            strcpy(pszOutput,pszEncodedData);
        }
        if(pszEncodedData) free((void*) pszEncodedData);
        return(nFinalLen);
    }  
    /**
     * @brief decode base 64 data to normal representation
     * If output area is not large enough to hold the data, nothing will be changed (!)
     * @param pszInput input string to be decoded
     * @param nInputLen len of the input string to be decoded
     * @param pszOutput output area, where the data can be written (needs to be large enough to hold the encoded string !)
     * @param nOutputLen size of the output area, to ensure no memory violation.
     * @return the final data length, or -1 if output area is too small...
     */
    static int base64DecodeData(const char * pszInput, size_t nInputLen, char * pszOutput, size_t nOutputLen) {
        int nFinalLen = -1;
        // Writes a null terminated string... return is the data len...
        unsigned char * pszDecodedData = unbase64(pszInput,nInputLen,&nFinalLen);
        if(pszDecodedData && nFinalLen > 0 && nFinalLen < (int) nOutputLen) {
            memset(pszOutput,'\0',nOutputLen);
            memcpy(pszOutput,pszDecodedData,nFinalLen);
        }
        if(pszDecodedData) free((void *) pszDecodedData);
        return(nFinalLen);
    }
};



