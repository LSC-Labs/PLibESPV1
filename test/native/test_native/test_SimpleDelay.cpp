
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
