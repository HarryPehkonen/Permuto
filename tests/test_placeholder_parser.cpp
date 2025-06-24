#include <gtest/gtest.h>
#include "../src/placeholder_parser.hpp"

using namespace permuto;

class PlaceholderParserTest : public ::testing::Test {
protected:
    PlaceholderParser parser;
    PlaceholderParser custom_parser{"<", ">"};
};

TEST_F(PlaceholderParserTest, ExactPlaceholder) {
    auto result = parser.extract_exact_placeholder("${/user/name}");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, "/user/name");
    
    auto no_result = parser.extract_exact_placeholder("Hello ${/user/name}!");
    EXPECT_FALSE(no_result.has_value());
}

TEST_F(PlaceholderParserTest, FindPlaceholders) {
    auto placeholders = parser.find_placeholders("Hello ${/user/name}! Your ID is ${/user/id}.");
    
    ASSERT_EQ(placeholders.size(), 2);
    
    EXPECT_EQ(placeholders[0].path, "/user/name");
    EXPECT_EQ(placeholders[0].start_pos, 6);
    EXPECT_EQ(placeholders[0].end_pos, 19);
    EXPECT_FALSE(placeholders[0].is_exact_match);
    
    EXPECT_EQ(placeholders[1].path, "/user/id");
    EXPECT_EQ(placeholders[1].start_pos, 32);
    EXPECT_EQ(placeholders[1].end_pos, 43);
    EXPECT_FALSE(placeholders[1].is_exact_match);
}

TEST_F(PlaceholderParserTest, ExactMatchDetection) {
    auto placeholders = parser.find_placeholders("${/user/name}");
    
    ASSERT_EQ(placeholders.size(), 1);
    EXPECT_TRUE(placeholders[0].is_exact_match);
    EXPECT_EQ(placeholders[0].path, "/user/name");
}

TEST_F(PlaceholderParserTest, CustomDelimiters) {
    auto result = custom_parser.extract_exact_placeholder("</user/name>");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, "/user/name");
    
    auto placeholders = custom_parser.find_placeholders("Hello </user/name>!");
    ASSERT_EQ(placeholders.size(), 1);
    EXPECT_EQ(placeholders[0].path, "/user/name");
}

TEST_F(PlaceholderParserTest, ReplacePlaceholders) {
    std::string text = "Hello ${/user/name}! Your ID is ${/user/id}.";
    
    auto result = parser.replace_placeholders(text, [](const std::string& path) -> std::string {
        if (path == "/user/name") return "Alice";
        if (path == "/user/id") return "123";
        return "UNKNOWN";
    });
    
    EXPECT_EQ(result, "Hello Alice! Your ID is 123.");
}

TEST_F(PlaceholderParserTest, NoPlaceholders) {
    auto placeholders = parser.find_placeholders("Just plain text");
    EXPECT_TRUE(placeholders.empty());
    
    auto result = parser.extract_exact_placeholder("Just plain text");
    EXPECT_FALSE(result.has_value());
}

TEST_F(PlaceholderParserTest, MalformedPlaceholders) {
    auto placeholders = parser.find_placeholders("${incomplete");
    EXPECT_TRUE(placeholders.empty());
    
    auto placeholders2 = parser.find_placeholders("incomplete}");
    EXPECT_TRUE(placeholders2.empty());
}

TEST_F(PlaceholderParserTest, InvalidDelimiters) {
    EXPECT_THROW(PlaceholderParser("", "}"), std::invalid_argument);
    EXPECT_THROW(PlaceholderParser("${", ""), std::invalid_argument);
    EXPECT_THROW(PlaceholderParser("same", "same"), std::invalid_argument);
}

TEST_F(PlaceholderParserTest, InvalidPaths) {
    // Invalid paths (not starting with /) should not be processed
    auto placeholders = parser.find_placeholders("${invalid_path}");
    EXPECT_TRUE(placeholders.empty());
    
    auto result = parser.extract_exact_placeholder("${invalid_path}");
    EXPECT_FALSE(result.has_value());
}