#pragma once
#include <nlohmann/json.hpp>
#include "../include/permuto/permuto.hpp"
#include "placeholder_parser.hpp"
#include <map>

namespace permuto {
    struct PathMapping {
        std::string context_path;     // Where in context this comes from
        std::string result_path;      // Where in result this goes to
    };
    
    class ReverseProcessor {
    public:
        explicit ReverseProcessor(const Options& options = {});
        
        // Create a reverse template from a forward template
        nlohmann::json create_reverse_template(const nlohmann::json& template_json) const;
        
        // Apply reverse template to extract context from result
        nlohmann::json apply_reverse(const nlohmann::json& reverse_template,
                                    const nlohmann::json& result_json) const;
        
    private:
        Options options_;
        PlaceholderParser parser_;
        
        // Analyze template to find all path mappings
        std::vector<PathMapping> analyze_template(const nlohmann::json& template_json,
                                                  const std::string& current_path = "") const;
        
        // Helper methods for analyzing different JSON types
        void analyze_object(const nlohmann::json& obj, const std::string& current_path,
                           std::vector<PathMapping>& mappings) const;
        
        void analyze_array(const nlohmann::json& arr, const std::string& current_path,
                          std::vector<PathMapping>& mappings) const;
        
        void analyze_string(const std::string& str, const std::string& current_path,
                           std::vector<PathMapping>& mappings) const;
        
        // Set value at JSON pointer path
        void set_at_path(nlohmann::json& target, const std::string& path, 
                        const nlohmann::json& value) const;
        
        // Get value at JSON pointer path
        std::optional<nlohmann::json> get_at_path(const nlohmann::json& source, 
                                                 const std::string& path) const;
        
        // Convert JSON pointer path to array of tokens
        std::vector<std::string> path_to_tokens(const std::string& path) const;
    };
}