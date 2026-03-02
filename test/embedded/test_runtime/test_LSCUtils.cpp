#include <gtest/gtest.h>
#include <LSCUtils.h>
#include <cstring>


TEST(LSCUtils,testStricmpWithEqualStrings) {
    ASSERT_EQ(0, LSC::stricmp("Hello", "hello"));
    ASSERT_EQ(0, LSC::stricmp("Hello", "HELLO"));
    ASSERT_EQ(0, LSC::stricmp("hello", "hello"));
}

TEST(LSCUtils,testStricmpWutcgDifferentStrings) {
    ASSERT_NE(0, LSC::stricmp("Hello", "World"));
}

TEST(LSCUtils,testParseBytesToArrayWithValidInput) {
    uint8_t bytes[6];
    int nResult = LSC::parseBytesToArray(bytes, "AA:BB:CC:DD:EE:FF", ':', 6, 16);
    ASSERT_EQ(6, nResult);
    ASSERT_EQ(0xAA, bytes[0]);
    ASSERT_EQ(0xBB, bytes[1]);
    ASSERT_EQ(0xFF, bytes[5]);
}

TEST(LSCUtils,testIsFalseValueWithNullPointer) {
    ASSERT_TRUE(LSC::isFalseValue(nullptr));
}

TEST(LSCUtils,testIsFalseValueWithFalseStrings) {
    ASSERT_TRUE(LSC::isFalseValue("0"));
    ASSERT_TRUE(LSC::isFalseValue("false"));
    ASSERT_TRUE(LSC::isFalseValue("off"));
    ASSERT_TRUE(LSC::isFalseValue("-"));
}

TEST(LSCUtils,testIsTrueValueExplicitTrue) {
    ASSERT_TRUE(LSC::isTrueValue("1", true));
    ASSERT_TRUE(LSC::isTrueValue("true", true));
    ASSERT_TRUE(LSC::isTrueValue("+", true));
    ASSERT_TRUE(LSC::isTrueValue("on", true));
}

TEST(LSCUtils,testIsTrueValueImplicit) {
    ASSERT_TRUE(LSC::isTrueValue("anything", false));
    ASSERT_FALSE(LSC::isTrueValue("0", false));
}

TEST(LSCUtils,testGetFarenheitFromCelsius) {
    float fResult = LSC::getFarenheitFromCelsius(0);
    ASSERT_EQ(32.0, fResult);
}

TEST(LSCUtils,testGetCelsiusFromFarenheit) {
    float fResult = LSC::getCelsiusFromFarenheit(32);
    ASSERT_EQ(0.0, fResult);
}
