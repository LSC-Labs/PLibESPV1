#include <gtest/gtest.h>
#include <EventHandler.h>
#include <ConfigHandler.h>
#include <StatusHandler.h>

// ---------------------------------------------------------------------------
// Test helpers
// ---------------------------------------------------------------------------
namespace {
    struct CCountingReceiver : public IMsgEventReceiver {
        int nCalls = 0;
        int receiveEvent(const void *, int, const void *, int) override { nCalls++; return EVENT_MSG_RESULT_OK; }
    };

    // Destroys another receiver while an event is dispatched
    struct CKillerReceiver : public IMsgEventReceiver {
        IMsgEventReceiver *pVictim = nullptr;
        int receiveEvent(const void *, int, const void *, int) override {
            delete pVictim; pVictim = nullptr; return EVENT_MSG_RESULT_OK;
        }
    };

    // Unregisters itself while an event is dispatched
    struct CSelfRemover : public IMsgEventReceiver {
        CEventHandler *pBus = nullptr;
        int nCalls = 0;
        int receiveEvent(const void *, int, const void *, int) override {
            nCalls++; pBus->unregisterEventReceiver(this); return EVENT_MSG_RESULT_OK;
        }
    };

    struct CModule : public IConfigHandler, public IStatusHandler, public IMsgEventReceiver {
        int nWrites = 0;
        void writeConfigTo(JsonNode &oNode, bool) override { oNode["v"] = 1; nWrites++; }
        void readConfigFrom(JsonNode &) override {}
        void writeStatusTo(JsonNode &oNode, int) override { oNode["s"] = 1; nWrites++; }
        int receiveEvent(const void *, int, const void *, int) override { return EVENT_MSG_RESULT_OK; }
    };
}

// ---------------------------------------------------------------------------
// Message bus
// ---------------------------------------------------------------------------
TEST(Registration, receiverUnregistersOnDestruction) {
    CEventHandler oBus;
    CCountingReceiver oStays;
    {
        CCountingReceiver oTemp;
        oBus.registerEventReceiver(&oTemp, "temp");
        oBus.registerEventReceiver(&oStays, "stays");
        EXPECT_TRUE(oBus.isRegistered(&oTemp));
        oBus.sendEvent(nullptr, 1, nullptr, 0);
        EXPECT_EQ(oTemp.nCalls, 1);
    }
    // oTemp is gone - sending must not touch it
    oBus.sendEvent(nullptr, 1, nullptr, 0);
    EXPECT_EQ(oStays.nCalls, 2);
}

TEST(Registration, explicitUnregister) {
    CEventHandler oBus;
    CCountingReceiver oReceiver;
    oBus.registerEventReceiver(&oReceiver, "r");
    oBus.unregisterEventReceiver(&oReceiver);
    EXPECT_FALSE(oBus.isRegistered(&oReceiver));
    oBus.sendEvent(nullptr, 1, nullptr, 0);
    EXPECT_EQ(oReceiver.nCalls, 0);
}

TEST(Registration, receiverDestroyedDuringDispatch) {
    CEventHandler oBus;
    CKillerReceiver oKiller;
    CCountingReceiver *pVictim = new CCountingReceiver();
    CCountingReceiver oLast;
    oKiller.pVictim = pVictim;
    oBus.registerEventReceiver(&oKiller, "killer");
    oBus.registerEventReceiver(pVictim, "victim");   // registered after the killer
    oBus.registerEventReceiver(&oLast, "last");
    oBus.sendEvent(nullptr, 1, nullptr, 0);          // victim deleted before its turn
    EXPECT_EQ(oLast.nCalls, 1);
    oBus.sendEvent(nullptr, 1, nullptr, 0);          // purged list still works
    EXPECT_EQ(oLast.nCalls, 2);
}

