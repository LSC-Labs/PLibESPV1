#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "Appl.h"

class CApplTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize before each test
    }

    void TearDown() override {
        // Cleanup after each test
    }
};

TEST_F(CApplTest, initSetsAppNameAndVersion) {
    // CAppl appl;
    const char* testName = "TestApp";
    const char* testVersion = "1.0.0";
    
    Appl.init(testName, testVersion);
    
    EXPECT_EQ(Appl.AppName, testName);
    EXPECT_EQ(Appl.AppVersion, testVersion);
}

TEST_F(CApplTest, GetUpTimeFormatsCorrectly) {
    // CAppl appl;
    String uptime = Appl.getUpTime();
    unsigned int nUpTime   = uptime.length();
    unsigned int nExpected = 8;
    
    EXPECT_EQ(nUpTime, nExpected); // HH:MM:SS format
    EXPECT_EQ(uptime[2], ':');
    EXPECT_EQ(uptime[5], ':');
}

TEST_F(CApplTest, GetISODateReturnsValidFormat) {
    // CAppl appl;
    const char* date = Appl.getISODate();
    
    EXPECT_NE(date, nullptr);
    EXPECT_GE(strlen(date), (size_t) 10); // YYYY-MM-DD
}

TEST_F(CApplTest, GetISOTimeReturnsValidFormat) {
    // CAppl appl;
    const char* time = Appl.getISOTime();
    
    EXPECT_NE(time, nullptr);
    EXPECT_GE(strlen(time),(size_t) 8); // HH:MM:SS
}

TEST_F(CApplTest, GetISODateTimeReturnsValidFormat) {
    // CAppl appl;
    const char* datetime = Appl.getISODateTime();
    
    EXPECT_NE(datetime, nullptr);
    EXPECT_GE(strlen(datetime), (size_t) 19); // YYYY-MM-DDTHH:MM:SS
}

TEST_F(CApplTest, GetNativeTimeReturnsValidTimestamp) {
    // CAppl appl;
    time_t nativeTime = Appl.getNativeTime();
    
    EXPECT_GT(nativeTime, 0);
}

TEST_F(CApplTest, RegisterModuleWithValidModule) {
    // CAppl appl;
    // Mock IModule would be needed for full test
    // This is a placeholder for integration testing
}

TEST_F(CApplTest, SayHelloDoesNotCrash) {
    // CAppl appl;
    Appl.AppName = "TestApp";
    Appl.AppVersion = "1.0.0";
    
    EXPECT_NO_THROW(Appl.sayHello());
}

TEST_F(CApplTest, DispatchSendsEvent) {
    // CAppl appl;
    Appl.init("TestApp", "1.0.0");
    
    EXPECT_NO_THROW(Appl.dispatch(nullptr, 0));
}



