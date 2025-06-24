#pragma once
#include <string>
#include <vector>
#include <optional>
#include <functional>

namespace permuto {
    struct Placeholder {
        std::string path;
        size_t start_pos;
        size_t end_pos;
        bool is_exact_match; // True if the entire string is just this placeholder
    };
    
    class PlaceholderParser {
    public:
        PlaceholderParser(const std::string& start_marker = "${", 
                         const std::string& end_marker = "}");
        
        // Find all placeholders in a string
        std::vector<Placeholder> find_placeholders(const std::string& text) const;
        
        // Check if string is exactly one placeholder (for exact-match substitution)
        std::optional<std::string> extract_exact_placeholder(const std::string& text) const;
        
        // Replace placeholders in text with provided values
        std::string replace_placeholders(const std::string& text, 
            const std::function<std::string(const std::string&)>& value_provider) const;
        
    private:
        std::string start_marker_;
        std::string end_marker_;
        
        bool is_valid_path(const std::string& path) const;
    };
}