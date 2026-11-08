#include <gtest/gtest.h>
#include <ConfigHandler.h>

namespace {
    struct CValueHandler : public IConfigHandler {
        int nValue = 0;
        int nReads = 0, nWrites = 0, nMigrates = 0;
        bool bHasValues = true;
        void writeConfigTo(JsonNode &oNode, bool) override { oNode["v"] = nValue; nWrites++; }
        void readConfigFrom(JsonNode &oNode) override { oNode.storeValueIf("v", &nValue); nReads++; }
        bool hasConfigValues() override { return(bHasValues); }
        void migrateConfig(JsonNode &, JsonNode &) override { nMigrates++; }
    };

    // Removes another handler from the list while the list is processed
    struct CRemovingHandler : public IConfigHandler {
        CConfigHandler *pList = nullptr;
        IConfigHandler *pVictim = nullptr;
        void writeConfigTo(JsonNode &, bool) override { if(pList) pList->removeConfigHandler(pVictim); }
        void readConfigFrom(JsonNode &) override {}
    };
}

TEST(ConfigHandler, writeAndReadSections) {
    CConfigHandler oList;
    CValueHandler oA, oB;
    oA.nValue = 1; oB.nValue = 2;
    EXPECT_TRUE(oList.addConfigHandler("a", &oA));
    EXPECT_TRUE(oList.addConfigHandler("b", &oB));
    JsonNode oCfg;
    oList.writeConfigTo(oCfg, false);
    EXPECT_STREQ(oCfg.getAsJsonText(), "{\"a\":{\"v\":1},\"b\":{\"v\":2}}");

    CValueHandler oA2, oB2;
    CConfigHandler oList2;
    oList2.addConfigHandler("a", &oA2);
    oList2.addConfigHandler("b", &oB2);
    oList2.readConfigFrom(oCfg);
    EXPECT_EQ(oA2.nValue, 1);
    EXPECT_EQ(oB2.nValue, 2);
}

TEST(ConfigHandler, missingSectionIsSkipped) {
    CConfigHandler oList;
    CValueHandler oA;
    oA.nValue = 7;
    oList.addConfigHandler("a", &oA);
    JsonNode oCfg;
    ASSERT_NE(oCfg.parse("{\"other\":{}}"), nullptr);
    oList.readConfigFrom(oCfg);
    EXPECT_EQ(oA.nReads, 0);
    EXPECT_EQ(oA.nValue, 7);
}

TEST(ConfigHandler, noValuesNoSection) {
    CConfigHandler oList;
    CValueHandler oA;
    oA.bHasValues = false;
    oList.addConfigHandler("a", &oA);
    JsonNode oCfg;
    oList.writeConfigTo(oCfg, false);
    EXPECT_STREQ(oCfg.getAsJsonText(), "{}");
}

TEST(ConfigHandler, duplicateNameIsRejected) {
    CConfigHandler oList;
    CValueHandler oA, oB;
    EXPECT_TRUE(oList.addConfigHandler("x", &oA));
    EXPECT_TRUE(oList.addConfigHandler("x", &oA));      // same handler again - ok
    EXPECT_FALSE(oList.addConfigHandler("x", &oB));     // other handler - rejected
    EXPECT_EQ(oList.getConfigHandler("x"), &oA);
    EXPECT_TRUE(oList.addConfigHandler("y", &oA));      // second name for the same handler
    EXPECT_EQ(oList.getConfigHandler("y"), &oA);
}

TEST(ConfigHandler, invalidArguments) {
    CConfigHandler oList;
    CValueHandler oA;
    EXPECT_FALSE(oList.addConfigHandler((const char *) nullptr, &oA));
    EXPECT_FALSE(oList.addConfigHandler("", &oA));
    EXPECT_FALSE(oList.addConfigHandler("a", nullptr));
    EXPECT_EQ(oList.getConfigHandler((const char *) nullptr), nullptr);
}

TEST(ConfigHandler, selfRegistrationIsRejected) {
    CConfigHandler oList;
    EXPECT_FALSE(oList.addConfigHandler("self", &oList));
    JsonNode oCfg;
    oList.writeConfigTo(oCfg, false);                  // must not recurse
    SUCCEED();
}

TEST(ConfigHandler, cyclicRegistrationDoesNotRecurse) {
    CConfigHandler oA, oB;
    CValueHandler oLeaf;
    oA.addConfigHandler("b", &oB);
    oB.addConfigHandler("a", &oA);                     // cycle a -> b -> a
    oB.addConfigHandler("leaf", &oLeaf);
    JsonNode oCfg;
    oA.writeConfigTo(oCfg, false);
    oA.readConfigFrom(oCfg);
    oA.migrateConfig(oCfg, oCfg);
    EXPECT_EQ(oLeaf.nWrites, 1);
    EXPECT_EQ(oLeaf.nMigrates, 1);
}

TEST(ConfigHandler, handlerRemovedDuringWriteIsSkipped) {
    CConfigHandler oList;
    CRemovingHandler oRemover;
    CValueHandler oVictim, oLast;
    oRemover.pList = &oList;
    oRemover.pVictim = &oVictim;
    oList.addConfigHandler("remover", &oRemover);
    oList.addConfigHandler("victim", &oVictim);
    oList.addConfigHandler("last", &oLast);
    JsonNode oCfg;
    oList.writeConfigTo(oCfg, false);
    EXPECT_EQ(oVictim.nWrites, 0);                     // removed before its turn
    EXPECT_EQ(oLast.nWrites, 1);                       // not skipped
}
