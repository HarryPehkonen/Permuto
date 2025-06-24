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

TEST_F(IntegrationTest, LLMAPIWithRemoveMode) {
    // Real-world LLM API scenario with optional parameters
    nlohmann::json llm_template = R"({
        "model": "${/config/model}",
        "messages": [
            {
                "role": "user", 
                "content": "${/user_input}"
            }
        ],
        "temperature": "${/config/temperature}",
        "max_tokens": "${/config/max_tokens}",
        "top_p": "${/config/top_p}",
        "frequency_penalty": "${/config/frequency_penalty}",
        "presence_penalty": "${/config/presence_penalty}",
        "stop": "${/config/stop_sequences}"
    })"_json;
    
    // Context with only some parameters specified
    nlohmann::json partial_context = R"({
        "config": {
            "model": "gpt-4",
            "temperature": 0.7
        },
        "user_input": "Explain quantum computing"
    })"_json;
    
    Options remove_opts;
    remove_opts.missing_key_behavior = MissingKeyBehavior::Remove;
    
    auto result = permuto::apply(llm_template, partial_context, remove_opts);
    
    // Should contain only specified parameters
    EXPECT_EQ(result["model"], "gpt-4");
    EXPECT_EQ(result["temperature"], 0.7);
    EXPECT_EQ(result["messages"][0]["content"], "Explain quantum computing");
    
    // Should NOT contain unspecified optional parameters
    EXPECT_FALSE(result.contains("max_tokens"));
    EXPECT_FALSE(result.contains("top_p"));
    EXPECT_FALSE(result.contains("frequency_penalty"));
    EXPECT_FALSE(result.contains("presence_penalty"));
    EXPECT_FALSE(result.contains("stop"));
    
    // Verify this produces clean API call JSON
    std::string api_call = result.dump();
    EXPECT_TRUE(api_call.find("null") == std::string::npos);  // No null values
    EXPECT_GT(api_call.length(), 50);  // Reasonable size
}

TEST_F(IntegrationTest, MixedModeWorkflow) {
    // Workflow combining Remove and Error modes with different markers
    nlohmann::json workflow_template = R"({
        "api_request": {
            "required_field": "<</user/id>>",
            "optional_field": "${/config/debug_mode}"
        },
        "processing_pipeline": [
            "validate_input",
            "${/middleware/auth}",
            "${/middleware/rate_limit}",
            "process_request"
        ]
    })"_json;
    
    nlohmann::json workflow_context = R"({
        "user": {
            "id": 12345
        },
        "middleware": {
            "auth": "jwt_middleware"
        }
    })"_json;
    
    // Step 1: Process optional fields with Remove mode
    Options remove_opts;
    remove_opts.missing_key_behavior = MissingKeyBehavior::Remove;
    remove_opts.start_marker = "${";
    remove_opts.end_marker = "}";
    
    auto step1_result = permuto::apply(workflow_template, workflow_context, remove_opts);
    
    // Optional fields should be removed
    EXPECT_FALSE(step1_result["api_request"].contains("optional_field"));
    EXPECT_EQ(step1_result["processing_pipeline"].size(), 3);  // rate_limit removed
    EXPECT_EQ(step1_result["processing_pipeline"][1], "jwt_middleware");
    
    // Step 2: Validate required fields with Error mode
    Options error_opts;
    error_opts.missing_key_behavior = MissingKeyBehavior::Error;
    error_opts.start_marker = "<<";
    error_opts.end_marker = ">>";
    
    auto final_result = permuto::apply(step1_result, workflow_context, error_opts);
    
    // Required fields should be processed successfully
    EXPECT_EQ(final_result["api_request"]["required_field"], 12345);
    
    // Test error case: missing required field
    nlohmann::json incomplete_context = R"({
        "middleware": {
            "auth": "jwt_middleware"
        }
    })"_json;
    
    auto step1_incomplete = permuto::apply(workflow_template, incomplete_context, remove_opts);
    EXPECT_THROW(permuto::apply(step1_incomplete, incomplete_context, error_opts), MissingKeyException);
}