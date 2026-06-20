
#include <gtest/gtest.h>
#include "JsonNode.h"


const char T1[] = "{\"IP\":\"192.154.22.1\",\"S\":{ \"Sub\":1,\"S2\":2},\"TS\":\"4456\", \"B1\":true, \"B2\":\"false\", \"T2\":\"T2Value\", \"Token\":\"das ist ein Token mit \\\"tütelchen\"}";
const char T2[] = "{\"T\":1,\"B\":true,\"F\":-9.5,\"S\":{ \"Sub\":1,\"S2\":2},\"A\":[ \"Hello world\", { \"A1\":3, \"A2\":4 } ],\"X\":\"Tütelchen\"}";
const char T3[] = "{\"Token\":\"mit \\\"tütelchen\"}";

#pragma region basic tests

// Test: Default constructor
TEST(CJsonNode,testDefaultConstructor) {
    CJsonNode node;
    EXPECT_EQ(node.getObject("Test"),nullptr);
}


// Test: Constructor with name
// - first run without creation
// - second run with creation
TEST(CJsonNode,testConstructorWithParms) {
    CJsonNode oNode("TestNode","TestValue");
    EXPECT_EQ(oNode.getObject("TestNode"),nullptr);
    EXPECT_STREQ(oNode.Name.c_str(),"TestNode");
    EXPECT_STREQ(oNode.getValue(),"TestValue");
    EXPECT_EQ(oNode.getType(),CJsonNode::ELEMENT_TYPE::VALUE);
}

// Test: Constructor with name
// - first run without creation
// - second run with creation
TEST(CJsonNode,testGetObject) {
    CJsonNode oNode;
    oNode.getObject("TestNode");
    EXPECT_EQ(oNode.getObject("TestNode"),nullptr);
    oNode.getObject("TestNode",true);
    EXPECT_NE(oNode.getObject("TestNode"),nullptr);
}

// Test: getValue with default
TEST(CJsonNode,testGetValueDefault) {
    CJsonNode node;
    const char* result = node.getValueOrDefault("nonexistent", "default");
    EXPECT_STREQ(result, "default");
}

// Test: createOrGetObject creates new object
TEST(CJsonNode,testCreateObjectNew) {
    CJsonNode node;
    CJsonNode* obj1 = node.getObject("child",true);
    EXPECT_NE(obj1,nullptr);
    CJsonNode* obj2 = node.getObject("child");
    // Expecting to get the same node...
    EXPECT_EQ(obj1,obj2);
}

// Test: getObject returns nullptr for nonexistent
TEST(CJsonNode,testGetObjectNonexistent) {
    CJsonNode node;
    CJsonNode* result = node.getObject("nonexistent");
    EXPECT_EQ(result,nullptr);
}

// Test: exists returns false for nonexistent key
TEST(CJsonNode,testExistsNonexistent) {
    CJsonNode node;
    EXPECT_FALSE(node.exists("key"));
}

#pragma endregion

#pragma region parsing text and type tests

// Test: Simple JSON parse
TEST(CJsonNode,testParseSimpleJson) {
    CJsonNode node;
    node.parse("{\"name\":\"value\"}");
    EXPECT_TRUE(node.exists("name"));
}

// Test: Test Object
TEST(CJsonNode,testIsTypeOfObject) {
    CJsonNode node;
    node.parse("{\"data\":{} }");
    EXPECT_TRUE(node.exists("data"));
    EXPECT_TRUE(node.isJsonObject("data"));
}

TEST(CJsonNode,testIsTypeOfArray) {
    CJsonNode node;
    node.parse("{\"data\":[ 3,4,5] }");
    EXPECT_TRUE(node.exists("data"));
    EXPECT_TRUE(node.isJsonArray("data"));
}

TEST(CJsonNode,testIsTypeOfValue) {
    CJsonNode node;
    node.parse("{\"data\": 5 }");
    EXPECT_TRUE(node.exists("data"));
    EXPECT_TRUE(node.isJsonValue("data"));
}


