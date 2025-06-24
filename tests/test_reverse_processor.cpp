#include <gtest/gtest.h>
#include "../src/reverse_processor.hpp"
#include "../src/template_processor.hpp"
#include <stdexcept>

using namespace permuto;

class ReverseProcessorTest : public ::testing::Test {
protected:
    Options default_options; // interpolation disabled by default
    
    nlohmann::json template_json = R"({
        "user_id": "${/user/id}",
        "name": "${/user/name}",
        "email": "${/user/email}",
        "settings": "${/preferences}"
    })"_json;
    
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
    
    nlohmann::json result = R"({
        "user_id": 123,
        "name": "Alice",
        "email": "alice@example.com",
        "settings": {
            "theme": "dark",
            "notifications": true
        }
    })"_json;
};

TEST_F(ReverseProcessorTest, CreateReverseTemplate) {
    ReverseProcessor processor(default_options);
    
    auto reverse_template = processor.create_reverse_template(template_json);
    
    EXPECT_EQ(reverse_template["/user_id"], "/user/id");
    EXPECT_EQ(reverse_template["/name"], "/user/name");
    EXPECT_EQ(reverse_template["/email"], "/user/email");
    EXPECT_EQ(reverse_template["/settings"], "/preferences");
}

TEST_F(ReverseProcessorTest, ApplyReverse) {
    ReverseProcessor processor(default_options);
    
    auto reverse_template = processor.create_reverse_template(template_json);
    auto reconstructed = processor.apply_reverse(reverse_template, result);
    
    EXPECT_EQ(reconstructed, context);
}

TEST_F(ReverseProcessorTest, RoundTripIntegrity) {
    ReverseProcessor processor(default_options);
    
    // Forward: template + context -> result
    TemplateProcessor forward_processor(default_options);
    auto forward_result = forward_processor.process(template_json, context);
    
    // Reverse: template + result -> context
    auto reverse_template = processor.create_reverse_template(template_json);
    auto reconstructed = processor.apply_reverse(reverse_template, forward_result);
    
    EXPECT_EQ(reconstructed, context);
}

TEST_F(ReverseProcessorTest, NestedTemplate) {
    nlohmann::json nested_template = R"({
        "user": {
            "profile": {
                "name": "${/user/name}",
                "id": "${/user/id}"
            }
        },
        "config": "${/preferences}"
    })"_json;
    
    ReverseProcessor processor(default_options);
    auto reverse_template = processor.create_reverse_template(nested_template);
    
    EXPECT_EQ(reverse_template["/user/profile/name"], "/user/name");
    EXPECT_EQ(reverse_template["/user/profile/id"], "/user/id");
    EXPECT_EQ(reverse_template["/config"], "/preferences");
}

TEST_F(ReverseProcessorTest, ArrayTemplate) {
    nlohmann::json array_template = R"([
        "${/user/name}",
        "${/user/id}",
        {"email": "${/user/email}"}
    ])"_json;
    
    ReverseProcessor processor(default_options);
    auto reverse_template = processor.create_reverse_template(array_template);
    
    EXPECT_EQ(reverse_template["/0"], "/user/name");
    EXPECT_EQ(reverse_template["/1"], "/user/id");
    EXPECT_EQ(reverse_template["/2/email"], "/user/email");
}

TEST_F(ReverseProcessorTest, InterpolationDisallowed) {
    Options interpolation_opts;
    interpolation_opts.enable_interpolation = true;
    
    try {
        ReverseProcessor processor(interpolation_opts);
        FAIL() << "Expected std::invalid_argument but no exception was thrown";
    } catch (const std::invalid_argument& e) {
        SUCCEED();
    } catch (const std::exception& e) {
        FAIL() << "Expected std::invalid_argument but got: " << e.what();
    }
}

TEST_F(ReverseProcessorTest, NonPlaceholderElements) {
    nlohmann::json mixed_template = R"({
        "placeholder": "${/user/name}",
        "literal": "constant_value",
        "number": 42,
        "boolean": true,
        "null_value": null
    })"_json;
    
    ReverseProcessor processor(default_options);
    auto reverse_template = processor.create_reverse_template(mixed_template);
    
    // Only placeholder should be in reverse template
    EXPECT_EQ(reverse_template.size(), 1);
    EXPECT_EQ(reverse_template["/placeholder"], "/user/name");
}

TEST_F(ReverseProcessorTest, EmptyTemplate) {
    nlohmann::json empty_template = nlohmann::json::object();
    
    ReverseProcessor processor(default_options);
    auto reverse_template = processor.create_reverse_template(empty_template);
    
    EXPECT_TRUE(reverse_template.empty());
}

TEST_F(ReverseProcessorTest, MissingDataInResult) {
    nlohmann::json incomplete_result = R"({
        "user_id": 123,
        "name": "Alice"
    })"_json;
    
    ReverseProcessor processor(default_options);
    auto reverse_template = processor.create_reverse_template(template_json);
    auto reconstructed = processor.apply_reverse(reverse_template, incomplete_result);
    
    // Should only reconstruct available data
    EXPECT_EQ(reconstructed["user"]["id"], 123);
    EXPECT_EQ(reconstructed["user"]["name"], "Alice");
    EXPECT_FALSE(reconstructed["user"].contains("email"));
    EXPECT_FALSE(reconstructed.contains("preferences"));
}