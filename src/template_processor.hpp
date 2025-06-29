#pragma once
#include <nlohmann/json.hpp>
#include <thread>
#include <unordered_map>
#include <mutex>
#include "../include/permuto/permuto.hpp"
#include "json_pointer.hpp"
#include "placeholder_parser.hpp"
#include "cycle_detector.hpp"

namespace permuto {
    // Thread-safe context for processing state
    // Each thread gets its own independent processing context via thread_local storage
    struct ProcessingContext {
        CycleDetector cycle_detector;
        size_t current_depth = 0;
    };
    
    // Thread-safe template processor
    // 
    // THREAD SAFETY:
    // - Multiple TemplateProcessor instances can be used concurrently
    // - A single TemplateProcessor instance can be used concurrently from multiple threads
    // - All mutable state is stored in thread-local storage (ProcessingContext)
    // - The processor itself contains only immutable data after construction
    class TemplateProcessor {
    public:
        explicit TemplateProcessor(const Options& options = {});
        
        // Process a template with the given context (thread-safe)
        // Can be called concurrently from multiple threads safely
        nlohmann::json process(const nlohmann::json& template_json, 
                              const nlohmann::json& context) const;
        
    private:
        const Options options_;
        const PlaceholderParser parser_;
        
        // Get or create processing context for current thread
        ProcessingContext& get_processing_context() const;
        
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
        
        // Safety checks (now thread-safe)
        void check_recursion_limit(ProcessingContext& ctx) const;
        void enter_recursion(ProcessingContext& ctx) const;
        void exit_recursion(ProcessingContext& ctx) const;
    };
}