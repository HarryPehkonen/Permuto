#include "../include/permuto/permuto.hpp"
#include "template_processor.hpp"
#include "reverse_processor.hpp"
#include "placeholder_parser.hpp"

namespace permuto {
    nlohmann::json apply(const nlohmann::json& template_json,
                        const nlohmann::json& context,
                        const Options& options) {
        // Validate root-level Remove mode
        if (options.missing_key_behavior == MissingKeyBehavior::Remove && 
            template_json.is_string()) {
            // Check if the entire template is a single placeholder
            PlaceholderParser parser(options.start_marker, options.end_marker);
            auto placeholder_path = parser.extract_exact_placeholder(template_json.get<std::string>());
            if (placeholder_path) {
                throw std::invalid_argument("Remove mode cannot be used with root-level placeholders");
            }
        }
        
        TemplateProcessor processor(options);
        return processor.process(template_json, context);
    }
    
    nlohmann::json create_reverse_template(const nlohmann::json& template_json,
                                          const Options& options) {
        ReverseProcessor processor(options);
        return processor.create_reverse_template(template_json);
    }
    
    nlohmann::json apply_reverse(const nlohmann::json& reverse_template,
                                const nlohmann::json& result_json) {
        ReverseProcessor processor; // Use default options
        return processor.apply_reverse(reverse_template, result_json);
    }
}