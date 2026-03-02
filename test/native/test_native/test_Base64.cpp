
#include <gtest/gtest.h>
#include <Base64Data.h>


// Test the base encoding / decoding
TEST(CBase64Data,getBase64String) {
    CBase64Data oB64;
    int nFinalLen = 0;
    const char *pszTestString = "Hello world";
    const char *pszExpectedEncodedString = "SGVsbG8gd29ybGQ=";
    const char *pszEncoded = oB64.getBase64EncodedString(pszTestString);
    const char *pszDecoded = (const char *) oB64.getBase64DecodedData(pszEncoded,nFinalLen);
    // Encoded and Decoded has different memory location
    EXPECT_NE(pszEncoded,pszDecoded);
    // Decoded string has correct len and is null terminated...
    EXPECT_EQ(nFinalLen,11);
    EXPECT_EQ(strlen(pszDecoded),11);
    // Decoded String from Encoded String is as the initial teststring
    EXPECT_STREQ(pszTestString,pszDecoded);
    // Encoded Base64 is the expected encoded string
    EXPECT_STREQ(pszExpectedEncodedString,pszEncoded);
}