// Test: Destructor cleanup
TEST(CJsonNode,textClear) {
   
    CJsonNode* node = new CJsonNode();
    CJsonNode * pSubNode = node->getObject("child",true);
    EXPECT_TRUE(node->exists("child"));
    node->clear();
    EXPECT_FALSE(node->exists("child"));
}

#pragma endregion

#pragma region get and check

// Test: getValue returns correct value
TEST(CJsonNode,testGetValueFromObject) {
    CJsonNode node;
    node.parse("{\"key\":\"testvalue\"}");
    const char* result = node.getValue("key");
    EXPECT_STREQ(result, "testvalue");
}

// Test: Parse get integer
TEST(CJsonNode,testIntIsNumberValue) {
    CJsonNode node;
    node.parse("{\"d\": \"2\" }");
    CJsonNode *pCheckNode = node.find("d");
    EXPECT_TRUE(pCheckNode->isNumberValue());
}

// Test: Parse get integer
TEST(CJsonNode,testFloatIsNumberValue) {
    CJsonNode node;
    node.parse("{\"d\": \"-9.5\" }");
    CJsonNode *pCheckNode = node.find("d");
    EXPECT_TRUE(pCheckNode->isNumberValue());
}

// Test: Parse get integer
TEST(CJsonNode,testGetIntegerExistingKey) {
    CJsonNode node;
    node.parse(T2);
    CJsonNode * pSubNode = node.find("T");
    EXPECT_TRUE(pSubNode->isNumberValue());
    EXPECT_EQ(node.getValueAsInt("T",-8),1);
}

// Test: Parse get unsigned integer
TEST(CJsonNode,testGetIntegerNotExists) {
    CJsonNode node;
    node.parse(T2);
    EXPECT_EQ(node.getValueAsInt("NOT",-3),-3);
}

// Test: Parse get unsigned integer
TEST(CJsonNode,testGetUnsignedIntegerExists) {
    CJsonNode node;
    node.parse(T2);
    CJsonNode * pSubNode = node.find("T");
    EXPECT_TRUE(pSubNode->isNumberValue());
    EXPECT_EQ(node.getValueAsUnsignedInt("T",-8),1);
}

// Test: Parse get integer
TEST(CJsonNode,testGetUnsignedIntNotExists) {
    CJsonNode node;
    node.parse(T2);
    EXPECT_EQ(node.getValueAsUnsignedInt("NOT",-3),(unsigned int) -3);
}

// Test: Parse get unsigned integer
TEST(CJsonNode,testGetLongExistingKey) {
    CJsonNode node;
    node.parse(T2);
    CJsonNode * pSubNode = node.find("T");
    EXPECT_TRUE(pSubNode->isNumberValue());
    EXPECT_EQ(node.getValueAsLong("T",-8),(long) 1);
}

// Test: Parse get integer
TEST(CJsonNode,testGetLongNotExistingKey) {
    CJsonNode node;
    node.parse(T2);
    EXPECT_EQ(node.getValueAsLong("NOT",-3),(long) -3);
}

// Test: Parse get float / double
TEST(CJsonNode,testGetFloatExistingKey) {
    CJsonNode node;
    node.parse(T2);
    CJsonNode * pSubNode = node.find("F");
    EXPECT_TRUE(pSubNode->isNumberValue());
    EXPECT_EQ(node.getValueAsFloat("F",-8),-9.5);
}

// Test: Parse get float / double
TEST(CJsonNode,testGetFloatNotExistingKey) {
    CJsonNode node;
    node.parse(T2);
    float fData = -3.3;
    EXPECT_NE(node.getValueAsFloat("NOT"),fData);
    EXPECT_EQ(node.getValueAsFloat("NOT",fData),fData);

}

// Test: Parse get boolean - existing key
TEST(CJsonNode,testGetBooleanExistingKey) {
    CJsonNode node;
    node.parse(T2);
    CJsonNode * pSubNode = node.find("B");
    EXPECT_TRUE(pSubNode->isBooleanValue());
    EXPECT_TRUE(node.getValueAsBool("B"));
    EXPECT_TRUE(node.getValueAsBool("B",false));
}

