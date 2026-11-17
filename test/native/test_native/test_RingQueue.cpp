
#include <gtest/gtest.h>
#include "RingQueue.h"

TEST(CRingQueue,newQueueIsEmpty) {
    CRingQueue<int,4> oQueue;
    EXPECT_TRUE(oQueue.empty());
    EXPECT_FALSE(oQueue.full());
    EXPECT_EQ(oQueue.size(),0u);
    EXPECT_EQ(oQueue.capacity(),4u);
}

TEST(CRingQueue,keepsFifoOrder) {
    CRingQueue<int,4> oQueue;
    EXPECT_TRUE(oQueue.push(1));
    EXPECT_TRUE(oQueue.push(2));
    EXPECT_TRUE(oQueue.push_back(3));
    EXPECT_EQ(oQueue.size(),3u);
    EXPECT_EQ(oQueue.front(),1); oQueue.pop();
    EXPECT_EQ(oQueue.front(),2); oQueue.pop_front();
    EXPECT_EQ(oQueue.front(),3); oQueue.pop();
    EXPECT_TRUE(oQueue.empty());
}

TEST(CRingQueue,rejectsPushWhenFull) {
    CRingQueue<int,2> oQueue;
    EXPECT_TRUE(oQueue.push(1));
    EXPECT_TRUE(oQueue.push(2));
    EXPECT_TRUE(oQueue.full());
    EXPECT_FALSE(oQueue.push(3));
    EXPECT_EQ(oQueue.size(),2u);
    EXPECT_EQ(oQueue.front(),1);
}

TEST(CRingQueue,wrapsAroundTheEnd) {
    CRingQueue<int,3> oQueue;
    // Move the head several times around the buffer end
    for(int nValue = 0; nValue < 10; nValue++) {
        EXPECT_TRUE(oQueue.push(nValue));
        EXPECT_TRUE(oQueue.push(nValue + 100));
        EXPECT_EQ(oQueue.front(),nValue);       oQueue.pop();
        EXPECT_EQ(oQueue.front(),nValue + 100); oQueue.pop();
        EXPECT_TRUE(oQueue.empty());
    }
}

TEST(CRingQueue,indexStartsAtOldestElement) {
    CRingQueue<int,3> oQueue;
    oQueue.push(1); oQueue.push(2); oQueue.pop();   // head moved
    oQueue.push(3); oQueue.push(4);                 // wraps
    ASSERT_EQ(oQueue.size(),3u);
    EXPECT_EQ(oQueue[0],2);
    EXPECT_EQ(oQueue[1],3);
    EXPECT_EQ(oQueue[2],4);
}

TEST(CRingQueue,popOnEmptyQueueIsNoOp) {
    CRingQueue<int,2> oQueue;
    oQueue.pop();
    EXPECT_TRUE(oQueue.empty());
    EXPECT_TRUE(oQueue.push(7));
    EXPECT_EQ(oQueue.front(),7);
}

TEST(CRingQueue,clearRemovesAllElements) {
    CRingQueue<int*,4> oQueue;
    int nA = 1, nB = 2;
    oQueue.push(&nA); oQueue.push(&nB);
    oQueue.clear();
    EXPECT_TRUE(oQueue.empty());
    EXPECT_TRUE(oQueue.push(&nB));
    EXPECT_EQ(oQueue.front(),&nB);
}
