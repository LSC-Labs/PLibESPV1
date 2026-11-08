#pragma once
/**
 * Backend independent tests for JsonHelper.h.
 * Used by the native and the embedded test runner. The same tests must pass
 * with JsonNode (default) and with -D LSC_JSON_USE_ARDUINOJSON.
 */
#include <gtest/gtest.h>
#include <JsonHelper.h>
#include <string.h>

TEST(JsonHelper, buildAndSerialize) {
    JSON_DOC(oDoc);
    JSON_OBJECT oRoot = JSON_ROOT(oDoc);
    JSON_SET(oRoot, "name", "value");
    JSON_SET(oRoot, "count", 5);
    JSON_SET(oRoot, "flag", true);
    String strTemp = "text";
    JSON_SET(oRoot, "str", strTemp);
    JSON_OBJECT oSub = JSON_CREATE_OBJECT(oRoot, "sub");
    JSON_SET(oSub, "x", 1);
    JSON_ARRAY oList = JSON_CREATE_ARRAY(oRoot, "list");
    JSON_ADD_VALUE(oList, 1);
    JSON_ADD_VALUE(oList, 2);
    JSON_OBJECT oItem = JSON_ADD_OBJECT(oList);
    JSON_SET(oItem, "y", 2);

    String strText;
    JSON_SERIALIZE(oDoc, strText);
    EXPECT_STREQ(strText.c_str(),
        "{\"name\":\"value\",\"count\":5,\"flag\":true,\"str\":\"text\",\"sub\":{\"x\":1},\"list\":[1,2,{\"y\":2}]}");
}

TEST(JsonHelper, parseAndRead) {
    JSON_DOC(oDoc);
    ASSERT_TRUE(JSON_PARSE(oDoc, "{\"s\":\"abc\",\"i\":42,\"f\":1.5,\"b\":true,\"u\":4000000000,\"o\":{\"k\":\"v\"}}"));
    JSON_OBJECT oRoot = JSON_ROOT(oDoc);
    EXPECT_STREQ(JSON_GET_STRING(oRoot, "s", ""), "abc");
    EXPECT_EQ(JSON_GET_INT(oRoot, "i", 0), 42);
    EXPECT_FLOAT_EQ(JSON_GET_FLOAT(oRoot, "f", 0.0f), 1.5f);
    EXPECT_TRUE(JSON_GET_BOOL(oRoot, "b", false));
    EXPECT_EQ(JSON_GET_ULONG(oRoot, "u", 0UL), 4000000000UL);
    EXPECT_TRUE(JSON_KEY_EXISTS(oRoot, "o"));
    EXPECT_FALSE(JSON_KEY_EXISTS(oRoot, "missing"));
    JSON_OBJECT oSub = JSON_GET_OBJECT(oRoot, "o");
    ASSERT_FALSE(JSON_IS_NULL(oSub));
    EXPECT_STREQ(JSON_GET_STRING(oSub, "k", ""), "v");
}

TEST(JsonHelper, defaultsForMissingKeys) {
    JSON_DOC(oDoc);
    ASSERT_TRUE(JSON_PARSE(oDoc, "{}"));
    JSON_OBJECT oRoot = JSON_ROOT(oDoc);
    EXPECT_STREQ(JSON_GET_STRING(oRoot, "x", "def"), "def");
    EXPECT_EQ(JSON_GET_INT(oRoot, "x", 7), 7);
    EXPECT_TRUE(JSON_GET_BOOL(oRoot, "x", true));
    EXPECT_TRUE(JSON_IS_NULL(JSON_GET_OBJECT(oRoot, "x")));
}

TEST(JsonHelper, parseErrorIsReported) {
    JSON_DOC(oDoc);
    EXPECT_FALSE(JSON_PARSE(oDoc, "{\"a\":"));
}

