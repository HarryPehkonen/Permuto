#include <gtest/gtest.h>
#include "../src/template_processor.hpp"

using namespace permuto;

class TemplateProcessorTest : public ::testing::Test {
protected:
    nlohmann::json context = R"({
        "user": {
            "id": 123,
            "name": "Alice",
            "email": "alice@example.com"
        },
        "preferences": {
            "theme": "dark",
            "notifications": true
        }
    })"_json;
    
    Options default_options;
    Options interpolation_options;
    Options error_options;
    
    void SetUp() override {
        interpolation_options.enable_interpolation = true;
        error_options.missing_key_behavior = MissingKeyBehavior::Error;
    }
};

TEST_F(TemplateProcessorTest, ExactMatchReplacement) {
    TemplateProcessor processor(default_options);
    
    nlohmann::json template_json = R"({
        "user_id": "${/user/id}",
        "name": "${/user/name}",
        "settings": "${/preferences}"
    })"_json;
    
    auto result = processor.process(template_json, context);
    
    EXPECT_EQ(result["user_id"], 123);
    EXPECT_EQ(result["name"], "Alice");
    EXPECT_EQ(result["settings"], context["preferences"]);
}

TEST_F(TemplateProcessorTest, StringInterpolation) {
    TemplateProcessor processor(interpolation_options);
    
    nlohmann::json template_json = R"({
        "greeting": "Hello ${/user/name}!",
        "info": "User ${/user/name} has ID ${/user/id}"
    })"_json;
    
    auto result = processor.process(template_json, context);
    
    EXPECT_EQ(result["greeting"], "Hello Alice!");
    EXPECT_EQ(result["info"], "User Alice has ID 123");
}

TEST_F(TemplateProcessorTest, NestedObjects) {
    TemplateProcessor processor(default_options);
    
    nlohmann::json template_json = R"({
        "level1": {
            "level2": {
                "user_name": "${/user/name}",
                "user_id": "${/user/id}"
            }
        }
    })"_json;
    
    auto result = processor.process(template_json, context);
    
    EXPECT_EQ(result["level1"]["level2"]["user_name"], "Alice");
    EXPECT_EQ(result["level1"]["level2"]["user_id"], 123);
}

TEST_F(TemplateProcessorTest, Arrays) {
    TemplateProcessor processor(default_options);
    
    nlohmann::json template_json = R"([
        "${/user/name}",
        "${/user/id}",
        "literal_string"
    ])"_json;
    
    auto result = processor.process(template_json, context);
    
    ASSERT_TRUE(result.is_array());
    ASSERT_EQ(result.size(), 3);
    EXPECT_EQ(result[0], "Alice");
    EXPECT_EQ(result[1], 123);
    EXPECT_EQ(result[2], "literal_string");
}

TEST_F(TemplateProcessorTest, MissingKeyIgnore) {
    TemplateProcessor processor(default_options);
    
    nlohmann::json template_json = R"({
        "existing": "${/user/name}",
        "missing": "${/user/missing_field}"
    })"_json;
    
    auto result = processor.process(template_json, context);
    
    EXPECT_EQ(result["existing"], "Alice");
    EXPECT_EQ(result["missing"], "${/user/missing_field}");
}

TEST_F(TemplateProcessorTest, MissingKeyError) {
    TemplateProcessor processor(error_options);
    
    nlohmann::json template_json = R"({
        "missing": "${/user/missing_field}"
    })"_json;
    
    EXPECT_THROW(processor.process(template_json, context), MissingKeyException);
}

TEST_F(TemplateProcessorTest, RecursionLimit) {
    Options limited_options;
    limited_options.max_recursion_depth = 2;
    TemplateProcessor processor(limited_options);
    
    nlohmann::json deep_template = R"({
        "level1": {
            "level2": {
                "level3": {
                    "value": "${/user/name}"
                }
            }
        }
    })"_json;
    
    EXPECT_THROW(processor.process(deep_template, context), RecursionLimitException);
}

TEST_F(TemplateProcessorTest, CustomDelimiters) {
    Options custom_options;
    custom_options.start_marker = "<<";
    custom_options.end_marker = ">>";
    TemplateProcessor processor(custom_options);
    
    nlohmann::json template_json = R"({
        "name": "<</user/name>>"
    })"_json;
    
    auto result = processor.process(template_json, context);
    EXPECT_EQ(result["name"], "Alice");
}

TEST_F(TemplateProcessorTest, TypePreservation) {
    TemplateProcessor processor(default_options);
    
    nlohmann::json template_json = R"({
        "string": "${/user/name}",
        "number": "${/user/id}",
        "boolean": "${/preferences/notifications}",
        "object": "${/preferences}",
        "literal": 42
    })"_json;
    
    auto result = processor.process(template_json, context);
    
    EXPECT_EQ(result["string"], "Alice");
    EXPECT_EQ(result["number"], 123);
    EXPECT_EQ(result["boolean"], true);
    EXPECT_EQ(result["object"], context["preferences"]);
    EXPECT_EQ(result["literal"], 42);
}