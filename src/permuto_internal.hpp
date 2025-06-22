// src/permuto_internal.hpp
#ifndef PERMUTO_INTERNAL_HPP
#define PERMUTO_INTERNAL_HPP

#include <nlohmann/json.hpp>
#include <string>
#include <set>
#include <vector>
#include <stdexcept> // For guard exception safety

// Include public headers needed for types used in internal functions
#include "permuto/permuto.hpp" // Brings in Options, MissingKeyBehavior, PlaceholderParser, PlaceholderInfo
#include "permuto/exceptions.hpp" // Brings in exception types

namespace permuto {
namespace detail {

// --- Forward Declarations for apply() ---
nlohmann::json process_node(
    const nlohmann::json& node,
    const nlohmann::json& context,
    const Options& options,
    std::set<std::string>& active_paths, // For cycle detection
    size_t current_depth = 0 // For recursion depth tracking
);

nlohmann::json process_string(
    const std::string& template_str,
    const nlohmann::json& context,
    const Options& options,
    std::set<std::string>& active_paths,
    size_t current_depth = 0 // For recursion depth tracking
);

// --- Refactored string processing functions ---
nlohmann::json process_exact_match_placeholder(
    const PlaceholderInfo& placeholder,
    const nlohmann::json& context,
    const Options& options,
    std::set<std::string>& active_paths,
    size_t current_depth
);

nlohmann::json process_interpolated_string(
    const std::string& template_str,
    const nlohmann::json& context,
    const Options& options,
    std::set<std::string>& active_paths,
    size_t current_depth
);

nlohmann::json resolve_and_process_placeholder(
    const std::string& path,
    const std::string& full_placeholder,
    const nlohmann::json& context,
    const Options& options,
    std::set<std::string>& active_paths,
    size_t current_depth = 0 // For recursion depth tracking
);

const nlohmann::json* resolve_path(
    const nlohmann::json& context,
    const std::string& path,
    const Options& options
);

std::string stringify_json(const nlohmann::json& value);

// --- Forward Declarations for Reverse Operations ---

void insert_pointer_at_context_path(
    nlohmann::json& reverse_template_node,
    const std::string& context_path,
    const std::string& pointer_to_insert);

// --- Refactored reverse template building functions ---
void process_template_object(
    const nlohmann::json& template_node,
    const std::string& current_pointer_str,
    nlohmann::json& reverse_template_ref,
    const Options& options
);

void process_template_array(
    const nlohmann::json& template_node,
    const std::string& current_pointer_str,
    nlohmann::json& reverse_template_ref,
    const Options& options
);

void process_template_string(
    const nlohmann::json& template_node,
    const std::string& current_pointer_str,
    nlohmann::json& reverse_template_ref,
    const Options& options
);

void build_reverse_template_recursive(
    const nlohmann::json& current_template_node,
    const std::string& current_result_pointer_str,
    nlohmann::json& reverse_template_ref, // Modifying this
    const Options& options);

// --- Refactored context reconstruction functions ---
void process_reverse_object(
    const nlohmann::json& reverse_node,
    const nlohmann::json& result_json,
    nlohmann::json& context_node
);

void process_reverse_pointer(
    const nlohmann::json& reverse_node,
    const nlohmann::json& result_json,
    nlohmann::json& context_node
);

void reconstruct_context_recursive(
    const nlohmann::json& current_reverse_node,
    const nlohmann::json& result_json,
    nlohmann::json& current_context_node // Modifying this
);

// --- RAII Guard for cycle detection (used by apply()) ---
class ActivePathGuard {
    std::set<std::string>& active_paths_;
    const std::string& path_;
    bool added_ = false; // Track if we actually added the path

public:
    ActivePathGuard(std::set<std::string>& active_paths, const std::string& path_to_check)
        : active_paths_(active_paths), path_(path_to_check)
    {
        // Add check for empty path early, shouldn't be processed anyway but defensive.
        if (path_.empty()) {
             // This case should ideally be handled before trying to resolve/guard.
             // If it happens, treat as a non-cycle insertion.
            return;
        }
        auto [iter, inserted] = active_paths_.insert(path_);
        if (!inserted) {
            // Construct cycle path string for better error reporting (simple version)
            std::string cycle_str = "Path '" + path_ + "' involved in cycle.";
             // Note: Could try to reconstruct a more detailed path by passing the stack.
            throw PermutoCycleException("Cycle detected during substitution", std::move(cycle_str));
        }
        added_ = true; // Mark that we added it
    }

    ~ActivePathGuard() {
        // Use a try-catch block because erase might throw, and destructors should not throw.
        // Although std::set::erase(const Key&) is not supposed to throw C++11 onwards if Key's comparison doesn't.
        try {
            if (added_) { // Only erase if we actually added it
                active_paths_.erase(path_);
            }
        } catch (...) {
            // Log error or handle? Difficult in a destructor. std::terminate might be called.
            // For simplicity, we assume erase won't throw here under normal conditions.
        }
    }

    // Prevent copying/moving
    ActivePathGuard(const ActivePathGuard&) = delete;
    ActivePathGuard& operator=(const ActivePathGuard&) = delete;
    ActivePathGuard(ActivePathGuard&&) = delete;
    ActivePathGuard& operator=(ActivePathGuard&&) = delete;
};

} // namespace detail
} // namespace permuto

#endif // PERMUTO_INTERNAL_HPP
