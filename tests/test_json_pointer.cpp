#include <gtest/gtest.h>
#include "../src/json_pointer.hpp"

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