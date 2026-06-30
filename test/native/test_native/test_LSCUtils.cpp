
#include <gtest/gtest.h>
#include "LSCUtils.h"

#pragma region String tests

TEST(LSCUtils,testStringCompares) {
    EXPECT_EQ(LSC::stricmp("This is Zulu","this is zulu"),0);
    EXPECT_NE(LSC::stricmp("This is Zulu","this is zulu2"),0);
}

TEST(LSCUtils,testSpecialCompares) {
    EXPECT_NE(LSC::stricmp("this is true","this no true"),0);
}

TEST(LSCUtils,testIsPositiveNumber) {
    EXPECT_TRUE(LSC::isNumber("55"));
    EXPECT_TRUE(LSC::isNumber("5.54"));
    EXPECT_FALSE(LSC::isNumber("otto"));
}

TEST(LSCUtils,testIsNegativeNumber) {
    EXPECT_TRUE(LSC::isNumber("-55"));
    EXPECT_TRUE(LSC::isNumber("-5.6"));
}

TEST(LSCUtils,testIsNotValidNumber) {
    EXPECT_FALSE(LSC::isNumber("otto"));
    EXPECT_FALSE(LSC::isNumber("+#"));
}

TEST(LSCUtils,testIsNotValidNumberWithoutDigits) {
    EXPECT_FALSE(LSC::isNumber(""));
    EXPECT_FALSE(LSC::isNumber("+"));
    EXPECT_FALSE(LSC::isNumber("-"));
    EXPECT_FALSE(LSC::isNumber("."));
    EXPECT_FALSE(LSC::isNumber("+."));
    EXPECT_FALSE(LSC::isNumber("-."));
}

TEST(LSCUtils,testIsNotValidNumberWithMultipleDecimalPoints) {
    EXPECT_FALSE(LSC::isNumber("5.5.5"));
    EXPECT_FALSE(LSC::isNumber("..5"));
}

TEST(LSCUtils,testIndexOfMiddle) {
    const char *pszTest = "Sub.Data.1";
    int nExpected = 3;
    int nIdx = LSC::indexOf(pszTest,'.');
    ASSERT_EQ(nIdx,nExpected);
}

TEST(LSCUtils,testIndexOfAtTheEnd) {
    const char *pszTest = "Sub.Data.1";
    int nExpected = 9;
    int nIdx = LSC::indexOf(pszTest,'1');
    ASSERT_EQ(nIdx,nExpected);
}


TEST(LSCUtils,testNotInIndexOf) {
    const char *pszTest = "Sub.Data.1";
    int nExpected = -1;
    int nIdx = LSC::indexOf(pszTest,'F');
    ASSERT_EQ(nIdx,nExpected);
}

TEST(LSCUtils,testIndexOfNullAndEmptyString) {
    EXPECT_EQ(LSC::indexOf(nullptr,'F'),-1);
    EXPECT_EQ(LSC::indexOf("",'F'),-1);
}

TEST(LSCUtils,testLastIndexOfDeli) {
    const char *pszTest = "Sub.Data.1";
    int nExpected = 8;
    int nIdx = LSC::lastIndexOf(pszTest,'.');
    ASSERT_EQ(nIdx,nExpected);
}
TEST(LSCUtils,testLastIndexOfDouble) {
    const char *pszTest = "Sub.Data.1";
    int nExpected = 7;
    int nIdx = LSC::lastIndexOf(pszTest,'a');
    ASSERT_EQ(nIdx,nExpected);
}
TEST(LSCUtils,testLastIndexOfEnd) {
    const char *pszTest = "Sub.Data.1";
    int nExpected = 9;
    int nIdx = LSC::lastIndexOf(pszTest,'1');
    ASSERT_EQ(nIdx,nExpected);
}
TEST(LSCUtils,testLastIndexOfStart) {
    const char *pszTest = "Sub.Data.1";
    int nExpected = 0;
    int nIdx = LSC::lastIndexOf(pszTest,'S');
    ASSERT_EQ(nIdx,nExpected);
}

TEST(LSCUtils,testLastIndexOfNullAndEmptyString) {
    EXPECT_EQ(LSC::lastIndexOf(nullptr,'F'),-1);
    EXPECT_EQ(LSC::lastIndexOf("",'F'),-1);
}


#pragma endregion

#pragma region True/False tests

// Test: Test "true" 
TEST(LSCUtils,testValueStringTrue) {
    EXPECT_TRUE(LSC::isTrueValue("true"));
    EXPECT_TRUE(LSC::isTrueValue("tRue"));
    EXPECT_FALSE(LSC::isTrueValue("false"));
}

// Test: Test "true" 
TEST(LSCUtils,testTrueValueSigns) {
    EXPECT_TRUE(LSC::isTrueValue("1"));
    EXPECT_TRUE(LSC::isTrueValue("+"));
}

// Test: Test "true" 
TEST(LSCUtils,testTrueValueStringYes) {
    EXPECT_TRUE(LSC::isTrueValue("yes"));
    EXPECT_TRUE(LSC::isTrueValue("Y"));
    EXPECT_TRUE(LSC::isTrueValue("yeS"));
    EXPECT_TRUE(LSC::isTrueValue("On"));
}

TEST(LSCUtils,testTrueNotExplicit) {
    EXPECT_TRUE(LSC::isTrueValue("yes", false));
    EXPECT_TRUE(LSC::isTrueValue("otto",false));
    EXPECT_FALSE(LSC::isTrueValue("Off",false));
}

