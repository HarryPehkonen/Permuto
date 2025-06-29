#pragma once
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <optional>
#include <stdexcept>

namespace permuto {
    // Configuration
    enum class MissingKeyBehavior {
        Ignore,  // Leave placeholder as-is (default)
        Error,   // Throw exception
        Remove   // Remove the containing key/element
    };

    struct Options {
        std::string start_marker = "${";
        std::string end_marker = "}";
        bool enable_interpolation = false;
        MissingKeyBehavior missing_key_behavior = MissingKeyBehavior::Ignore;
        size_t max_recursion_depth = 64;
        
        void validate() const;  // Throws std::invalid_argument if invalid
    };

    // Exception hierarchy
    class PermutoException : public std::runtime_error {
    public:
        explicit PermutoException(const std::string& message);
    };

    class CycleException : public PermutoException {
        std::vector<std::string> cycle_path_;
    public:
        CycleException(const std::string& message, std::vector<std::string> cycle_path);
        const std::vector<std::string>& cycle_path() const;
    };

    class MissingKeyException : public PermutoException {
        std::string key_path_;
    public:
        MissingKeyException(const std::string& message, std::string key_path);
        const std::string& key_path() const;
    };

    class InvalidTemplateException : public PermutoException {
    public:
        explicit InvalidTemplateException(const std::string& message);
    };

    class RecursionLimitException : public PermutoException {
        size_t depth_;
    public:
        RecursionLimitException(const std::string& message, size_t depth);
        size_t depth() const;
    };

    // Primary API functions
    // 
    // THREAD SAFETY GUARANTEE:
    // All public API functions are thread-safe and can be called concurrently
    // from multiple threads without synchronization. Each function call operates
    // on independent data and uses thread-local storage for internal state.
    
    // Apply template substitutions to a JSON template using a context
    // Thread-safe: Can be called concurrently from multiple threads
    nlohmann::json apply(
        const nlohmann::json& template_json,
        const nlohmann::json& context,
        const Options& options = {}
    );
    
    // Create a reverse template that can reconstruct the original context
    // Thread-safe: Can be called concurrently from multiple threads
    nlohmann::json create_reverse_template(
        const nlohmann::json& template_json,
        const Options& options = {}
    );
    
    // Apply reverse template to reconstruct original context from processed result
    // Thread-safe: Can be called concurrently from multiple threads
    nlohmann::json apply_reverse(
        const nlohmann::json& reverse_template,
        const nlohmann::json& result_json
    );
}