#pragma once
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <optional>

namespace permuto {
    class JsonPointer {
    public:
        explicit JsonPointer(const std::string& path);
        
        // Resolve path in context, returns nullopt if path doesn't exist
        std::optional<nlohmann::json> resolve(const nlohmann::json& context) const;
        
        // Get the path tokens
        const std::vector<std::string>& tokens() const { return tokens_; }
        
        // Get the raw path string
        const std::string& path() const { return path_; }
        
        // Check if this is a root path (empty)
        bool is_root() const { return tokens_.empty(); }
        
    private:
        std::string path_;
        std::vector<std::string> tokens_;
        
        void parse_path(const std::string& path);
        std::string unescape_token(const std::string& token) const;
    };
}