// Test: Test "false" 
TEST(LSCUtils,testFalseValueSigns) {
    EXPECT_TRUE(LSC::isFalseValue("0"));
    EXPECT_TRUE(LSC::isFalseValue("-"));
}
// Test: Test "false" 
TEST(LSCUtils,testFalseValueStrings) {
    EXPECT_TRUE(LSC::isFalseValue("false"));
    EXPECT_TRUE(LSC::isFalseValue("No"));
    EXPECT_TRUE(LSC::isFalseValue("n"));
    EXPECT_TRUE(LSC::isFalseValue("Off"));
}

#pragma endregion

#pragma region White space tests

TEST(LSCUtils,testIsWhiteSpaceChar) {
    EXPECT_TRUE(LSC::isWhite(' '));
    EXPECT_TRUE(LSC::isWhite('\t'));
    EXPECT_TRUE(LSC::isWhite('\v'));
    EXPECT_TRUE(LSC::isWhite('\f'));
    EXPECT_TRUE(LSC::isWhite('\r'));
    EXPECT_TRUE(LSC::isWhite('\n'));
}

TEST(LSCUtils,testIsNotWhiteSpace) {
    EXPECT_FALSE(LSC::isWhite('.'));
    EXPECT_FALSE(LSC::isWhite('z'));
    EXPECT_FALSE(LSC::isWhite('#'));
}

TEST(LSCUtils,testSkipWhiteSpaces) {
    char szData[] = "\t \f\r #Data";
    const char *psz = LSC::skipWhite(szData);
    EXPECT_EQ(*psz,'#');
}

TEST(LSCUtils,testSkipWhiteNullPointer) {
    EXPECT_EQ(LSC::skipWhite(nullptr),nullptr);
}

TEST(LSCUtils,testSkipWhiteOnlyWhiteSpaces) {
    char szData[] = " \t\r\n";
    const char *psz = LSC::skipWhite(szData);
    EXPECT_EQ(*psz,'\0');
}

#pragma endregion

#pragma region parse byte tests

TEST(LSCUtils,testParseToByteArray) {
    char szData[] = "127.128.99.5";
    uint8_t bData[8];
    int nSize = LSC::parseBytesToArray(bData,szData,'.',sizeof(bData),10);
    EXPECT_EQ(nSize,4);
    EXPECT_EQ(bData[0],127);
    EXPECT_EQ(bData[1],128);
    EXPECT_EQ(bData[2],99);
    EXPECT_EQ(bData[3],5);
}

TEST(LSCUtils,testParseToByteArrayStopsAtMaxBytes) {
    char szData[] = "1.2.3.4";
    uint8_t bData[2];
    int nSize = LSC::parseBytesToArray(bData,szData,'.',sizeof(bData),10);
    EXPECT_EQ(nSize,2);
    EXPECT_EQ(bData[0],1);
    EXPECT_EQ(bData[1],2);
}

TEST(LSCUtils,testParseHexToByteArray) {
    char szData[] = "0A:ff:10";
    uint8_t bData[4];
    int nSize = LSC::parseBytesToArray(bData,szData,':',sizeof(bData),16);
    EXPECT_EQ(nSize,3);
    EXPECT_EQ(bData[0],10);
    EXPECT_EQ(bData[1],255);
    EXPECT_EQ(bData[2],16);
}

#pragma endregion

#pragma region Date functions

TEST(LSCUtils,testGetIsoDateFunction) {
    char szTimeBuffer[256];
    time_t oNativeTime;
    time(&oNativeTime);    
    LSC::getISODateTime(oNativeTime,szTimeBuffer,sizeof(szTimeBuffer));
    
    struct tm* oTimeInfo = localtime(&oNativeTime);
    char szCheckBuffer[32];


    EXPECT_EQ(szTimeBuffer[4], '-');
    EXPECT_EQ(szTimeBuffer[7], '-');
    EXPECT_EQ(szTimeBuffer[10],'T');
    EXPECT_EQ(szTimeBuffer[13],':');
    EXPECT_EQ(szTimeBuffer[16],':');

    snprintf(   szCheckBuffer,
                sizeof(szCheckBuffer),"%04d-%02d-%02dT%02d:%02d:",
                oTimeInfo->tm_year + 1900,
                oTimeInfo->tm_mon  + 1,
                oTimeInfo->tm_mday,
                oTimeInfo->tm_hour,
                oTimeInfo->tm_min
            );
    EXPECT_EQ(strncmp(szCheckBuffer,szTimeBuffer,17),0);

}

#pragma endregion


TEST(LSCUtils,testGetFarenheitFromCelsius) {
    float fResult = LSC::getFarenheitFromCelsius(0);
    ASSERT_EQ(32.0, fResult);
}

TEST(LSCUtils,testGetFarenheitFromCelsiusBoilingPoint) {
    float fResult = LSC::getFarenheitFromCelsius(100);
    ASSERT_FLOAT_EQ(212.0, fResult);
}

TEST(LSCUtils,testGetFarenheitFromCelsiusNegativeForty) {
    float fResult = LSC::getFarenheitFromCelsius(-40);
    ASSERT_FLOAT_EQ(-40.0, fResult);
}

TEST(LSCUtils,testGetCelsiusFromFarenheit) {
    float fResult = LSC::getCelsiusFromFarenheit(32);
    ASSERT_EQ(0.0, fResult);
}

TEST(LSCUtils,testGetCelsiusFromFarenheitBoilingPoint) {
    float fResult = LSC::getCelsiusFromFarenheit(212);
    ASSERT_FLOAT_EQ(100.0, fResult);
}

TEST(LSCUtils,testGetCelsiusFromFarenheitNegativeForty) {
    float fResult = LSC::getCelsiusFromFarenheit(-40);
    ASSERT_FLOAT_EQ(-40.0, fResult);
}
