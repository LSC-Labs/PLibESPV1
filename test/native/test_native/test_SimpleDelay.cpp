
#include <gtest/gtest.h>
#include "SimpleDelay.h"

TEST(CSimpleDelay,testDefaultDelayIsInactiveAndNotDone) {
    CSimpleDelay oDelay;
    EXPECT_FALSE(oDelay.isActive());
    EXPECT_FALSE(oDelay.isDone());
    EXPECT_EQ(oDelay.getRemaining(),0);
}

TEST(CSimpleDelay,testStartActivatesDelay) {
    CSimpleDelay oDelay;
    oDelay.start(1000);
    EXPECT_TRUE(oDelay.isActive());
    EXPECT_FALSE(oDelay.isExpired());
    EXPECT_FALSE(oDelay.isDone());
}

TEST(CSimpleDelay,testSetExpiredMarksDelayDone) {
    CSimpleDelay oDelay(1000);
    oDelay.setExpired();
    EXPECT_TRUE(oDelay.isExpired());
    EXPECT_TRUE(oDelay.isDone());
}

TEST(CSimpleDelay,testStopDeactivatesDelay) {
    CSimpleDelay oDelay(1000);
    oDelay.stop();
    EXPECT_FALSE(oDelay.isActive());
    EXPECT_FALSE(oDelay.isExpired());
    EXPECT_FALSE(oDelay.isDone());
}

TEST(CSimpleDelay,testResetClearsRemainingTime) {
    CSimpleDelay oDelay(1000);
    oDelay.reset();
    EXPECT_FALSE(oDelay.isActive());
    EXPECT_FALSE(oDelay.isExpired());
    EXPECT_FALSE(oDelay.isDone());
    EXPECT_EQ(oDelay.getRemaining(),0);
}

TEST(CSimpleDelay,testRestartReturnsFutureEndTime) {
    CSimpleDelay oDelay(1000);
    unsigned long ulEndTime = oDelay.restart();
    EXPECT_TRUE(oDelay.isActive());
    EXPECT_GT(ulEndTime,millis());
}

TEST(CSimpleDelay,testStartCanDelayFirstRun) {
    CSimpleDelay oDelay;
    oDelay.start(1000,false);
    EXPECT_FALSE(oDelay.isActive());
    EXPECT_FALSE(oDelay.isDone());
}

// ---------------------------------------------------------------------------
// Controlled time source: overflow, millis()==0, next()
// ---------------------------------------------------------------------------
namespace {
    unsigned long s_ulFakeMillis = 0;
    unsigned long fakeMillis() { return(s_ulFakeMillis); }

    class CSimpleDelayTimeTest : public ::testing::Test {
        protected:
            void SetUp() override    { CSimpleDelay::setTimeSource(fakeMillis); }
            void TearDown() override { CSimpleDelay::setTimeSource(nullptr); }
    };
}

TEST_F(CSimpleDelayTimeTest, startAtMillisZeroIsActive) {
    s_ulFakeMillis = 0;
    CSimpleDelay oDelay(100);
    EXPECT_TRUE(oDelay.isActive());
    EXPECT_FALSE(oDelay.isDone());
    s_ulFakeMillis = 100;
    EXPECT_TRUE(oDelay.isDone());
}

TEST_F(CSimpleDelayTimeTest, worksAcrossMillisOverflow) {
    s_ulFakeMillis = (unsigned long) -50;              // 50 ms before overflow
    CSimpleDelay oDelay(100);
    EXPECT_FALSE(oDelay.isDone());
    EXPECT_EQ(oDelay.getRemaining(), 100u);
    s_ulFakeMillis = 30;                               // after overflow, 80 ms elapsed
    EXPECT_FALSE(oDelay.isDone());
    EXPECT_EQ(oDelay.getElapsed(), 80u);
    EXPECT_EQ(oDelay.getRemaining(), 20u);
    s_ulFakeMillis = 50;                               // 100 ms elapsed
    EXPECT_TRUE(oDelay.isDone());
    EXPECT_EQ(oDelay.getRemaining(), 0u);
}

TEST_F(CSimpleDelayTimeTest, nextKeepsTheIntervalAndSkipsMissedPeriods) {
    s_ulFakeMillis = 1000;
    CSimpleDelay oDelay(100);
    s_ulFakeMillis = 1350;                             // 3.5 periods late
    EXPECT_TRUE(oDelay.isDone());
    EXPECT_EQ(oDelay.next(), 1400u);                   // next end on the period grid
    EXPECT_FALSE(oDelay.isDone());
    EXPECT_EQ(oDelay.getRemaining(), 50u);
}

TEST_F(CSimpleDelayTimeTest, nextWithZeroDelayDoesNotHang) {
    s_ulFakeMillis = 1000;
    CSimpleDelay oDelay;
    oDelay.start(0);                                   // delay time 0
    s_ulFakeMillis = 5000;
    oDelay.next();                                     // previously an endless loop
    EXPECT_TRUE(oDelay.isDone());
}

TEST_F(CSimpleDelayTimeTest, notStartedUntilRestart) {
    s_ulFakeMillis = 1000;
    CSimpleDelay oDelay;
    oDelay.start(100, false);
    s_ulFakeMillis = 5000;
    EXPECT_FALSE(oDelay.isDone());
    EXPECT_EQ(oDelay.getElapsed(), 0u);
    oDelay.restart();
    EXPECT_FALSE(oDelay.isDone());
    s_ulFakeMillis = 5100;
    EXPECT_TRUE(oDelay.isDone());
}

TEST_F(CSimpleDelayTimeTest, setExpiredForImmediateFirstRun) {
    s_ulFakeMillis = 1000;
    CSimpleDelay oDelay;
    oDelay.start(5000, false);
    oDelay.setExpired();                               // pattern used by CMQTTController
    EXPECT_TRUE(oDelay.isDone());
    oDelay.restart();
    EXPECT_FALSE(oDelay.isDone());
}

TEST_F(CSimpleDelayTimeTest, restartWithSetsTheExactDelay) {
    s_ulFakeMillis = 1000;
    CSimpleDelay oDelay(500);
    EXPECT_EQ(oDelay.restartWith(0), 1000u);           // 0 is kept (start(0) would keep 500)
    EXPECT_EQ(oDelay.getDelay(), 0u);
    EXPECT_TRUE(oDelay.isDone());
    EXPECT_EQ(oDelay.restartWith(200), 1200u);
    EXPECT_FALSE(oDelay.isDone());
    s_ulFakeMillis = 1200;
    EXPECT_TRUE(oDelay.isDone());
    oDelay.setDelay(300);                              // running state is kept
    EXPECT_TRUE(oDelay.isActive());
    EXPECT_FALSE(oDelay.isDone());
}
