#include <gtest/gtest.h>
#include <StatusHandler.h>

namespace {
    struct CValueStatus : public IStatusHandler {
        int nValue = 0, nWrites = 0;
        int nMinLevel = 0;              // section only from this level on
        void writeStatusTo(JsonNode &oNode, int) override { oNode["v"] = nValue; nWrites++; }
        bool hasStatusValues(int nLevel) override { return(nLevel >= nMinLevel); }
    };
    struct CRemovingStatus : public IStatusHandler {
        CStatusHandler *pList = nullptr;
        IStatusHandler *pVictim = nullptr;
        void writeStatusTo(JsonNode &, int) override { if(pList) pList->removeStatusHandler(pVictim); }
    };
}

TEST(StatusHandler, writesSections) {
    CStatusHandler oList;
    CValueStatus oA, oB;
    oA.nValue = 1; oB.nValue = 2;
    EXPECT_TRUE(oList.addStatusHandler("a", &oA));
    EXPECT_TRUE(oList.addStatusHandler("b", &oB));
    JsonNode oStatus;
    oList.writeStatusTo(oStatus, STATUS_LEVEL_INFO);
    EXPECT_STREQ(oStatus.getAsJsonText(), "{\"a\":{\"v\":1},\"b\":{\"v\":2}}");
}

TEST(StatusHandler, hasStatusValuesIsRespected) {
    CStatusHandler oList;
    CValueStatus oVerboseOnly;
    oVerboseOnly.nMinLevel = STATUS_LEVEL_VERBOSE;
    oList.addStatusHandler("diag", &oVerboseOnly);
    JsonNode oInfo, oVerbose;
    oList.writeStatusTo(oInfo, STATUS_LEVEL_INFO);
    oList.writeStatusTo(oVerbose, STATUS_LEVEL_VERBOSE);
    EXPECT_STREQ(oInfo.getAsJsonText(), "{}");         // no empty section
    EXPECT_STREQ(oVerbose.getAsJsonText(), "{\"diag\":{\"v\":0}}");
    EXPECT_EQ(oVerboseOnly.nWrites, 1);
}

TEST(StatusHandler, duplicateNameAndInvalidArguments) {
    CStatusHandler oList;
    CValueStatus oA, oB;
    EXPECT_TRUE(oList.addStatusHandler("x", &oA));
    EXPECT_TRUE(oList.addStatusHandler("x", &oA));
    EXPECT_FALSE(oList.addStatusHandler("x", &oB));
    EXPECT_EQ(oList.getStatusHandler("x"), &oA);
    EXPECT_FALSE(oList.addStatusHandler((const char *) nullptr, &oA));
    EXPECT_FALSE(oList.addStatusHandler("", &oA));
    EXPECT_FALSE(oList.addStatusHandler("y", nullptr));
    EXPECT_FALSE(oList.addStatusHandler("self", &oList));
    EXPECT_EQ(oList.getStatusHandler((const char *) nullptr), nullptr);
}

TEST(StatusHandler, cyclicRegistrationDoesNotRecurse) {
    CStatusHandler oA, oB;
    CValueStatus oLeaf;
    oA.addStatusHandler("b", &oB);
    oB.addStatusHandler("a", &oA);
    oB.addStatusHandler("leaf", &oLeaf);
    JsonNode oStatus;
    oA.writeStatusTo(oStatus, STATUS_LEVEL_INFO);
    EXPECT_EQ(oLeaf.nWrites, 1);
}

TEST(StatusHandler, handlerRemovedDuringWriteIsSkipped) {
    CStatusHandler oList;
    CRemovingStatus oRemover;
    CValueStatus oVictim, oLast;
    oRemover.pList = &oList;
    oRemover.pVictim = &oVictim;
    oList.addStatusHandler("remover", &oRemover);
    oList.addStatusHandler("victim", &oVictim);
    oList.addStatusHandler("last", &oLast);
    JsonNode oStatus;
    oList.writeStatusTo(oStatus, STATUS_LEVEL_INFO);
    EXPECT_EQ(oVictim.nWrites, 0);
    EXPECT_EQ(oLast.nWrites, 1);
}
