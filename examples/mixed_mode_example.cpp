#include <iostream>
#include <nlohmann/json.hpp>
#include "permuto/permuto.hpp"

using json = nlohmann::json;

int main() {
    std::cout << "=== Mixed Placeholder Mode Example ===" << std::endl;
    std::cout << "Demonstrating different behaviors with different placeholder markers\n" << std::endl;
    
    // Context with some missing values
    json context = R"({
        "user": {
            "name": "Alice",
            "id": 123
        },
        "coordinates": {
            "x": 10.5,
            "y": 20.3
        },
        "config": {
            "temperature": 0.7
        }
    })"_json;
    
    std::cout << "Context:" << std::endl;
    std::cout << context.dump(2) << "\n" << std::endl;
    
    // Template using two different placeholder styles:
    // ${} with Remove mode for optional fields
    // <<>> with Error mode for required coordinates
    json mixed_template = R"({
        "llm_api_request": {
            "model": "gpt-4",
            "temperature": "${/config/temperature}",
            "max_tokens": "${/config/max_tokens}",
            "top_p": "${/config/top_p}",
            "user_id": "${/user/id}"
        },
        "coordinates": ["<</coordinates/x>>", "<</coordinates/y>>", "<</coordinates/z>>"],
        "optional_middleware": [
            "auth",
            "${/middleware/rate_limiter}",
            "${/middleware/analytics}",
            "cors"
        ],
        "user_info": {
            "name": "${/user/name}",
            "email": "${/user/email}",
            "preferences": "${/user/preferences}"
        }
    })"_json;
    
    std::cout << "Template (using ${} for optional, <<>> for required):" << std::endl;
    std::cout << mixed_template.dump(2) << "\n" << std::endl;
    
    try {
        // Process 1: ${} markers with Remove mode (optional fields)
        std::cout << "=== Processing with ${} markers (Remove mode) ===" << std::endl;
        permuto::Options remove_options;
        remove_options.missing_key_behavior = permuto::MissingKeyBehavior::Remove;
        remove_options.start_marker = "${";
        remove_options.end_marker = "}";
        
        auto result_remove = permuto::apply(mixed_template, context, remove_options);
        std::cout << "Result (missing keys removed):" << std::endl;
        std::cout << result_remove.dump(2) << "\n" << std::endl;
        
        // Process 2: <<>> markers with Error mode (required fields)
        std::cout << "=== Processing with <<>> markers (Error mode) ===" << std::endl;
        permuto::Options error_options;
        error_options.missing_key_behavior = permuto::MissingKeyBehavior::Error;
        error_options.start_marker = "<<";
        error_options.end_marker = ">>";
        
        try {
            auto result_error = permuto::apply(result_remove, context, error_options);
            std::cout << "This should not be reached - missing /coordinates/z should cause error" << std::endl;
        } catch (const permuto::MissingKeyException& e) {
            std::cout << "Expected error caught: " << e.what() << std::endl;
            std::cout << "Missing key path: " << e.key_path() << "\n" << std::endl;
        }
        
        // Process 3: Demonstrate successful case with complete coordinates
        std::cout << "=== Successful case with complete coordinates ===" << std::endl;
        
        // Add missing coordinate to context
        json complete_context = context;
        complete_context["coordinates"]["z"] = 30.7;
        
        std::cout << "Complete context:" << std::endl;
        std::cout << complete_context.dump(2) << "\n" << std::endl;
        
        // First pass: Remove mode for optional fields
        auto step1_result = permuto::apply(mixed_template, complete_context, remove_options);
        
        // Second pass: Error mode for required fields (should succeed now)
        auto final_result = permuto::apply(step1_result, complete_context, error_options);
        
        std::cout << "Final result (all processing successful):" << std::endl;
        std::cout << final_result.dump(2) << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "\n=== Summary ===" << std::endl;
    std::cout << "- ${} markers with Remove mode: Optional fields get removed if missing" << std::endl;
    std::cout << "- <<>> markers with Error mode: Required fields throw exception if missing" << std::endl;
    std::cout << "- Different marker types allow fine-grained control over behavior" << std::endl;
    std::cout << "- Process order matters: Remove mode first, then Error mode validation" << std::endl;
    
    return 0;
}