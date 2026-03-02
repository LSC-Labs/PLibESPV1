
#include <gtest/gtest.h>
#include "SimpleJsonNode.h"

// Test: Constructor with name
TEST(CSimpleJsonNode,testConstructorWithName) {
    CSimpleJsonNode node("TestNode");
    EXPECT_EQ(node.getObject("NonExistent"),nullptr);
}

// Test: Default constructor
TEST(CSimpleJsonNode,testDefaultConstructor) {
    CSimpleJsonNode node;
    EXPECT_EQ(node.getObject("Test"),nullptr);
}

// Test: getValue with default
TEST(CSimpleJsonNode,testGetValueDefault) {
    CSimpleJsonNode node("root");
    const char* result = node.getValue("nonexistent", "default");
    EXPECT_STREQ(result, "default");
}

// Test: createOrGetObject creates new object
TEST(CSimpleJsonNode,testCreateoGObjectNew) {
    CSimpleJsonNode node("root");
    CSimpleJsonNode* obj1 = node.createOrGetObject("child");
    EXPECT_NE(obj1,nullptr);
    CSimpleJsonNode* obj2 = node.createOrGetObject("child");
    // Expecting to get the same node...
    EXPECT_EQ(obj1,obj2);
}

// Test: getObject returns nullptr for nonexistent
TEST(CSimpleJsonNode,testGetObjectNonexistent) {
    CSimpleJsonNode node("root");
    CSimpleJsonNode* result = node.getObject("nonexistent");
    EXPECT_EQ(result,nullptr);
}

// Test: exists returns false for nonexistent key
TEST(CSimpleJsonNode,testExistsNonexistent) {
    CSimpleJsonNode node("root");
    EXPECT_FALSE(node.exists("key"));
}

// Test: Simple JSON parse
TEST(CSimpleJsonNode,testParseSimpleJson) {
    CSimpleJsonNode node("root");
    node.parse("{\"name\":\"value\"}");
    EXPECT_TRUE(node.exists("name"));
}

// Test: Destructor cleanup
TEST(CSimpleJsonNode,testDestructor) {
   
    CSimpleJsonNode* node = new CSimpleJsonNode("root");
    node->createOrGetObject("child");
    delete node;

    // No crash indicates successful cleanup
    ASSERT_TRUE(true);
}

// Test: getValue returns correct value
TEST(CSimpleJsonNode,testGetValueCorrect) {
    CSimpleJsonNode node("root");
    node.parse("{\"key\":\"testvalue\"}");
    const char* result = node.getValue("key");
    EXPECT_STREQ(result, "testvalue");
}

// Test: dump outputs key-value pairs
TEST(CSimpleJsonNode,testDump) {
    CSimpleJsonNode node;
    node.parse("{\"key1\":\"value1\"}");
    node.dump(); // Should not crash
    EXPECT_TRUE(true);
}

// Test: Nested objects
TEST(CSimpleJsonNode,testNestedObjects) {
    CSimpleJsonNode node("root");
    CSimpleJsonNode* child = node.createOrGetObject("child");
    CSimpleJsonNode* grandchild = child->createOrGetObject("grandchild");
    EXPECT_NE(grandchild,nullptr);
    EXPECT_EQ(node.getObject("child"), child);
}

// Test: Parse JSON with escaped quotes
TEST(CSimpleJsonNode,testParseEscapedQuotes) {
    CSimpleJsonNode node("root");
    node.parse("{\"key\":\"value\\\"quoted\\\"\"}");
    EXPECT_TRUE(node.exists("key"));
}

// Test: Multiple key-value pairs
TEST(CSimpleJsonNode,testMultipleKeyValues) {
    CSimpleJsonNode node("root");
    node.parse("{\"key1\":\"value1\",\"key2\":\"value2\"}");
    EXPECT_TRUE(node.exists("key1"));
    EXPECT_TRUE(node.exists("key2"));
}

// Test: Parse with nested objects
TEST(CSimpleJsonNode,testParseNestedJson) {
    CSimpleJsonNode node("root");
    node.parse("{\"parent\":{\"child\":\"value\"}}");
    CSimpleJsonNode* parent = node.getObject("parent");
    EXPECT_NE(parent,nullptr);
}