TEST(Registration, receiverRemovesItselfDuringDispatch) {
    CEventHandler oBus;
    CSelfRemover oSelf;
    CCountingReceiver oOther;
    oSelf.pBus = &oBus;
    oBus.registerEventReceiver(&oSelf, "self");
    oBus.registerEventReceiver(&oOther, "other");
    oBus.sendEvent(nullptr, 1, nullptr, 0);
    oBus.sendEvent(nullptr, 1, nullptr, 0);
    EXPECT_EQ(oSelf.nCalls, 1);
    EXPECT_EQ(oOther.nCalls, 2);
}

TEST(Registration, busDestroyedBeforeReceiver) {
    CCountingReceiver *pReceiver = new CCountingReceiver();
    {
        CEventHandler oBus;
        oBus.registerEventReceiver(pReceiver, "r");
    }
    delete pReceiver;   // must not access the destroyed bus
    SUCCEED();
}

TEST(Registration, receiverOnTwoBuses) {
    CEventHandler oBus1, oBus2;
    CCountingReceiver oStays;
    {
        CCountingReceiver oTemp;
        oBus1.registerEventReceiver(&oTemp, "t");
        oBus2.registerEventReceiver(&oTemp, "t");
        oBus1.registerEventReceiver(&oStays, "s");
    }
    EXPECT_EQ(oBus1.sendEvent(nullptr, 1, nullptr, 0), EVENT_MSG_RESULT_OK);
    EXPECT_EQ(oBus2.sendEvent(nullptr, 1, nullptr, 0), EVENT_MSG_RESULT_OK);
    EXPECT_EQ(oStays.nCalls, 1);
}

// ---------------------------------------------------------------------------
// Config / status handler lists
// ---------------------------------------------------------------------------
TEST(Registration, moduleUnregistersFromAllLists) {
    CEventHandler   oBus;
    CConfigHandler  oConfig;
    CStatusHandler  oStatus;
    {
        CModule oModule;
        oConfig.addConfigHandler("mod", &oModule);
        oStatus.addStatusHandler("mod", &oModule);
        oBus.registerEventReceiver(&oModule, "mod");
        EXPECT_EQ(oConfig.getConfigHandler("mod"), &oModule);
        EXPECT_EQ(oStatus.getStatusHandler("mod"), &oModule);
    }
    EXPECT_EQ(oConfig.getConfigHandler("mod"), nullptr);
    EXPECT_EQ(oStatus.getStatusHandler("mod"), nullptr);
    EXPECT_FALSE(oBus.isRegistered(nullptr));
    JsonNode oCfg, oState;
    oConfig.writeConfigTo(oCfg, false);       // must not call the destroyed module
    oStatus.writeStatusTo(oState, STATUS_LEVEL_INFO);
    EXPECT_STREQ(oCfg.getAsJsonText(), "{}");
    EXPECT_STREQ(oState.getAsJsonText(), "{}");
}

TEST(Registration, explicitRemoveFromLists) {
    CConfigHandler oConfig;
    CStatusHandler oStatus;
    CModule oModule;
    oConfig.addConfigHandler("a", &oModule);
    oConfig.addConfigHandler("b", &oModule);
    oStatus.addStatusHandler("a", &oModule);
    oConfig.removeConfigHandler(&oModule);
    oStatus.removeStatusHandler(&oModule);
    EXPECT_EQ(oConfig.getConfigHandler("a"), nullptr);
    EXPECT_EQ(oConfig.getConfigHandler("b"), nullptr);
    EXPECT_EQ(oStatus.getStatusHandler("a"), nullptr);
}

TEST(Registration, listsDestroyedBeforeModule) {
    CModule *pModule = new CModule();
    {
        CConfigHandler oConfig;
        CStatusHandler oStatus;
        oConfig.addConfigHandler("m", pModule);
        oStatus.addStatusHandler("m", pModule);
    }
    delete pModule;     // must not access the destroyed lists
    SUCCEED();
}

TEST(Registration, nestedConfigHandler) {
    CConfigHandler oRoot;
    {
        CConfigHandler oSub;
        oRoot.addConfigHandler("sub", &oSub);
        EXPECT_EQ(oRoot.getConfigHandler("sub"), &oSub);
    }
    EXPECT_EQ(oRoot.getConfigHandler("sub"), nullptr);
}
