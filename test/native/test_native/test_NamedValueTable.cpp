
#include <gtest/gtest.h>
#include "NamedValueTable.h"

TEST(CNamedValueEntry,testStoresKeyAndValue) {
    CNamedValueEntry<int> oEntry("key",42);
    EXPECT_STREQ(oEntry.getKey(),"key");
    EXPECT_EQ(oEntry.value,42);
}

TEST(CNamedValueEntry,testDoesKeyMatchCaseSensitive) {
    CNamedValueEntry<int> oEntry("TestKey",1);
    EXPECT_TRUE(oEntry.doesKeyMatch("TestKey",true));
    EXPECT_FALSE(oEntry.doesKeyMatch("testkey",true));
}

TEST(CNamedValueEntry,testDoesKeyMatchCaseInsensitive) {
    CNamedValueEntry<int> oEntry("TestKey",1);
    EXPECT_TRUE(oEntry.doesKeyMatch("testkey",false));
}

TEST(CNamedValueTable,testGetReturnsDefaultWhenMissing) {
    CNamedValueTable<int> oTable(true,-1);
    EXPECT_EQ(oTable.get("missing"),-1);
}

TEST(CNamedValueTable,testSetAndFindByKey) {
    CNamedValueTable<int> oTable;
    CNamedValueEntry<int> *pEntry = oTable.set("one",1);
    EXPECT_NE(pEntry,nullptr);
    EXPECT_EQ(oTable.size(),1);
    EXPECT_EQ(oTable.findFirstByKey("one"),pEntry);
    EXPECT_EQ(oTable.get("one"),1);
}

TEST(CNamedValueTable,testCaseInsensitiveKeySearch) {
    CNamedValueTable<int> oTable(false,-1);
    oTable.set("WiFi",7);
    EXPECT_TRUE(oTable.hasKeyEntry("wifi"));
    EXPECT_EQ(oTable.get("WIFI"),7);
}

TEST(CNamedValueTable,testFindNextByValueReturnsFollowingEntry) {
    CNamedValueTable<int> oTable;
    CNamedValueEntry<int> *pFirst = oTable.set("first",7);
    CNamedValueEntry<int> *pSecond = oTable.set("second",7);

    EXPECT_EQ(oTable.findFirstByValue(7),pFirst);
    EXPECT_EQ(oTable.findNextByValue(7,pFirst),pSecond);
    EXPECT_EQ(oTable.findNextByValue(7,pSecond),nullptr);
}

TEST(CNamedValueTable,testGetKeysKeepsInsertionOrder) {
    CNamedValueTable<int> oTable;
    oTable.set("first",1);
    oTable.set("second",2);

    std::vector<const char *> tKeys = oTable.getKeys();
    ASSERT_EQ(tKeys.size(),2);
    EXPECT_STREQ(tKeys[0],"first");
    EXPECT_STREQ(tKeys[1],"second");
}

TEST(CMultiNameUniqueValueTable,testRejectsDuplicateValue) {
    CMultiNameUniqueValueTable<int> oTable;
    EXPECT_NE(oTable.set("first",5),nullptr);
    EXPECT_EQ(oTable.set("second",5),nullptr);
    EXPECT_EQ(oTable.size(),1);
    EXPECT_FALSE(oTable.hasKeyEntry("second"));
}

TEST(CUniqueNameMultiValueTable,testAllowsDuplicateValuesForDifferentKeys) {
    CUniqueNameMultiValueTable<int> oTable;
    EXPECT_NE(oTable.set("first",5),nullptr);
    EXPECT_NE(oTable.set("second",5),nullptr);
    EXPECT_EQ(oTable.size(),2);
}

TEST(CUniqueNameMultiValueTable,testUpdatesExistingKeyWithoutAddingEntry) {
    CUniqueNameMultiValueTable<int> oTable;
    CNamedValueEntry<int> *pOriginal = oTable.set("key",5);
    CNamedValueEntry<int> *pUpdated = oTable.set("key",9);

    EXPECT_EQ(pUpdated,pOriginal);
    EXPECT_EQ(oTable.size(),1);
    EXPECT_EQ(oTable.get("key"),9);
}
