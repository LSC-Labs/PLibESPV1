#include <gtest/gtest.h>
// #include <gmock/gmock.h>
#include <Vars.h>

class CVarTest : public ::testing::Test {
public:
    CVar oVar;
    CVarTable oVarTable;
};

TEST_F(CVarTest, setSingleVarValue) {
    oVar.setValue("TestData");
    const char * pszValue = oVar.getValue();
    EXPECT_NE(pszValue,nullptr);
    EXPECT_EQ(strcmp("TestData",pszValue),0);
}
/*
TEST_F(CVarTable,setSingleVarValue) {
    CVarTable oVarTable;
    oVarTable.set("key1",true);
    oVarTable.set("key2","TestData");
    EXPECT_STREQ(oVarTable.getValue("key2"),"TestData");
}
    */

