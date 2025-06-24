#include "template_processor.hpp"
#include <sstream>

namespace permuto {
    TemplateProcessor::TemplateProcessor(const Options& options) 
        : options_(options), parser_(options.start_marker, options.end_marker), 
          current_depth_(0) {
        options_.validate();
    }
    
    nlohmann::json TemplateProcessor::process(const nlohmann::json& template_json, 
                                            const nlohmann::json& context) const {
        // Reset state for new processing
        cycle_detector_.clear();
        current_depth_ = 0;
        return process_value(template_json, context);
    }
    
    nlohmann::json TemplateProcessor::process_value(const nlohmann::json& value, 
                                                   const nlohmann::json& context) const {
        enter_recursion();
        
        try {
            if (value.is_string()) {
                auto result = process_string(value.get<std::string>(), context);
                exit_recursion();
                return result;
            } else if (value.is_object()) {
                auto result = process_object(value, context);
                exit_recursion();
                return result;
            } else if (value.is_array()) {
                auto result = process_array(value, context);
                exit_recursion();
                return result;
            } else {
                // Primitive values (numbers, booleans, null) are returned as-is
                exit_recursion();
                return value;
            }
        } catch (...) {
            exit_recursion();
            throw;
        }
    }
    
    nlohmann::json TemplateProcessor::process_string(const std::string& str, 
                                                    const nlohmann::json& context) const {
        // Check for exact-match placeholder first
        auto exact_path = parser_.extract_exact_placeholder(str);
        if (exact_path) {
            auto resolved = resolve_path(*exact_path, context);
            if (resolved) {
                return *resolved;
            } else {
                // Handle missing key based on options
                if (options_.missing_key_behavior == MissingKeyBehavior::Error) {
                    throw MissingKeyException("Missing key in context", *exact_path);
                } else {
                    // Return original string
                    return str;
                }
            }
        }
        
        // Handle interpolation mode
        if (!options_.enable_interpolation) {
            return str;
        }
        
        // Process placeholders within the string
        std::string result = parser_.replace_placeholders(str, 
            [this, &context](const std::string& path) -> std::string {
                auto resolved = resolve_path(path, context);
                if (resolved) {
                    return json_to_string(*resolved);
                } else {
                    if (options_.missing_key_behavior == MissingKeyBehavior::Error) {
                        throw MissingKeyException("Missing key in context", path);
                    } else {
                        // Return the original placeholder
                        return options_.start_marker + path + options_.end_marker;
                    }
                }
            });
        
        return result;
    }
    
    nlohmann::json TemplateProcessor::process_object(const nlohmann::json& obj, 
                                                    const nlohmann::json& context) const {
        nlohmann::json result = nlohmann::json::object();
        
        for (auto it = obj.begin(); it != obj.end(); ++it) {
            result[it.key()] = process_value(it.value(), context);
        }
        
        return result;
    }
    
    nlohmann::json TemplateProcessor::process_array(const nlohmann::json& arr, 
                                                   const nlohmann::json& context) const {
        nlohmann::json result = nlohmann::json::array();
        
        for (const auto& item : arr) {
            result.push_back(process_value(item, context));
        }
        
        return result;
    }
    
    std::optional<nlohmann::json> TemplateProcessor::resolve_path(const std::string& path, 
                                                                 const nlohmann::json& context) const {
        // Check for cycles
        if (cycle_detector_.would_create_cycle(path)) {
            auto cycle_path = cycle_detector_.get_current_path();
            cycle_path.push_back(path);
            throw CycleException("Cycle detected in template processing", cycle_path);
        }
        
        // Add path to cycle detector
        cycle_detector_.push_path(path);
        
        try {
            JsonPointer pointer(path);
            auto result = pointer.resolve(context);
            cycle_detector_.pop_path();
            return result;
        } catch (const std::exception&) {
            cycle_detector_.pop_path();
            return std::nullopt;
        }
    }
    
    std::string TemplateProcessor::json_to_string(const nlohmann::json& value) const {
        if (value.is_string()) {
            return value.get<std::string>();
        } else if (value.is_number_integer()) {
            return std::to_string(value.get<int64_t>());
        } else if (value.is_number_unsigned()) {
            return std::to_string(value.get<uint64_t>());
        } else if (value.is_number_float()) {
            return std::to_string(value.get<double>());
        } else if (value.is_boolean()) {
            return value.get<bool>() ? "true" : "false";
        } else if (value.is_null()) {
            return "null";
        } else {
            // For objects and arrays, serialize to JSON string
            return value.dump();
        }
    }
    
    void TemplateProcessor::check_recursion_limit() const {
        if (current_depth_ >= options_.max_recursion_depth) {
            throw RecursionLimitException("Maximum recursion depth exceeded", current_depth_);
        }
    }
    
    void TemplateProcessor::enter_recursion() const {
        check_recursion_limit();
        ++current_depth_;
    }
    
    void TemplateProcessor::exit_recursion() const {
        if (current_depth_ > 0) {
            --current_depth_;
        }
    }
}