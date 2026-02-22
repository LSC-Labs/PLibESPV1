#include <gtest/gtest.h>
// #include <gmock/gmock.h>
#include "Vars.h"

class CVarTableTest : public ::testing::Test {
protected:
    CVarTable varTable;
    CVarTable varTableCaseSensitive{true};

    CVarTableTest() : varTable(false) {}
};

TEST_F(CVarTableTest, findNonExistentVarReturnsNull) {
    CVar* result = varTable.find("nonexistent");
    EXPECT_EQ(result, nullptr);
}



TEST_F(CVarTableTest, createAndFindVar) {
    CVar* var = varTable.set("testVar","hello");
    EXPECT_NE(var, nullptr);
    
    CVar* foundVar = varTable.find("testVar");
    EXPECT_EQ(var, foundVar);
}

TEST_F(CVarTableTest, testIfTableContainsConfigVars) {
    EXPECT_FALSE(varTable.hasConfigValues());
    varTable.set("testVar","1");
    EXPECT_TRUE(varTable.hasConfigValues());
}
TEST_F(CVarTableTest, createVarWithNullptrName) {
    CVar* var = varTable.set(nullptr,"hello");
    EXPECT_NE(var, nullptr);
}

TEST_F(CVarTableTest, setAndGetStringValue) {
    varTable.set("key1", "value1");
    const char* result = varTable.getValue("key1", "default");
    EXPECT_STREQ(result, "value1");
}

TEST_F(CVarTableTest, getDefaultValueWhenVarNotFound) {
    const char* result = varTable.getValue("nonexistent", "default");
    EXPECT_STREQ(result, "default");
}

TEST_F(CVarTableTest, setAndGetIntValue) {
    varTable.set("intKey", 42);
    int result = varTable.getIntValue("intKey", 0);
    EXPECT_EQ(result, 42);
}

TEST_F(CVarTableTest, getDefaultIntValueWhenVarNotFound) {
    int result = varTable.getIntValue("nonexistent", 99);
    EXPECT_EQ(result, 99);
}

TEST_F(CVarTableTest, setAndGetBoolValue) {
    varTable.set("boolKey", true);
    bool result = varTable.getBoolValue("boolKey", false);
    EXPECT_TRUE(result);
}

TEST_F(CVarTableTest, getDefaultBoolValueWhenVarNotFound) {
    bool result = varTable.getBoolValue("nonexistent", true);
    EXPECT_TRUE(result);
}

TEST_F(CVarTableTest, caseInsensitiveFind) {
    varTable.set("TestKey", "value");
    CVar* result = varTable.find("testkey");
    EXPECT_NE(result, nullptr);
}

TEST_F(CVarTableTest, caseSensitiveFind) {
    varTableCaseSensitive.set("TestKey", "value");
    CVar* result = varTableCaseSensitive.find("testkey");
    EXPECT_EQ(result, nullptr);
}

TEST_F(CVarTableTest, setUnsignedLongValue) {
    varTable.set("ulongKey", 1000000UL);
    int result = varTable.getIntValue("ulongKey", 0);
    EXPECT_EQ(result, 1000000);
}

TEST_F(CVarTableTest, hasCriticalVarsWhenNone) {
    varTable.set("normalKey", "value");
    bool result = varTable.hasCriticalVars();
    EXPECT_FALSE(result);
}

TEST_F(CVarTableTest, hasCriticalVarsWhenCriticals) {
    varTable.set("criticalKey", "value")->setCriticalVar(true);
    bool result = varTable.hasCriticalVars();
    EXPECT_TRUE(result);
}

TEST_F(CVarTableTest, multipleVarsInTable) {
    varTable.set("key1", "value1");
    varTable.set("key2", "value2");
    varTable.set("key3", "value3");
    
    EXPECT_STREQ(varTable.getValue("key1", ""), "value1");
    EXPECT_STREQ(varTable.getValue("key2", ""), "value2");
    EXPECT_STREQ(varTable.getValue("key3", ""), "value3");
}

TEST_F(CVarTableTest, overwriteExistingVar) {
    varTable.set("key", "value1");
    varTable.set("key", "value2");
    EXPECT_STREQ(varTable.getValue("key", ""), "value2");
}

TEST_F(CVarTableTest, setWithStringObject) {
    String name("stringKey");
    String value("stringValue");
    varTable.set(name, value);
    const char* result = varTable.getValue("stringkey", "default");
    EXPECT_STREQ(result, "stringValue");
}

TEST_F(CVarTableTest, findWithFlashStringHelper) {
    varTable.set("flashKey", "value");
    CVar* result = varTable.find(F("flashKey"));
    EXPECT_NE(result, nullptr);
}

TEST_F(CVarTableTest, getValueWithStringReference) {
    varTable.set("strKey", "strValue");
    String strName("strKey");
    const char* result = varTable.getValue(strName, "default");
    EXPECT_STREQ(result, "strValue");
}