#include <iostream>
#include <nlohmann/json.hpp>
#include "permuto/permuto.hpp"

using json = nlohmann::json;
using namespace permuto;

int main() {
    try {
        std::cout << "=== Multi-Stage Template Processing Example ===\n\n";
        
        // =================================================================
        // STAGE 1: Model Name Resolution Template
        // This first template resolves the model name from a context
        // Using "error" mode ensures the operation fails if required data is missing
        // =================================================================
        
        std::cout << "Stage 1: Model Name Resolution\n";
        std::cout << "------------------------------\n";
        
        // Template for resolving model name - simple key-value transformation
        json model_template = json::parse(R"({
            "model_name": "${/the_model}"
        })");
        
        // Context containing the actual model identifier
        json model_context = json::parse(R"({
            "the_model": "claude-3-sonnet-20240229"
        })");
        
        std::cout << "Model template: " << model_template.dump(2) << "\n";
        std::cout << "Model context: " << model_context.dump(2) << "\n";
        
        // Apply template with ERROR mode - this ensures we fail fast if required data is missing
        // The "error" mode will throw an exception if any placeholder cannot be resolved
        permuto::Options error_options;
        error_options.missing_key_behavior = permuto::MissingKeyBehavior::Error;
        error_options.start_marker = "${";
        error_options.end_marker = "}";
        
        json resolved_model = apply(model_template, model_context, error_options);
        std::cout << "Resolved model: " << resolved_model.dump(2) << "\n\n";
        
        // =================================================================
        // STAGE 2: Main API Template Setup
        // This template represents a realistic OpenAI-style API call structure
        // Using #{} delimiters to distinguish from the first stage
        // =================================================================
        
        std::cout << "Stage 2: Main API Template Setup\n";
        std::cout << "--------------------------------\n";
        
        // Main template for OpenAI-style API call with multiple placeholder types
        json main_template = json::parse(R"({
            "model": "#{/model_name}",
            "messages": [
                {
                    "role": "user", 
                    "content": "#{/prompt}"
                }
            ],
            "temperature": "#{/temp}",
            "max_tokens": 1000,
            "stream": false
        })");
        
        std::cout << "Main template: " << main_template.dump(2) << "\n\n";
        
        // =================================================================
        // STAGE 3: Progressive Template Population
        // We'll apply different contexts with different modes to build up the final result
        // =================================================================
        
        std::cout << "Stage 3: Progressive Template Population\n";
        std::cout << "---------------------------------------\n";
        
        // Step 3a: Apply model information using IGNORE mode
        // IGNORE mode populates available fields and leaves missing ones unchanged
        std::cout << "Step 3a: Applying model information (IGNORE mode)\n";
        
        permuto::Options ignore_options;
        ignore_options.missing_key_behavior = permuto::MissingKeyBehavior::Ignore;
        ignore_options.start_marker = "#{";
        ignore_options.end_marker = "}";
        
        // Use the resolved model as context for the main template
        json step1_result = apply(main_template, resolved_model, ignore_options);
        std::cout << "After model application: " << step1_result.dump(2) << "\n\n";
        
        // Step 3b: Apply prompt information using IGNORE mode
        // This will populate the prompt field while leaving temperature unchanged
        std::cout << "Step 3b: Applying prompt information (IGNORE mode)\n";
        
        json prompt_context = json::parse(R"({
            "prompt": "What are the key benefits of declarative programming?"
        })");
        
        std::cout << "Prompt context: " << prompt_context.dump(2) << "\n";
        
        json step2_result = apply(step1_result, prompt_context, ignore_options);
        std::cout << "After prompt application: " << step2_result.dump(2) << "\n\n";
        
        // Step 3c: Apply empty context using REMOVE mode
        // REMOVE mode will eliminate any remaining placeholders that cannot be resolved
        // Since we have no temperature value, the temperature field will be removed entirely
        std::cout << "Step 3c: Removing unresolved placeholders (REMOVE mode)\n";
        
        permuto::Options remove_options;
        remove_options.missing_key_behavior = permuto::MissingKeyBehavior::Remove;
        remove_options.start_marker = "#{";
        remove_options.end_marker = "}";
        
        // Empty context - no temperature value provided
        json empty_context = json::object();
        std::cout << "Empty context: " << empty_context.dump(2) << "\n";
        
        json final_result = apply(step2_result, empty_context, remove_options);
        std::cout << "Final result: " << final_result.dump(2) << "\n\n";
        
        // =================================================================
        // SUMMARY
        // =================================================================
        
        std::cout << "=== Processing Summary ===\n";
        std::cout << "1. ERROR behavior: Ensured required model data was available\n";
        std::cout << "2. IGNORE behavior (step 1): Populated model field, ignored missing fields\n";
        std::cout << "3. IGNORE behavior (step 2): Populated prompt field, ignored missing fields\n";
        std::cout << "4. REMOVE behavior: Eliminated temperature field due to no value\n";
        std::cout << "\nFinal API call is ready for submission!\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}