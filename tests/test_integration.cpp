#include <gtest/gtest.h>
#include <permuto/permuto.hpp>
#include <chrono>

using namespace permuto;

class IntegrationTest : public ::testing::Test {
protected:
    nlohmann::json api_template = R"({
        "model": "${/config/model}",
        "messages": [
            {
                "role": "user",
                "content": "${/user_input}"
            }
        ],
        "max_tokens": "${/config/max_tokens}",
        "temperature": "${/config/temperature}"
    })"_json;
    
    nlohmann::json context = R"({
        "config": {
            "model": "gpt-4",
            "max_tokens": 1000,
            "temperature": 0.7
        },
        "user_input": "Hello, world!"
    })"_json;
};

TEST_F(IntegrationTest, FullAPIWorkflow) {
    // Test all main API functions
    Options opts;
    
    // Forward processing
    auto result = permuto::apply(api_template, context, opts);
    
    EXPECT_EQ(result["model"], "gpt-4");
    EXPECT_EQ(result["messages"][0]["content"], "Hello, world!");
    EXPECT_EQ(result["max_tokens"], 1000);
    EXPECT_EQ(result["temperature"], 0.7);
    
    // Reverse processing
    auto reverse_template = permuto::create_reverse_template(api_template, opts);
    auto reconstructed = permuto::apply_reverse(reverse_template, result);
    
    EXPECT_EQ(reconstructed, context);
}

TEST_F(IntegrationTest, ComplexNestedStructure) {
    nlohmann::json complex_template = R"({
        "api": {
            "version": "v1",
            "endpoint": "${/config/endpoint}",
            "auth": {
                "type": "bearer",
                "token": "${/credentials/token}"
            }
        },
        "request": {
            "method": "POST",
            "headers": {
                "Content-Type": "application/json",
                "User-Agent": "${/client/user_agent}"
            },
            "body": {
                "query": "${/user/query}",
                "options": "${/user/options}"
            }
        },
        "metadata": {
            "timestamp": "${/request/timestamp}",
            "request_id": "${/request/id}"
        }
    })"_json;
    
    nlohmann::json complex_context = R"({
        "config": {
            "endpoint": "https://api.example.com"
        },
        "credentials": {
            "token": "secret-token"
        },
        "client": {
            "user_agent": "MyApp/1.0"
        },
        "user": {
            "query": "search term",
            "options": {
                "limit": 10,
                "sort": "relevance"
            }
        },
        "request": {
            "timestamp": 1234567890,
            "id": "req-123"
        }
    })"_json;
    
    auto result = permuto::apply(complex_template, complex_context);
    
    // Verify nested structure is properly processed
    EXPECT_EQ(result["api"]["endpoint"], "https://api.example.com");
    EXPECT_EQ(result["api"]["auth"]["token"], "secret-token");
    EXPECT_EQ(result["request"]["headers"]["User-Agent"], "MyApp/1.0");
    EXPECT_EQ(result["request"]["body"]["query"], "search term");
    EXPECT_EQ(result["metadata"]["timestamp"], 1234567890);
    
    // Test round-trip
    auto reverse_template = permuto::create_reverse_template(complex_template);
    auto reconstructed = permuto::apply_reverse(reverse_template, result);
    EXPECT_EQ(reconstructed, complex_context);
}

TEST_F(IntegrationTest, StringInterpolationIntegration) {
    Options interp_opts;
    interp_opts.enable_interpolation = true;
    
    nlohmann::json greeting_template = R"({
        "greeting": "Hello ${/user/name}!",
        "message": "Welcome to ${/app/name}, version ${/app/version}.",
        "info": "You have ${/user/notifications} new notifications."
    })"_json;
    
    nlohmann::json greeting_context = R"({
        "user": {
            "name": "Alice",
            "notifications": 3
        },
        "app": {
            "name": "MyApp",
            "version": "2.1.0"
        }
    })"_json;
    
    auto result = permuto::apply(greeting_template, greeting_context, interp_opts);
    
    EXPECT_EQ(result["greeting"], "Hello Alice!");
    EXPECT_EQ(result["message"], "Welcome to MyApp, version 2.1.0.");
    EXPECT_EQ(result["info"], "You have 3 new notifications.");
}

TEST_F(IntegrationTest, ErrorHandlingIntegration) {
    Options error_opts;
    error_opts.missing_key_behavior = MissingKeyBehavior::Error;
    
    nlohmann::json incomplete_context = R"({
        "config": {
            "model": "gpt-4"
        }
    })"_json;
    
    EXPECT_THROW(permuto::apply(api_template, incomplete_context, error_opts), MissingKeyException);
    
    try {
        permuto::apply(api_template, incomplete_context, error_opts);
    } catch (const MissingKeyException& e) {
        std::string key_path = e.key_path();
        // Should be one of the missing keys
        EXPECT_TRUE(key_path == "/config/max_tokens" || 
                   key_path == "/config/temperature" || 
                   key_path == "/user_input");
    }
}

TEST_F(IntegrationTest, CustomDelimitersIntegration) {
    Options custom_opts;
    custom_opts.start_marker = "{{";
    custom_opts.end_marker = "}}";
    
    nlohmann::json custom_template = R"({
        "name": "{{/user/name}}",
        "id": "{{/user/id}}"
    })"_json;
    
    nlohmann::json simple_context = R"({
        "user": {
            "name": "Bob",
            "id": 456
        }
    })"_json;
    
    auto result = permuto::apply(custom_template, simple_context, custom_opts);
    
    EXPECT_EQ(result["name"], "Bob");
    EXPECT_EQ(result["id"], 456);
    
    // Test round-trip with custom delimiters
    auto reverse_template = permuto::create_reverse_template(custom_template, custom_opts);
    auto reconstructed = permuto::apply_reverse(reverse_template, result);
    EXPECT_EQ(reconstructed, simple_context);
}

TEST_F(IntegrationTest, PerformanceBaseline) {
    // Create a template with many placeholders
    nlohmann::json large_template = nlohmann::json::object();
    nlohmann::json large_context = nlohmann::json::object();
    
    for (int i = 0; i < 100; ++i) {
        std::string key = "field_" + std::to_string(i);
        std::string path = "/data/" + key;
        large_template[key] = "${" + path + "}";
        large_context["data"][key] = "value_" + std::to_string(i);
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    auto result = permuto::apply(large_template, large_context);
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Should process 100 placeholders in reasonable time (< 100ms)
    EXPECT_LT(duration.count(), 100);
    
    // Verify correctness
    EXPECT_EQ(result["field_0"], "value_0");
    EXPECT_EQ(result["field_99"], "value_99");
}