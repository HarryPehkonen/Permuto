#pragma once
#include <nlohmann/json.hpp>
#include "../include/permuto/permuto.hpp"
#include "json_pointer.hpp"
#include "placeholder_parser.hpp"
#include "cycle_detector.hpp"

namespace permuto {
    class TemplateProcessor {
    public:
        explicit TemplateProcessor(const Options& options = {});
        
        // Process a template with the given context
        nlohmann::json process(const nlohmann::json& template_json, 
                              const nlohmann::json& context) const;
        
    private:
        Options options_;
        PlaceholderParser parser_;
        mutable CycleDetector cycle_detector_;
        mutable size_t current_depth_;
        
        // Process different JSON value types
        nlohmann::json process_value(const nlohmann::json& value, 
                                    const nlohmann::json& context) const;
        
        nlohmann::json process_string(const std::string& str, 
                                     const nlohmann::json& context) const;
        
        nlohmann::json process_object(const nlohmann::json& obj, 
                                     const nlohmann::json& context) const;
        
        nlohmann::json process_array(const nlohmann::json& arr, 
                                    const nlohmann::json& context) const;
        
        // Resolve a path in the context with safety checks
        std::optional<nlohmann::json> resolve_path(const std::string& path, 
                                                  const nlohmann::json& context) const;
        
        // Convert JSON value to string for interpolation
        std::string json_to_string(const nlohmann::json& value) const;
        
        // Safety checks
        void check_recursion_limit() const;
        void enter_recursion() const;
        void exit_recursion() const;
    };
}