TEST(JsonHelper, getOrCreateObject) {
    JSON_DOC(oDoc);
    JSON_OBJECT oRoot = JSON_ROOT(oDoc);
    JSON_OBJECT oFirst = JSON_GET_OR_CREATE_OBJECT(oRoot, "cfg");
    JSON_SET(oFirst, "a", 1);
    JSON_OBJECT oSecond = JSON_GET_OR_CREATE_OBJECT(oRoot, "cfg");
    EXPECT_EQ(JSON_GET_INT(oSecond, "a", 0), 1);
    String strText;
    JSON_SERIALIZE(oDoc, strText);
    EXPECT_STREQ(strText.c_str(), "{\"cfg\":{\"a\":1}}");
}

TEST(JsonHelper, createPayloadStructureWithData) {
    JSON_DOC(oDoc);
    JSON_OBJECT oPayload = JSON_PAYLOAD(oDoc, "SET", "CONFIG", "{\"key\":\"value\"}");
    ASSERT_FALSE(JSON_IS_NULL(oPayload));
    EXPECT_STREQ(JSON_GET_STRING(oPayload, "key", ""), "value");
    EXPECT_STREQ(JSON_GET_STRING(JSON_ROOT(oDoc), "command", ""), "SET");
    EXPECT_STREQ(JSON_GET_STRING(JSON_ROOT(oDoc), "data", ""), "CONFIG");
}

TEST(JsonHelper, createPayloadStructureWithoutData) {
    JSON_DOC(oDoc);
    JSON_OBJECT oPayload = JSON_PAYLOAD(oDoc, "GET", "STATUS", nullptr);
    ASSERT_FALSE(JSON_IS_NULL(oPayload));
    JSON_SET(oPayload, "n", 1);
    String strText;
    JSON_SERIALIZE(oDoc, strText);
    EXPECT_STREQ(strText.c_str(), "{\"command\":\"GET\",\"data\":\"STATUS\",\"payload\":{\"n\":1}}");
}

TEST(JsonHelper, storeValues) {
    JSON_DOC(oDoc);
    ASSERT_TRUE(JSON_PARSE(oDoc, "{\"s\":\"hello\",\"i\":42,\"f\":3.25,\"b\":true,\"bs\":\"false\",\"u\":123456,\"pw\":\"*****\",\"ip\":\"192.168.1.2\"}"));
    JSON_OBJECT oRoot = JSON_ROOT(oDoc);

    String strValue;
    EXPECT_TRUE(JSON_STORE_IF(oRoot, "s", strValue));
    EXPECT_STREQ(strValue.c_str(), "hello");

    int nValue = 0;
    EXPECT_TRUE(JSON_STORE_IF(oRoot, "i", &nValue));
    EXPECT_EQ(nValue, 42);

    float fValue = 0.0f;
    EXPECT_TRUE(JSON_STORE_IF(oRoot, "f", &fValue));
    EXPECT_FLOAT_EQ(fValue, 3.25f);

    bool bValue = false;
    EXPECT_TRUE(JSON_STORE_IF(oRoot, "b", &bValue));
    EXPECT_TRUE(bValue);
    EXPECT_TRUE(JSON_STORE_IF(oRoot, "bs", &bValue));
    EXPECT_FALSE(bValue);

    unsigned long ulValue = 0;
    EXPECT_TRUE(JSON_STORE_IF(oRoot, "u", &ulValue));
    EXPECT_EQ(ulValue, 123456UL);

    String strPassword = "secret";
    EXPECT_FALSE(JSON_STORE_IF_NOT(oRoot, "pw", strPassword, "*****"));
    EXPECT_STREQ(strPassword.c_str(), "secret");

    IPAddress oIP;
    EXPECT_TRUE(JSON_STORE_IF(oRoot, "ip", oIP));
    EXPECT_STREQ(oIP.toString().c_str(), "192.168.1.2");

    String strMissing = "default";
    EXPECT_FALSE(JSON_STORE_IF(oRoot, "missing", strMissing));
    EXPECT_STREQ(strMissing.c_str(), "default");
}
