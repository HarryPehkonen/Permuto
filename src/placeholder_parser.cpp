#include "placeholder_parser.hpp"
#include <functional>
#include <stdexcept>

namespace permuto {
    PlaceholderParser::PlaceholderParser(const std::string& start_marker, 
                                       const std::string& end_marker)
        : start_marker_(start_marker), end_marker_(end_marker) {
        if (start_marker_.empty() || end_marker_.empty()) {
            throw std::invalid_argument("Markers cannot be empty");
        }
        if (start_marker_ == end_marker_) {
            throw std::invalid_argument("Start and end markers must be different");
        }
    }
    
    std::vector<Placeholder> PlaceholderParser::find_placeholders(const std::string& text) const {
        std::vector<Placeholder> placeholders;
        
        size_t pos = 0;
        while (pos < text.length()) {
            size_t start = text.find(start_marker_, pos);
            if (start == std::string::npos) {
                break;
            }
            
            size_t path_start = start + start_marker_.length();
            size_t end = text.find(end_marker_, path_start);
            if (end == std::string::npos) {
                // No matching end marker, skip this start marker
                pos = start + 1;
                continue;
            }
            
            std::string path = text.substr(path_start, end - path_start);
            if (is_valid_path(path)) {
                Placeholder placeholder;
                placeholder.path = path;
                placeholder.start_pos = start;
                placeholder.end_pos = end + end_marker_.length();
                placeholder.is_exact_match = (start == 0 && placeholder.end_pos == text.length());
                
                placeholders.push_back(placeholder);
            }
            
            pos = end + end_marker_.length();
        }
        
        return placeholders;
    }
    
    std::optional<std::string> PlaceholderParser::extract_exact_placeholder(const std::string& text) const {
        if (text.length() < start_marker_.length() + end_marker_.length()) {
            return std::nullopt;
        }
        
        if (text.substr(0, start_marker_.length()) != start_marker_ || 
            text.substr(text.length() - end_marker_.length()) != end_marker_) {
            return std::nullopt;
        }
        
        size_t path_start = start_marker_.length();
        size_t path_length = text.length() - start_marker_.length() - end_marker_.length();
        std::string path = text.substr(path_start, path_length);
        
        if (is_valid_path(path)) {
            return path;
        }
        
        return std::nullopt;
    }
    
    std::string PlaceholderParser::replace_placeholders(const std::string& text,
        const std::function<std::string(const std::string&)>& value_provider) const {
        
        auto placeholders = find_placeholders(text);
        if (placeholders.empty()) {
            return text;
        }
        
        std::string result;
        size_t last_pos = 0;
        
        for (const auto& placeholder : placeholders) {
            // Add text before this placeholder
            result += text.substr(last_pos, placeholder.start_pos - last_pos);
            
            // Add the replacement value
            result += value_provider(placeholder.path);
            
            last_pos = placeholder.end_pos;
        }
        
        // Add remaining text
        result += text.substr(last_pos);
        
        return result;
    }
    
    bool PlaceholderParser::is_valid_path(const std::string& path) const {
        // Empty path is valid (refers to root)
        if (path.empty()) {
            return true;
        }
        
        // Must start with '/' for JSON Pointer
        return path[0] == '/';
    }
}