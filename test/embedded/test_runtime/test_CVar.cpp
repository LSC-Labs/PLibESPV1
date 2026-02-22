#include <gtest/gtest.h>
// #include <gmock/gmock.h>
#include <Vars.h>

class CVarTest : public ::testing::Test {
public:
    CVar oVar;
};

TEST_F(CVarTest, setValue) {
    oVar.setValue("TestData");
    const char * pszValue = oVar.getValue();
    EXPECT_NE(pszValue,nullptr);
    EXPECT_EQ(strcmp("TestData",pszValue),0);
}

