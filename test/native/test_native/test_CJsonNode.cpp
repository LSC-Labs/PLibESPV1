
#include <gtest/gtest.h>
#include "JsonNode.h"
#include "LSCUtils.h"


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


TEST(CJsonNode,testGetExistingElement) {
    CJsonNode oNode;
    CJsonNode *pNode = oNode.getElement("NON");
    EXPECT_EQ(pNode,nullptr);
}
// Test: getValue with default
TEST(CJsonNode,testGetValueDefault) {
    CJsonNode node;
    const char* result = node.getValue("nonexistent", "default");
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

// Test: Simple JSON parse
TEST(CJsonNode,testParseEmptyValueJson) {
    CJsonNode node;
    node.parse("{ \"wifi\":{\"key\":\"test\",\"name\":\"\"}}");
    SerialPrintf(">> %s << ",node.getAsJsonText());
    CJsonNode * pSub = node.getObject("wifi");
    EXPECT_TRUE(pSub->exists("name"));
    EXPECT_STREQ(pSub->getValue("name"),"");
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
TEST(CJsonNode,testToClearElementValueText) {
    CJsonNode* node = new CJsonNode();
    CJsonNode * pSubNode = node->getObject("child",true);
    EXPECT_TRUE(node->exists("child"));
    node->clear();
    EXPECT_FALSE(node->exists("child"));
}

// Test: Destructor cleanup
TEST(CJsonNode,testClearAllObjectsInNodes) {
    CJsonNode oNode;
    oNode.parse("{ \"wifi\":{\"key\":\"test\",\"name\":\"\"}}");
    EXPECT_EQ(oNode.Elements.size(),1);
    CJsonNode *pNode = oNode.getObject("wifi");
    EXPECT_EQ(pNode->Elements.size(),2);
    pNode->clear();
    EXPECT_EQ(pNode->Elements.size(),0);
    oNode.clear();
    EXPECT_EQ(oNode.Elements.size(),0);
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


// Test: Parse get unsigned integer
TEST(CJsonNode,testGetCharPointerExistingKey) {
    CJsonNode node;
    node.parse("{ \"X\": \"Teststring\" }");
    const char *pszResult = node.getValue("X","DD");
    SerialPrintf(">>> \"%s\" (%s) ### ",pszResult,node.getValue());
    EXPECT_NE(pszResult,nullptr);
    EXPECT_STREQ(pszResult,"Teststring");
    EXPECT_TRUE(strlen(pszResult) > 0);
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

// Test: Parse store boolean - key does not exist
TEST(CJsonNode,testStoreNotIFMaskMatches) {
    CJsonNode node;
    const char *pszNot = "****";
    const char *pszShould = "JoJoBa";

    node.parse("{ \"X\": \"****\" }");

    String strResult = node.getValue("X");
    SerialPrintf(">>> NOT mask matches :%d ### ",strResult == pszNot);
    strResult = pszShould;
    node.storeValueIfNot("X",strResult,pszNot);
    SerialPrintf("Result : %s - expected - %s ### ",strResult.c_str(), pszShould );
    EXPECT_STREQ(strResult.c_str(),"JoJoBa");
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

TEST(CJsonNode,testSetRealStringValue) {
    CJsonNode oNode;
    oNode.parse("{ \"data\": 3 }");
    String strData = "Hello world";
    oNode.setValue("test",strData);
    EXPECT_STREQ(oNode.getValue("test"),"Hello world");
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


// test set integer  value
TEST(CJsonNode,testFindSubValue) {
    CJsonNode oNode;
    oNode.parse("{ \"data\": 3 }");
    int nData = -555;
    CJsonNode *pSubNode = oNode.getObject("sub",true);
    (*pSubNode)["value"] = nData;
    int nResult = pSubNode->getValueAsInt("value");
    SerialPrintf(">>> result (%d) == value (%d)",nResult,nData);
    EXPECT_EQ(nResult,nData);
}


// test set integer  value
TEST(CJsonNode,testSetSubObjectValue) {
    const char *pszSearchName = "S.Sub";
    CJsonNode oNode;
    oNode.parse(T1);
    int nExpected   = 1;
    CJsonNode *pSubNode = oNode.find(pszSearchName);
    EXPECT_NE(pSubNode,nullptr);
    int nData       = oNode.getValueAsInt(pszSearchName);
    SerialPrintf(">>> expeted (%d) == value (%d) ###",nExpected,nData);
    EXPECT_EQ(nExpected,nData);
}

// test set integer  value
TEST(CJsonNode,testCreatePathToElement) {
    const char *pszSearchName = "Test.const.Sub";
    CJsonNode oNode;
    CJsonNode *pSubNode = oNode.createJsonPathToElement(pszSearchName);
    EXPECT_NE(pSubNode,nullptr);
    EXPECT_FALSE(pSubNode->exists("Sub"));
    EXPECT_TRUE(oNode.exists("Test.const"));
    EXPECT_NE(oNode.getObject("Test.const"),nullptr);
}

TEST(CJsonNode,testGetAsObjectWithPath) {
    const char *pszSearchName = "Test.const.Sub";
    CJsonNode oNode;
    CJsonNode *pSubNode = oNode.getObject(pszSearchName,true);
    SerialPrintf(">>> %s ###",oNode.getAsJsonText());
    EXPECT_FALSE(pSubNode->isJsonObject("Sub"));
    EXPECT_TRUE(oNode.exists(pszSearchName));
}

TEST(CJsonNode,testGetAsArrayWithPath) {
    const char *pszSearchName = "Test.const.Sub";
    CJsonNode oNode;
    CJsonNode *pSubNode = oNode.getArray(pszSearchName,true);
    SerialPrintf(">>> %s ###",oNode.getAsJsonText());
    EXPECT_FALSE(pSubNode->isJsonArray("Sub"));
    EXPECT_TRUE(oNode.exists(pszSearchName));
}

TEST(CJsonNode,testGetAsElementWithPath) {
    const char *pszSearchName = "Test.const.Sub";
    CJsonNode oNode;
    CJsonNode *pSubNode = oNode.getElement(pszSearchName,true);
    SerialPrintf(">>> %s ###",oNode.getAsJsonText());
    EXPECT_FALSE(pSubNode->isJsonArray("Sub"));
    EXPECT_TRUE(oNode.exists(pszSearchName));
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


TEST(CJsonNode, RejectsMalformedDocumentsWithoutChangingTree) {
    const char* invalid[] = {
        "", " ", "{", "[", "{\"x\":", "{\"x\":1", "[1", "{]", "[}",
        "{\"x\" 1}", "{x:1}", "{\"x\":}", "[,1]", "[1,]", "{\"x\":1,}",
        "[1 2]", "[1:2]", "{}garbage", "{}{}", "true false", "[01]", "[+1]",
        "[.1]", "[1.]", "[1e]", "[1e+]", "[NaN]", "[TRUE]", "[tru]",
        "[\"unterminated]", "[\"\\", "[\"\\q\"]", "[\"a\nb\"]",
        "[\"\\u12\"]", "[\"\\ud800\"]", "[\"\\udc00\"]", "[\"\\ud800\\u0041\"]",
        "[\"\\u0000\"]", "{\"x\":1,\"x\":2}", "\v{}"
    };
    CJsonNode node;
    ASSERT_NE(node.parse("{\"original\":42}"), nullptr);
    for (const char* input : invalid) {
        SCOPED_TRACE(input);
        EXPECT_EQ(node.parse(input), nullptr);
        EXPECT_STREQ(node.getAsJsonText(), "{\"original\":42}");
    }
    EXPECT_EQ(node.parse(nullptr), nullptr);
    EXPECT_EQ(node.getValueAsInt("original"), 42);
}

TEST(CJsonNode, ParsesScalarsAndReplacesPreviousTree) {
    const char* valid[] = {"0", "-0", "1.25e-10", "-42E+2", "true", "false", "null", "\"text\"", "{}", "[]"};
    CJsonNode node;
    for (const char* input : valid) {
        SCOPED_TRACE(input);
        ASSERT_NE(node.parse(input), nullptr);
        EXPECT_STREQ(node.getAsJsonText(), input);
    }
    ASSERT_NE(node.parse(" {\"child\":{\"x\":1}} \r\n"), nullptr);
    ASSERT_NE(node.find("child"), nullptr);
    EXPECT_EQ(node.find("child")->getParentNode(), &node);
    ASSERT_NE(node.parse("[2]"), nullptr);
    EXPECT_EQ(node.Elements.size(), 1u);
    EXPECT_EQ(node.Elements[0]->getParentNode(), &node);
}

TEST(CJsonNode, EscapesAndUnicodeRoundTrip) {
    CJsonNode node;
    ASSERT_NE(node.parse("{\"\":0,\"a.b\":1,\"q\\\"\\\\\":\"\\b\\f\\n\\r\\t\\/\\u00e4\\ud83d\\ude00\"}"), nullptr);
    EXPECT_NE(node.find("a.b", false), nullptr);
    ASSERT_NE(node.find("q\"\\", false), nullptr);
    EXPECT_STREQ(node.find("q\"\\", false)->getValue(), "\b\f\n\r\t/ä😀");
    CJsonNode copy;
    ASSERT_NE(copy.parse(node.getAsJsonText()), nullptr);
    EXPECT_STREQ(copy.getAsJsonText(), node.getAsJsonText());
    ASSERT_NE(copy.parse(node.getAsJsonTextPretty()), nullptr);
    EXPECT_NE(copy.find("", false), nullptr);
}

TEST(CJsonNode, LimitsContainerNesting) {
    CJsonNode node;
    std::string input(32, '[');
    input += "0";
    input += std::string(32, ']');
    ASSERT_NE(node.parse(input.c_str()), nullptr);
    input = "[" + input + "]";
    EXPECT_EQ(node.parse(input.c_str()), nullptr);
}

TEST(CJsonNode, LongPathSegmentsDoNotOverflow) {
    CJsonNode node;
    std::string path(1024, 'a');
    path += ".value";
    node.setValue(path.c_str(), 7);
    EXPECT_EQ(node.getValueAsInt(path.c_str()), 7);
}

TEST(CJsonNode, RejectsInvalidUtf8) {
    const char* invalid[] = {"\"\x80\"", "\"\xc0\xaf\"", "\"\xe0\x80\xaf\"",
        "\"\xed\xa0\x80\"", "\"\xf4\x90\x80\x80\"", "\"\xc2", "\"\xe2\x82"};
    CJsonNode node;
    for (const char* input : invalid) EXPECT_EQ(node.parse(input), nullptr);
}

TEST(CJsonNode, RejectsEveryTruncatedPrefix) {
    const std::string document = "{\"key\":[true,false,null,-1.2e+3,\"a\\\"b\\u00e4\\ud83d\\ude00\",{}]}";
    CJsonNode node;
    for (size_t length = 0; length < document.size(); ++length) {
        SCOPED_TRACE(length);
        const std::string prefix = document.substr(0, length);
        EXPECT_EQ(node.parse(prefix.c_str()), nullptr);
    }
    EXPECT_NE(node.parse(document.c_str()), nullptr);
}

// ---------------------------------------------------------------------------
// Path handling of getArray / getObject / getElement ("a.b")
// ---------------------------------------------------------------------------
TEST(CJsonNode, testGetArrayWithPathCreatesOnce) {
    CJsonNode oRoot;
    CJsonNode *pFirst  = oRoot.getArray("a.list", true);
    CJsonNode *pSecond = oRoot.getArray("a.list", true);
    ASSERT_NE(pFirst, nullptr);
    EXPECT_EQ(pFirst, pSecond);
    EXPECT_EQ(pFirst->getParentNode(), oRoot.getObject("a"));
    EXPECT_EQ(oRoot.getArray("a.list"), pFirst);
    EXPECT_STREQ(oRoot.getAsJsonText(), "{\"a\":{\"list\":[]}}");
}

TEST(CJsonNode, testGetObjectWithPathCreatesOnce) {
    CJsonNode oRoot;
    CJsonNode *pFirst  = oRoot.getObject("a.b.c", true);
    CJsonNode *pSecond = oRoot.getObject("a.b.c", true);
    ASSERT_NE(pFirst, nullptr);
    EXPECT_EQ(pFirst, pSecond);
    EXPECT_EQ(pFirst->getParentNode(), oRoot.getObject("a.b"));
    EXPECT_STREQ(oRoot.getAsJsonText(), "{\"a\":{\"b\":{\"c\":{}}}}");
}

TEST(CJsonNode, testGetElementWithPathCreatesOnce) {
    CJsonNode oRoot;
    CJsonNode *pFirst = oRoot.getElement("cfg.name", true);
    ASSERT_NE(pFirst, nullptr);
    pFirst->setValue("x");
    EXPECT_EQ(oRoot.getElement("cfg.name", true), pFirst);
    EXPECT_EQ(pFirst->getParentNode(), oRoot.getObject("cfg"));
    EXPECT_STREQ(oRoot.getAsJsonText(), "{\"cfg\":{\"name\":\"x\"}}");
}

TEST(CJsonNode, testGetArrayFindsParsedPath) {
    CJsonNode oRoot;
    ASSERT_NE(oRoot.parse("{\"a\":{\"list\":[1,2]}}"), nullptr);
    CJsonNode *pList = oRoot.getArray("a.list");
    ASSERT_NE(pList, nullptr);
    EXPECT_EQ(pList->Elements.size(), 2u);
    EXPECT_EQ(oRoot.getArray("a.list", true), pList);   // no duplicate, data kept
    EXPECT_STREQ(oRoot.getAsJsonText(), "{\"a\":{\"list\":[1,2]}}");
}

TEST(CJsonNode, testGetArrayWrongTypeWithoutCreate) {
    CJsonNode oRoot;
    ASSERT_NE(oRoot.parse("{\"a\":{\"v\":5}}"), nullptr);
    EXPECT_EQ(oRoot.getArray("a.v"), nullptr);
    EXPECT_EQ(oRoot.getArray("a.missing"), nullptr);
    EXPECT_EQ(oRoot.getArray("x.y"), nullptr);
    EXPECT_STREQ(oRoot.getAsJsonText(), "{\"a\":{\"v\":5}}");   // nothing created
}

TEST(CJsonNode, testGetArrayConvertsWrongTypeWithCreate) {
    CJsonNode oRoot;
    ASSERT_NE(oRoot.parse("{\"a\":{\"v\":5}}"), nullptr);
    CJsonNode *pList = oRoot.getArray("a.v", true);
    ASSERT_NE(pList, nullptr);
    EXPECT_STREQ(oRoot.getAsJsonText(), "{\"a\":{\"v\":[]}}");
}

TEST(CJsonNode, testCreatePayloadStructureWithJsonPayload) {
    CJsonNode oDoc;
    CJsonNode *pPayload = oDoc.createPayloadStructure("update","status","{\"a\":1}");
    ASSERT_NE(pPayload, nullptr);
    EXPECT_TRUE(pPayload->isJsonObject());             // object - not a string
    EXPECT_STREQ(oDoc.getAsJsonText(), "{\"command\":\"update\",\"data\":\"status\",\"payload\":{\"a\":1}}");
}

TEST(CJsonNode, testCreatePayloadStructureWithTextPayload) {
    CJsonNode oDoc;
    oDoc.createPayloadStructure("update","msg","hello");
    EXPECT_STREQ(oDoc.getAsJsonText(), "{\"command\":\"update\",\"data\":\"msg\",\"payload\":\"hello\"}");
}

#pragma region legacy (lenient) parsing

// Strict parsing rejects, legacy parsing accepts files written by older versions
static void expectLegacyOnly(const char *pszJson, CJsonNode &oNode) {
    EXPECT_EQ(oNode.parse(pszJson), nullptr) << "strict should reject: " << pszJson;
    EXPECT_NE(oNode.parse(pszJson, true), nullptr) << "legacy should accept: " << pszJson;
    // The result is written as valid JSON again
    CJsonNode oCheck;
    EXPECT_NE(oCheck.parse(oNode.getAsJsonText()), nullptr) << "re-serialized: " << oNode.getAsJsonText();
}

TEST(CJsonNode,testLegacyDuplicateKeysLastWins) {
    CJsonNode oNode;
    expectLegacyOnly("{\"wifi\":{\"ssid\":\"old\"},\"wifi\":{\"ssid\":\"new\",\"ap_mode\":false}}", oNode);
    ASSERT_NE(oNode.getObject("wifi"), nullptr);
    EXPECT_STREQ(oNode.getObject("wifi")->getValue("ssid"), "new");
    EXPECT_EQ(oNode.Elements.size(), (size_t) 1);
}

TEST(CJsonNode,testLegacyEmptyValueAndTrailingComma) {
    CJsonNode oNode;
    expectLegacyOnly("{\"wifi\":{\"ipaddress\": ,\"ssid\":\"MyNet\",},\"msgs\":[1,2,],}", oNode);
    EXPECT_STREQ(oNode.getObject("wifi")->getValue("ssid"), "MyNet");
    EXPECT_TRUE(oNode.getObject("wifi")->exists("ipaddress"));
    EXPECT_EQ(oNode.getArray("msgs")->Elements.size(), (size_t) 2);
}

TEST(CJsonNode,testLegacyRawControlCharsAndLatin1) {
    CJsonNode oNode;
    expectLegacyOnly("{\"pwd\":\"a\tb\",\"name\":\"K\xfc" "che\"}", oNode);
    EXPECT_STREQ(oNode.getValue("pwd"), "a\tb");
    EXPECT_STREQ(oNode.getValue("name"), "K\xc3\xbc" "che");      // Latin-1 => UTF-8
}

TEST(CJsonNode,testLegacyUnquotedTextBomAndTrailingContent) {
    CJsonNode oNode;
    expectLegacyOnly("\xef\xbb\xbf{\"ip\":192.168.4.1,\"on\":true,\"n\":-5} garbage", oNode);
    EXPECT_STREQ(oNode.getValue("ip"), "192.168.4.1");
    EXPECT_EQ(oNode.getValueAsInt("n"), -5);
    EXPECT_TRUE(oNode.getValueAsBool("on"));
}

TEST(CJsonNode,testLegacyStillRejectsBrokenStructure) {
    CJsonNode oNode;
    EXPECT_EQ(oNode.parse("{\"a\":\"unterminated}", true), nullptr);
    EXPECT_EQ(oNode.parse("{\"a\":{\"b\":1}", true), nullptr);
}

TEST(CJsonNode,testStrictValidJsonUnchangedInLegacyMode) {
    CJsonNode oStrict, oLegacy;
    EXPECT_NE(oStrict.parse(T2), nullptr);
    EXPECT_NE(oLegacy.parse(T2, true), nullptr);
    EXPECT_STREQ(oStrict.getAsJsonText(), oLegacy.getAsJsonText());
}

#pragma endregion
