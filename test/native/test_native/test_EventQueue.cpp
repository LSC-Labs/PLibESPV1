#include <gtest/gtest.h>
#include <EventHandler.h>
#include <JsonNode.h>
#include <Msgs.h>
#include <vector>
#include <string>

namespace {
    bool s_bLoopContext = true;
    bool testContextProbe() { return(s_bLoopContext); }

    struct CRecorder : public IMsgEventReceiver {
        std::vector<int>         tMsgs;
        std::vector<std::string> tTexts;
        std::vector<int>         tClasses;
        int receiveEvent(const void *, int nMsg, const void *pMessage, int nClass) override {
            tMsgs.push_back(nMsg);
            tClasses.push_back(nClass);
            if(nMsg == 1001 || nMsg == MSG_LOG_ENTRY) tTexts.push_back(pMessage ? (const char *) pMessage : "<null>");
            if(nMsg == 1002) {
                CJsonNode *pNode = (CJsonNode *) pMessage;
                tTexts.push_back(pNode ? pNode->getAsJsonText() : "<null>");
            }
            return(EVENT_MSG_RESULT_OK);
        }
    };

    // Simulates the async context for the duration of a test
    class EventQueueTest : public ::testing::Test {
        protected:
            void SetUp() override    { s_bLoopContext = true; CEventHandler::setContextProbe(testContextProbe); }
            void TearDown() override { CEventHandler::setContextProbe(nullptr); }
    };
}

TEST_F(EventQueueTest, asyncEventWithoutPayloadIsQueued) {
    CEventHandler oBus;
    CRecorder oRec;
    oBus.registerEventReceiver(&oRec, "rec");
    s_bLoopContext = false;
    EXPECT_EQ(oBus.sendEvent(nullptr, 10, nullptr, 1), EVENT_MSG_RESULT_OK);
    EXPECT_EQ(oBus.sendEvent(nullptr, 11, nullptr, 2), EVENT_MSG_RESULT_OK);
    EXPECT_TRUE(oRec.tMsgs.empty());                 // not delivered in the async context
    EXPECT_EQ(oBus.getDeferredEventCount(), 2u);
    oBus.processDeferredEvents();                    // still async - nothing happens
    EXPECT_TRUE(oRec.tMsgs.empty());
    s_bLoopContext = true;
    oBus.processDeferredEvents();
    ASSERT_EQ(oRec.tMsgs.size(), 2u);
    EXPECT_EQ(oRec.tMsgs[0], 10);
    EXPECT_EQ(oRec.tMsgs[1], 11);
    EXPECT_EQ(oRec.tClasses[1], 2);
}

TEST_F(EventQueueTest, asyncEventWithPayloadIsDeliveredDirectly) {
    CEventHandler oBus;
    CRecorder oRec;
    oBus.registerEventReceiver(&oRec, "rec");
    s_bLoopContext = false;
    oBus.sendEvent(nullptr, 1001, "direct", 0);
    ASSERT_EQ(oRec.tTexts.size(), 1u);
    EXPECT_EQ(oRec.tTexts[0], "direct");
    EXPECT_EQ(oBus.getDeferredEventCount(), 0u);
}

TEST_F(EventQueueTest, postEventTextCopiesThePayload) {
    CEventHandler oBus;
    CRecorder oRec;
    oBus.registerEventReceiver(&oRec, "rec");
    s_bLoopContext = false;
    char szText[16] = "hello";
    oBus.postEventText(nullptr, 1001, szText, 7);
    strcpy(szText, "changed");                       // the caller's buffer is gone/changed
    s_bLoopContext = true;
    oBus.processDeferredEvents();
    ASSERT_EQ(oRec.tTexts.size(), 1u);
    EXPECT_EQ(oRec.tTexts[0], "hello");
    EXPECT_EQ(oRec.tClasses[0], 7);
}

TEST_F(EventQueueTest, postEventJsonDeliversANode) {
    CEventHandler oBus;
    CRecorder oRec;
    oBus.registerEventReceiver(&oRec, "rec");
    s_bLoopContext = false;
    {
        CJsonNode oNode;
        oNode["a"] = 1;
        oBus.postEventJson(nullptr, 1002, oNode);
    }                                                // node destroyed before delivery
    s_bLoopContext = true;
    oBus.processDeferredEvents();
    ASSERT_EQ(oRec.tTexts.size(), 1u);
    EXPECT_EQ(oRec.tTexts[0], "{\"a\":1}");
}

TEST_F(EventQueueTest, loopEventDeliversQueuedEventsFirst) {
    CEventHandler oBus;
    CRecorder oRec;
    oBus.registerEventReceiver(&oRec, "rec");
    s_bLoopContext = false;
    oBus.postEvent(nullptr, 20);
    oBus.postEvent(nullptr, 21);
    s_bLoopContext = true;
    oBus.sendEvent(nullptr, 22, nullptr, 0);         // queued ones first - order kept
    ASSERT_EQ(oRec.tMsgs.size(), 3u);
    EXPECT_EQ(oRec.tMsgs[0], 20);
    EXPECT_EQ(oRec.tMsgs[1], 21);
    EXPECT_EQ(oRec.tMsgs[2], 22);
    EXPECT_EQ(oBus.getDeferredEventCount(), 0u);
}

TEST_F(EventQueueTest, queueOverflowIsReported) {
    CEventHandler oBus;
    CRecorder oRec;
    oBus.registerEventReceiver(&oRec, "rec");
    s_bLoopContext = false;
    for(int nIdx = 0; nIdx < EVENT_DEFERRED_MAX_ENTRIES + 5; nIdx++) oBus.postEvent(nullptr, 30);
    EXPECT_FALSE(oBus.postEvent(nullptr, 30));
    s_bLoopContext = true;
    oBus.processDeferredEvents();
    ASSERT_EQ(oRec.tMsgs.size(), (size_t) EVENT_DEFERRED_MAX_ENTRIES + 1);
    EXPECT_EQ(oRec.tMsgs.back(), MSG_LOG_ENTRY);     // warning about the dropped events
    EXPECT_EQ(oRec.tClasses.back(), 'W');
    EXPECT_NE(oRec.tTexts.back().find("6 queued events dropped"), std::string::npos);
}

TEST_F(EventQueueTest, receiverDestroyedBeforeQueuedEvent) {
    CEventHandler oBus;
    s_bLoopContext = false;
    {
        CRecorder oTemp;
        oBus.registerEventReceiver(&oTemp, "temp");
        oBus.postEventText(nullptr, 1001, "late");
    }
    s_bLoopContext = true;
    oBus.processDeferredEvents();                    // must not call the destroyed receiver
    SUCCEED();
}