// Test: Parse get boolean - not existing key
TEST(CJsonNode,testGetBooleanNotExists) {
    CJsonNode node;
    node.parse(T2);
    EXPECT_TRUE(node.getValueAsBool("NOT",true));
    EXPECT_FALSE(node.getValueAsBool("NOT"));
    EXPECT_FALSE(node.getValueAsBool("NOT",false));
}

#pragma endregion

#pragma region store functions

// Test: Parse store integer
TEST(CJsonNode,testStoreIntExistingKey) {
    CJsonNode node;
    node.parse(T2);
    int nValue = -9;
    bool bResult = node.storeValueIf("T",&nValue);
    EXPECT_EQ(nValue,1);
    EXPECT_TRUE(bResult);
}

// Test: Parse store integer - key does not exist
TEST(CJsonNode,testStoreIntNonExistingKey) {
    CJsonNode node;
    node.parse(T2);
    int nValue = -9;
    bool bResult = node.storeValueIf("X",&nValue);
    EXPECT_EQ(nValue,-9);
    EXPECT_FALSE(bResult);
}

// Test: Parse store integer
TEST(CJsonNode,testStoreUIntExistingKey) {
    CJsonNode node;
    node.parse(T2);
    unsigned int nValue = -9;
    bool bResult = node.storeValueIf("T", &nValue);
    EXPECT_EQ(nValue,1);
    EXPECT_TRUE(bResult);
}

// Test: Parse store integer - key does not exist
TEST(CJsonNode,testStoreUIntNonExistingKey) {
    CJsonNode node;
    node.parse(T2);
    unsigned int nValue = -9;
    bool bResult = node.storeValueIf("X",&nValue);
    EXPECT_EQ(nValue,(unsigned int) -9);
    EXPECT_FALSE(bResult);
}

// Test: Parse store long
TEST(CJsonNode,testStoreLongExistingKey) {
    CJsonNode node;
    node.parse(T2);
    long nValue = -9;
    bool bResult = node.storeValueIf("T", &nValue);
    EXPECT_EQ(nValue,1);
    EXPECT_TRUE(bResult);
}

// Test: Parse store long - key does not exist
TEST(CJsonNode,testStoreLongNonExistingKey) {
    CJsonNode node;
    node.parse(T2);
    long nValue = -9;
    bool bResult = node.storeValueIf("X",&nValue);
    EXPECT_EQ(nValue,(long) -9);
    EXPECT_FALSE(bResult);
}

// Test: Parse store float
TEST(CJsonNode,testStoreFloatExistingKey) {
    CJsonNode node;
    node.parse(T2);
    float fValue = +5.5;
    SerialPrintf("KEY::: %s :::",node.getValue("F"));
    bool bResult = node.storeValueIf("F",&fValue);
    EXPECT_EQ(fValue,-9.5);
    EXPECT_TRUE(bResult);
}

// Test: Parse store integer - key does not exist
TEST(CJsonNode,testStoreFloatNonExistingKey) {
    CJsonNode node;
    node.parse(T2);
    float fValue = +5.5;
    bool bResult = node.storeValueIf("NOT",&fValue);
    EXPECT_EQ(fValue,+5.5);
    EXPECT_FALSE(bResult);
}

// Test: Parse store booolean
TEST(CJsonNode,testStoreBoolExistingKey) {
    CJsonNode node;
    node.parse(T2);
    bool bValue = false;
    bool bResult = node.storeValueIf("B",&bValue);
    EXPECT_TRUE(bValue);
    EXPECT_TRUE(bResult);
}

// Test: Parse store boolean - key does not exist
TEST(CJsonNode,testStoreBoolNonExistingKey) {
    CJsonNode node;
    node.parse(T2);
    bool bValue = false;
    bool bResult = node.storeValueIf("NOT",&bValue);
    EXPECT_FALSE(bValue);
    EXPECT_FALSE(bResult);
}

#pragma endregion

#pragma region set values

// set string
TEST(CJsonNode,testSetSimpleStringValue) {
    CJsonNode oNode;
    oNode.parse("{ \"data\": 3 }");
    oNode.setValue("test","otto");
    EXPECT_EQ(oNode.getValueAsInt("data"),3);
    EXPECT_STREQ(oNode.getValue("test"),"otto");
}

