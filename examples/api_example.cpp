/**
 * @file api_example.cpp
 * @brief Complete example demonstrating Permuto API usage
 * 
 * This example shows how to use Permuto for generating API payloads
 * for different LLM providers using the same context data.
 */

#include <permuto/permuto.hpp>
#include <iostream>
#include <fstream>

void print_section(const std::string& title) {
    std::cout << "\n" << std::string(50, '=') << "\n";
    std::cout << title << "\n";
    std::cout << std::string(50, '=') << "\n";
}

int main() {
    try {
        print_section("Permuto API Example");
        
        // Context data - shared across all templates
        nlohmann::json context = R"({
            "user": {
                "id": 12345,
                "name": "Alice Johnson",
                "email": "alice@example.com",
                "role": "researcher"
            },
            "request": {
                "prompt": "Explain the concept of quantum entanglement",
                "max_tokens": 1000,
                "temperature": 0.7
            },
            "config": {
                "api_version": "v1",
                "timeout": 30,
                "retry_count": 3
            },
            "metadata": {
                "timestamp": 1703097600,
                "request_id": "req_abc123",
                "client_version": "1.2.0"
            }
        })"_json;
        
        std::cout << "Context Data:\n" << context.dump(2) << "\n";
        
        // ===== Example 1: OpenAI API Template =====
        print_section("Example 1: OpenAI API Template");
        
        nlohmann::json openai_template = R"({
            "model": "gpt-4",
            "messages": [
                {
                    "role": "system",
                    "content": "You are a helpful assistant."
                },
                {
                    "role": "user", 
                    "content": "${/request/prompt}"
                }
            ],
            "max_tokens": "${/request/max_tokens}",
            "temperature": "${/request/temperature}",
            "user": "${/user/id}"
        })"_json;
        
        auto openai_result = permuto::apply(openai_template, context);
        std::cout << "OpenAI API Payload:\n" << openai_result.dump(2) << "\n";
        
        // ===== Example 2: Anthropic API Template =====
        print_section("Example 2: Anthropic API Template");
        
        nlohmann::json anthropic_template = R"({
            "model": "claude-3-sonnet-20240229",
            "prompt": "Human: ${/request/prompt}\n\nAssistant:",
            "max_tokens_to_sample": "${/request/max_tokens}",
            "temperature": "${/request/temperature}",
            "metadata": {
                "user_id": "${/user/id}"
            }
        })"_json;
        
        auto anthropic_result = permuto::apply(anthropic_template, context);
        std::cout << "Anthropic API Payload:\n" << anthropic_result.dump(2) << "\n";
        
        // ===== Example 3: String Interpolation =====
        print_section("Example 3: String Interpolation");
        
        permuto::Options interp_options;
        interp_options.enable_interpolation = true;
        
        nlohmann::json notification_template = R"({
            "subject": "Request ${/metadata/request_id} completed",
            "body": "Hello ${/user/name},\n\nYour request '${/request/prompt}' has been processed successfully.\n\nUser ID: ${/user/id}\nTimestamp: ${/metadata/timestamp}\n\nBest regards,\nAPI Service",
            "recipient": "${/user/email}",
            "metadata": {
                "client_version": "${/metadata/client_version}",
                "user_role": "${/user/role}"
            }
        })"_json;
        
        auto notification_result = permuto::apply(notification_template, context, interp_options);
        std::cout << "Notification Template Result:\n" << notification_result.dump(2) << "\n";
        
        // ===== Example 4: Reverse Operations =====
        print_section("Example 4: Reverse Operations (Round-trip)");
        
        nlohmann::json simple_template = R"({
            "user_info": {
                "name": "${/user/name}",
                "id": "${/user/id}"
            },
            "settings": {
                "max_tokens": "${/request/max_tokens}",
                "temperature": "${/request/temperature}"
            }
        })"_json;
        
        // Forward operation
        auto forward_result = permuto::apply(simple_template, context);
        std::cout << "Forward Result:\n" << forward_result.dump(2) << "\n";
        
        // Create reverse template
        auto reverse_template = permuto::create_reverse_template(simple_template);
        std::cout << "\nReverse Template:\n" << reverse_template.dump(2) << "\n";
        
        // Apply reverse operation
        auto reconstructed = permuto::apply_reverse(reverse_template, forward_result);
        std::cout << "\nReconstructed Context:\n" << reconstructed.dump(2) << "\n";
        
        // Verify round-trip integrity
        bool round_trip_success = true;
        if (reconstructed["user"]["name"] == context["user"]["name"] &&
            reconstructed["user"]["id"] == context["user"]["id"] &&
            reconstructed["request"]["max_tokens"] == context["request"]["max_tokens"] &&
            reconstructed["request"]["temperature"] == context["request"]["temperature"]) {
            std::cout << "\n✓ Round-trip integrity verified!\n";
        } else {
            std::cout << "\n✗ Round-trip integrity failed!\n";
            round_trip_success = false;
        }
        
        // ===== Example 5: Error Handling =====
        print_section("Example 5: Error Handling");
        
        nlohmann::json error_template = R"({
            "existing_field": "${/user/name}",
            "missing_field": "${/user/nonexistent}"
        })"_json;
        
        // Default behavior: ignore missing keys
        permuto::Options ignore_options;
        auto ignore_result = permuto::apply(error_template, context, ignore_options);
        std::cout << "Missing key ignored:\n" << ignore_result.dump(2) << "\n";
        
        // Error on missing keys
        try {
            permuto::Options error_options;
            error_options.missing_key_behavior = permuto::MissingKeyBehavior::Error;
            
            auto error_result = permuto::apply(error_template, context, error_options);
            std::cout << "This shouldn't print!\n";
        } catch (const permuto::MissingKeyException& e) {
            std::cout << "\n✓ Caught expected error: " << e.what() << "\n";
            std::cout << "Missing key path: " << e.key_path() << "\n";
        }
        
        // ===== Example 6: Custom Delimiters =====
        print_section("Example 6: Custom Delimiters");
        
        permuto::Options custom_options;
        custom_options.start_marker = "{{";
        custom_options.end_marker = "}}";
        
        nlohmann::json custom_template = R"({
            "message": "Hello {{/user/name}}!",
            "user_id": "{{/user/id}}",
            "role": "{{/user/role}}"
        })"_json;
        
        auto custom_result = permuto::apply(custom_template, context, custom_options);
        std::cout << "Custom Delimiters Result:\n" << custom_result.dump(2) << "\n";
        
        print_section("Example Complete");
        std::cout << "All examples completed successfully!\n";
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}