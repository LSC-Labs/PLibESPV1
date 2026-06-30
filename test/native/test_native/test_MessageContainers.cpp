
#include <gtest/gtest.h>
#include "MQTTController.h"
#include "WebSocket.h"

TEST(CWebSocketMessage,testConstructorAllocatesZeroTerminatedBuffer) {
    CWebSocketMessage oMessage(nullptr,nullptr,5,WS_TEXT);
    EXPECT_EQ(oMessage.MessageSize,5);
    EXPECT_EQ(oMessage.MessageType,WS_TEXT);
    ASSERT_NE(oMessage.pSerializedMessage,nullptr);
    EXPECT_STREQ(oMessage.pSerializedMessage,"");
}

TEST(CWebSocketMessage,testSetMessageDataWritesSegment) {
    CWebSocketMessage oMessage(nullptr,nullptr,11,WS_TEXT);
    uint8_t szHello[] = "Hello";
    uint8_t szWorld[] = " world";

    oMessage.setMessageData(szHello,0,5);
    oMessage.setMessageData(szWorld,5,6);

    EXPECT_STREQ(oMessage.pSerializedMessage,"Hello world");
}

TEST(CWebSocketMessage,testSetMessageDataIgnoresOverflowSegment) {
    CWebSocketMessage oMessage(nullptr,nullptr,5,WS_TEXT);
    uint8_t szHello[] = "Hello";
    uint8_t szOverflow[] = "!";

    oMessage.setMessageData(szHello,0,5);
    oMessage.setMessageData(szOverflow,5,1);

    EXPECT_STREQ(oMessage.pSerializedMessage,"Hello");
}

TEST(MQTTMessage,testConstructorCopiesTopicAndMessage) {
    char szTopic[] = "device/state";
    char szMessage[] = "online";
    MQTTMessage oMessage(szTopic,szMessage);

    szTopic[0] = 'x';
    szMessage[0] = 'x';

    EXPECT_STREQ(oMessage.Topic,"device/state");
    EXPECT_STREQ(oMessage.Message,"online");
}

TEST(MQTTMessage,testTopicChecksReturnFalseWithoutController) {
    MQTTMessage oMessage("device/cmd/restart","1");
    EXPECT_FALSE(oMessage.isDeviceTopic());
    EXPECT_FALSE(oMessage.isDeviceCommandTopic());
}