// test replace element
TEST(CJsonNode,testReplaceStringValue) {
    CJsonNode oNode;
    oNode.parse("{ \"data\": 3 }");
    oNode.setValue("data","otto");
    EXPECT_NE(oNode.getValueAsInt("data"),3);
    EXPECT_STREQ(oNode.getValue("data"),"otto");
}

// test set bool value
TEST(CJsonNode,testSetSimpleBoolValue) {
    CJsonNode oNode;
    oNode.parse("{ \"data\": 3 }");
    bool bData = true;
    oNode.setValue("test",bData);
    EXPECT_TRUE(oNode.getValueAsBool("test"));
    EXPECT_STREQ(oNode.getValue("test"),"true");
}

// test set pseudo bool value
TEST(CJsonNode,testSetPseudoBoolValue) {
    CJsonNode oNode;
    oNode.parse("{ \"data\": 3 }");
    bool bData = true;
    oNode.setValue("test","+");
    oNode.setValue("t1","oN");
    EXPECT_TRUE(oNode.getValueAsBool("test"));
    EXPECT_TRUE(oNode.getValueAsBool("t1"));
}

// test set pseudo bool value
TEST(CJsonNode,testSetSimpleFloatValue) {
    CJsonNode oNode;
    oNode.parse("{ \"data\": 3 }");
    float fData = 1.34;
    oNode.setValue("test",fData);
    EXPECT_EQ(oNode.getValueAsFloat("test"),fData);
}

// test set integer  value
TEST(CJsonNode,testSetSimpleIntValue) {
    CJsonNode oNode;
    oNode.parse("{ \"data\": 3 }");
    int nData = -555;
    oNode.setValue("test",nData);
    SerialPrintf("INT::: %s %d == %d (%d) ### ",oNode.getValue("test"), nData,oNode.getValueAsInt("test",5),oNode.find("test")->isNumberValue());
    EXPECT_EQ(oNode.getValueAsInt("test"),nData);
}


#pragma endregion



#pragma region dump and special behaviours

// Test: dump outputs key-value pairs
TEST(CJsonNode,testDump) {
    CJsonNode node;
    node.parse("{\"key1\":\"value1\"}");
    node.dump(); // Should not crash
    EXPECT_TRUE(true);
}

// Test: Nested objects
TEST(CJsonNode,testNestedObjects) {
    CJsonNode node;
    CJsonNode* child = node.getObject("child",true);
    CJsonNode* grandchild = child->getObject("grandchild",true);
    EXPECT_NE(grandchild,nullptr);
    EXPECT_EQ(node.getObject("child"), child);
}

// Test: Parse JSON with escaped quotes
TEST(CJsonNode,testParseEscapedQuotes) {
    CJsonNode node;
    node.parse("{\"key\":\"value\\\"quoted\\\"\"}");
    EXPECT_TRUE(node.exists("key"));
    EXPECT_STREQ(node.getValue("key"),"value\"quoted\"");
}

// Test: Multiple key-value pairs
TEST(CJsonNode,testMultipleKeyValues) {
    CJsonNode node;
    node.parse("{\"key1\":\"value1\",\"key2\":\"value2\"}");
    EXPECT_TRUE(node.exists("key1"));
    EXPECT_TRUE(node.exists("key2"));
}

// Test: Parse with nested objects
TEST(CJsonNode,testParseNestedJson) {
    CJsonNode node;;
    node.parse("{\"parent\":{\"child\":\"value\"}}");
    CJsonNode* parent = node.getObject("parent");
    EXPECT_NE(parent,nullptr);
}
// Test: Parse with complex objects
TEST(CJsonNode,testParseComplexJson) {
    CJsonNode node;;
    node.parse(T2);
    CJsonNode* pSubObj = node.getObject("S");
    EXPECT_NE(pSubObj,nullptr);
    EXPECT_EQ(pSubObj->getType(),CJsonNode::ELEMENT_TYPE::OBJECT);
    EXPECT_STREQ(pSubObj->getValue("Sub"),"1");
    CJsonNode *pSubElement = pSubObj->find("Sub");
    EXPECT_TRUE(pSubElement->isNumberValue());
}

#pragma endregion

