#pragma once
#include <gtest/gtest.h>
#include <Vars.h>
#include <climits>
#include <type_traits>

TEST(CVarTableHardening, LongNamesAndCaseSensitivity) {
    String name;
    for (int i = 0; i < 1024; ++i) name += 'A';
    CVarTable table;
    CVar* original = table.set(name, "value");
    ASSERT_NE(original, nullptr);
    EXPECT_EQ(table.find(name), original);
    String lower;
    for (int i = 0; i < 1024; ++i) lower += 'a';
    EXPECT_EQ(table.find(lower), original);
    EXPECT_EQ(table.set(lower, "updated"), original);
    EXPECT_STREQ(original->getValue(), "updated");
    CVarTable sensitive(true);
    sensitive.set(name, "value");
    EXPECT_EQ(sensitive.find(lower), nullptr);
    EXPECT_NE(sensitive.find(name), nullptr);
}

TEST(CVarTableHardening, NullAndAliasedValuesAreSafe) {
    CVarTable table;
    CVar* empty = table.set(nullptr, "empty");
    EXPECT_EQ(table.set("", "updated"), empty);
    EXPECT_EQ(table.find(nullptr), nullptr);
    CVar* var = table.set("key", "abcdef");
    ASSERT_NE(var, nullptr);
    table.set("key", var->getValue());
    EXPECT_STREQ(var->getValue(), "abcdef");
    table.set("key", var->getValue() + 2);
    EXPECT_STREQ(var->getValue(), "cdef");
    var->setVarName(var->getName());
    EXPECT_STREQ(var->getName(), "key");
    var->setVarName(var->getKeyName());
    EXPECT_EQ(table.find("KEY"), var);
    table.set("key", nullptr);
    EXPECT_STREQ(var->getValue(), "");
    CVar uninitialized;
    EXPECT_STREQ(uninitialized.getValue(), "");
    EXPECT_EQ(uninitialized.getIntValue(), 0);
    EXPECT_EQ(uninitialized.getUnsignedLongValue(), 0UL);
    EXPECT_FALSE(uninitialized.getBoolValue());
    var->setValue("9999999999999999999999999999999");
    EXPECT_EQ(var->getIntValue(), INT_MAX);
    var->setValue("-9999999999999999999999999999999");
    EXPECT_EQ(var->getIntValue(), INT_MIN);
}

TEST(CVarTableHardening, CriticalMetadataRoundTripAndRepeatedExport) {
    CVarTable source;
    source.set("secret", "password")->setCriticalVar(true);
    source.set("other", "second")->setCriticalVar(true);
    source.set("wifi.name", "literal");
    source.set("", "empty key");
    JsonNode config;
    source.writeConfigTo(config, false);
    source.writeConfigTo(config, false);
    EXPECT_EQ(config.Elements.size(), 5u);
    ASSERT_NE(config.find("__CN.n.1"), nullptr);
    EXPECT_EQ(config.getValueAsInt("__CN.n.0"), 2);
    ASSERT_NE(config.find("wifi.name", false), nullptr);
    CVarTable restored;
    restored.readConfigFrom(config);
    ASSERT_NE(restored.find("secret"), nullptr);
    EXPECT_TRUE(restored.find("secret")->isCriticalVar());
    EXPECT_TRUE(restored.find("other")->isCriticalVar());
    EXPECT_STREQ(restored.getValue("wifi.name", ""), "literal");
    EXPECT_STREQ(restored.getValue("", ""), "empty key");
    source.writeConfigTo(config, true);
    EXPECT_STREQ(config.getValue("secret"), "******");
    restored.readConfigFrom(config);
    EXPECT_STREQ(restored.getValue("secret", ""), "password");
    source.find("secret")->setCriticalVar(false);
    source.find("other")->setCriticalVar(false);
    source.writeConfigTo(config, false);
    EXPECT_EQ(config.find("__CN"), nullptr);
}

TEST(CVarTableHardening, RejectsContainersAndBoundsUntrustedMetadata) {
    CVarTable table;
    JsonNode config;
    ASSERT_NE(config.parse("{\"key\":\"value\",\"object\":{},\"array\":[],\"__CN\":{\"n\":{\"0\":2147483647,\"1\":\"key\",\"bad\":\"other\"}}}"), nullptr);
    table.readConfigFrom(config);
    EXPECT_EQ(table.find("object"), nullptr);
    EXPECT_EQ(table.find("array"), nullptr);
    ASSERT_NE(table.find("key"), nullptr);
    EXPECT_TRUE(table.find("key")->isCriticalVar());
    JsonNode array;
    ASSERT_NE(array.parse("[1,2]"), nullptr);
    table.readConfigFrom(array);
    table.writeConfigTo(array, false);
    EXPECT_STREQ(array.getAsJsonText(), "[1,2]");
}

static_assert(!std::is_copy_constructible<CVarTable>::value, "Tables own their entries");
static_assert(!std::is_copy_constructible<CVar>::value, "Variables own their buffers");

#ifndef NATIVE_RUNTIME
TEST(CVarTableHardening, FlashNamesValuesAndDefaults) {
    CVarTable table;
    table.set(F("FlashKey"), "RAM value");
    EXPECT_STREQ(table.getValue(F("flashkey"), "default"), "RAM value");
    table.set(F("FlashKey"), F("Flash value"));
    EXPECT_STREQ(table.getValue("FLASHKEY", ""), "Flash value");
    table.set("ram", F("from flash"));
    EXPECT_STREQ(table.getValue("ram", ""), "from flash");
    table.set(String("string"), F("from flash"));
    EXPECT_STREQ(table.getValue("string", ""), "from flash");
    EXPECT_STREQ(table.getValue(F("missing"), F("fallback")), "fallback");
    table.set(F("number"), 42);
    EXPECT_EQ(table.getIntValue(F("number"), 0), 42);
    table.set(F("flag"), true);
    EXPECT_TRUE(table.getBoolValue(F("flag"), false));
    table.set(F("ulong"), ULONG_MAX);
    EXPECT_EQ(table.find(F("ulong"))->getUnsignedLongValue(), ULONG_MAX);
    CVar var(F("name"), F("value"));
    EXPECT_STREQ(var.getName(), "name");
    EXPECT_STREQ(var.getValue(), "value");
    var.setValue(F("changed"));
    EXPECT_STREQ(var.getValue(), "changed");
}
#endif
