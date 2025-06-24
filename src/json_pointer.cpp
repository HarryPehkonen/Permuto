#include "json_pointer.hpp"
#include <stdexcept>
#include <sstream>

namespace permuto {
    JsonPointer::JsonPointer(const std::string& path) : path_(path) {
        parse_path(path);
    }
    
    std::optional<nlohmann::json> JsonPointer::resolve(const nlohmann::json& context) const {
        if (is_root()) {
            return context;
        }
        
        const nlohmann::json* current = &context;
        
        for (const auto& token : tokens_) {
            if (current->is_object()) {
                auto it = current->find(token);
                if (it == current->end()) {
                    return std::nullopt;
                }
                current = &(*it);
            } else if (current->is_array()) {
                // Try to parse as array index
                try {
                    size_t index = std::stoull(token);
                    if (index >= current->size()) {
                        return std::nullopt;
                    }
                    current = &(*current)[index];
                } catch (const std::exception&) {
                    // Not a valid array index
                    return std::nullopt;
                }
            } else {
                // Can't traverse further
                return std::nullopt;
            }
        }
        
        return *current;
    }
    
    void JsonPointer::parse_path(const std::string& path) {
        if (path.empty()) {
            // Root path
            return;
        }
        
        if (path[0] != '/') {
            throw std::invalid_argument("JSON Pointer must start with '/' or be empty");
        }
        
        std::stringstream ss(path.substr(1)); // Skip leading '/'
        std::string token;
        
        while (std::getline(ss, token, '/')) {
            tokens_.push_back(unescape_token(token));
        }
    }
    
    std::string JsonPointer::unescape_token(const std::string& token) const {
        std::string result;
        result.reserve(token.size());
        
        for (size_t i = 0; i < token.size(); ++i) {
            if (token[i] == '~' && i + 1 < token.size()) {
                if (token[i + 1] == '0') {
                    result += '~';
                    ++i; // Skip next character
                } else if (token[i + 1] == '1') {
                    result += '/';
                    ++i; // Skip next character
                } else {
                    result += token[i];
                }
            } else {
                result += token[i];
            }
        }
        
        return result;
    }
}