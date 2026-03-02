#include <gtest/gtest.h>
#include <JsonHelper.h>
#include <ArduinoJson.h>


TEST(JsonHelper,createPayloadStructure_with_data) {
    JsonDocument doc;
    const char* pszCommand = "SET";
    const char* pszDataType = "CONFIG";
    const char* pszData = R"({"key":"value"})";
    
    JsonObject result = LSC::createPayloadStructure(pszCommand, pszDataType, doc, pszData);
    
    EXPECT_TRUE(result["key"] == "value");
}

TEST(JsonHelper,_createPayloadStructure_without_data) {
    JsonDocument doc;
    const char* pszCommand = "GET";
    const char* pszDataType = "STATUS";
    
    JsonObject result = LSC::createPayloadStructure(pszCommand, pszDataType, doc, nullptr);
    
    EXPECT_TRUE(result.isNull() || result.size() == 0);
}

TEST(JsonHelper,setJsonValue_string) {
    JsonDocument doc;
    doc["test"] = "hello";
    JsonObject obj = doc.as<JsonObject>();
    String result;
    
    bool bResult = LSC::setJsonValue(obj, "test", result);
    
    EXPECT_TRUE(bResult);
    EXPECT_STREQ("hello", result.c_str());
}

TEST(JsonHelper,setJsonValue_int) {
    JsonDocument oDoc;
    oDoc["test"] = 42;
    JsonObject obj = oDoc.as<JsonObject>();
    int nResult = 0;
    
    Serial.println(" --- try to set int..");

    bool bResult = LSC::setJsonValue(obj, "test", &nResult);
    
    EXPECT_TRUE(bResult);
    EXPECT_EQ(42, nResult);
}

TEST(JsonHelper,setJsonValue_float) {
    JsonDocument doc;
    doc["test"] = 3.14f;
    JsonObject obj = doc.as<JsonObject>();
    float result = 0.0f;
    
    bool bResult = LSC::setJsonValue(obj, "test", &result);
    
    EXPECT_TRUE(bResult);
    EXPECT_EQ(3.14f, result);
}

TEST(JsonHelper,setJsonValue_bool_true) {
    JsonDocument doc;
    doc["test"] = true;
    JsonObject obj = doc.as<JsonObject>();
    bool result = false;
    
    bool bResult = LSC::setJsonValue(obj, "test", &result);
    
    EXPECT_TRUE(bResult);
    EXPECT_TRUE(result);
}

TEST(JsonHelper,setJsonValue_bool_false_string) {
    JsonDocument doc;
    doc["test"] = "false";
    JsonObject obj = doc.as<JsonObject>();
    bool result = true;
    
    bool bResult = LSC::setJsonValue(obj, "test", &result);
    
    EXPECT_TRUE(bResult);
    EXPECT_FALSE(result);
}

TEST(JsonHelper,setJsonValue_key_not_found) {
    JsonDocument doc;
    JsonObject obj = doc.as<JsonObject>();
    String result = "default";
    
    bool bResult = LSC::setJsonValue(obj, "nonexistent", result);
    
    EXPECT_FALSE(bResult);
}
