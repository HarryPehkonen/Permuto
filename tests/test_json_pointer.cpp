#include "../src/json_pointer.hpp"
#include <gtest/gtest.h>

using namespace permuto;

class JsonPointerTest : public ::testing::Test {
protected:
    nlohmann::json test_data = R"({
        "user": {
            "id": 123,
            "name": "Alice",
            "settings": {
                "theme": "dark"
            }
        },
        "items": [
            {"name": "item1", "value": 10},
            {"name": "item2", "value": 20}
        ],
        "special~key": "tilde",
        "key/with/slashes": "slashes"
    })"_json;
};

TEST_F(JsonPointerTest, RootPath) {
    JsonPointer pointer("");
    EXPECT_TRUE(pointer.is_root());

    auto result = pointer.resolve(test_data);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, test_data);
}

// RFC 6901 §3-4: "" is the whole document; "/" is ONE token — the empty string — naming
// the member whose key is empty. They are different pointers, and parse_path() treated
// both as the root, because it ran the pointer's tail through std::getline(), which
// emits no final empty token: resolve("/") returned the whole document. Found by
// fuzz/fuzz_permuto.cpp's round-trip oracle, 2026-09-20; see INCIDENTS.md.
TEST_F(JsonPointerTest, SlashIsTheMemberWithTheEmptyKey) {
    nlohmann::json document = {{"", 1}, {"a", 2}};

    JsonPointer pointer("/");
    EXPECT_FALSE(pointer.is_root());
    ASSERT_EQ(pointer.tokens().size(), 1U);
    EXPECT_EQ(pointer.tokens()[0], "");

    auto result = pointer.resolve(document);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, 1);
}

// The same getline bug ate a TRAILING empty token: "/a/" names member "a", then the
// member whose key is empty inside it.
TEST_F(JsonPointerTest, TrailingSlashNamesTheEmptyKeyMember) {
    nlohmann::json document = {{"a", {{"", 2}, {"b", 3}}}};

    JsonPointer pointer("/a/");
    ASSERT_EQ(pointer.tokens().size(), 2U);
    EXPECT_EQ(pointer.tokens()[0], "a");
    EXPECT_EQ(pointer.tokens()[1], "");

    auto result = pointer.resolve(document);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, 2);
}

// "//" is two empty tokens, so it addresses document[""][""] — and must FAIL against a
// document whose "" member is a scalar rather than quietly returning that scalar.
TEST_F(JsonPointerTest, DoubleSlashIsTwoEmptyTokens) {
    nlohmann::json nested = {{"", {{"", 9}}}};

    JsonPointer pointer("//");
    ASSERT_EQ(pointer.tokens().size(), 2U);

    auto result = pointer.resolve(nested);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, 9);

    auto missing = pointer.resolve(nlohmann::json{{"", 1}});
    EXPECT_FALSE(missing.has_value());
}

TEST_F(JsonPointerTest, SimpleObjectAccess) {
    JsonPointer pointer("/user/id");
    EXPECT_FALSE(pointer.is_root());

    auto result = pointer.resolve(test_data);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, 123);
}

TEST_F(JsonPointerTest, NestedObjectAccess) {
    JsonPointer pointer("/user/settings/theme");

    auto result = pointer.resolve(test_data);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, "dark");
}

TEST_F(JsonPointerTest, ArrayAccess) {
    JsonPointer pointer("/items/0/name");

    auto result = pointer.resolve(test_data);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, "item1");

    JsonPointer pointer2("/items/1/value");
    auto result2 = pointer2.resolve(test_data);
    ASSERT_TRUE(result2.has_value());
    EXPECT_EQ(*result2, 20);
}

TEST_F(JsonPointerTest, EscapedKeys) {
    JsonPointer pointer("/special~0key");
    auto result = pointer.resolve(test_data);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, "tilde");

    JsonPointer pointer2("/key~1with~1slashes");
    auto result2 = pointer2.resolve(test_data);
    ASSERT_TRUE(result2.has_value());
    EXPECT_EQ(*result2, "slashes");
}

TEST_F(JsonPointerTest, MissingKeys) {
    JsonPointer pointer("/user/missing");
    auto result = pointer.resolve(test_data);
    EXPECT_FALSE(result.has_value());

    JsonPointer pointer2("/missing/path");
    auto result2 = pointer2.resolve(test_data);
    EXPECT_FALSE(result2.has_value());
}

TEST_F(JsonPointerTest, ArrayOutOfBounds) {
    JsonPointer pointer("/items/10");
    auto result = pointer.resolve(test_data);
    EXPECT_FALSE(result.has_value());
}

TEST_F(JsonPointerTest, InvalidArrayIndex) {
    JsonPointer pointer("/items/invalid");
    auto result = pointer.resolve(test_data);
    EXPECT_FALSE(result.has_value());
}

TEST_F(JsonPointerTest, InvalidPath) {
    EXPECT_THROW(JsonPointer("invalid"), std::invalid_argument);
    EXPECT_THROW(JsonPointer("missing_slash"), std::invalid_argument);
}