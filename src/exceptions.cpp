#include "../include/permuto/permuto.hpp"

namespace permuto {
    namespace {
        // Constants for validation
        const size_t MIN_RECURSION_DEPTH = 0;
    }
    // Options validation
    void Options::validate() const {
        if (start_marker.empty()) {
            throw std::invalid_argument("Start marker cannot be empty");
        }
        if (end_marker.empty()) {
            throw std::invalid_argument("End marker cannot be empty");
        }
        if (start_marker == end_marker) {
            throw std::invalid_argument("Start and end markers must be different");
        }
        if (max_recursion_depth == MIN_RECURSION_DEPTH) {
            throw std::invalid_argument("Max recursion depth must be greater than 0");
        }
    }
    
    // Exception implementations
    PermutoException::PermutoException(const std::string& message) 
        : std::runtime_error(message) {}
    
    CycleException::CycleException(const std::string& message, std::vector<std::string> cycle_path)
        : PermutoException(message), cycle_path_(std::move(cycle_path)) {}
    
    const std::vector<std::string>& CycleException::cycle_path() const {
        return cycle_path_;
    }
    
    MissingKeyException::MissingKeyException(const std::string& message, std::string key_path)
        : PermutoException(message), key_path_(std::move(key_path)) {}
    
    const std::string& MissingKeyException::key_path() const {
        return key_path_;
    }
    
    InvalidTemplateException::InvalidTemplateException(const std::string& message)
        : PermutoException(message) {}
    
    RecursionLimitException::RecursionLimitException(const std::string& message, size_t depth)
        : PermutoException(message), depth_(depth) {}
    
    size_t RecursionLimitException::depth() const {
        return depth_;
    }
}