// src/clavis_internal.hpp
#ifndef CLAVIS_INTERNAL_HPP
#define CLAVIS_INTERNAL_HPP

#include <nlohmann/json.hpp>
#include <string>
#include <set>
#include <stdexcept> // For guard exception safety

// Include public headers needed for types used in internal functions
#include "clavis/clavis.hpp" // Brings in Options, MissingKeyBehavior
#include "clavis/exceptions.hpp" // Brings in exception types

namespace clavis {
namespace detail {

// --- Forward Declarations ---
nlohmann::json process_node(
    const nlohmann::json& node,
    const nlohmann::json& context,
    const Options& options,
    std::set<std::string>& active_paths // For cycle detection
);

nlohmann::json process_string(
    const std::string& template_str,
    const nlohmann::json& context,
    const Options& options,
    std::set<std::string>& active_paths
);

nlohmann::json resolve_and_process_placeholder(
    const std::string& path,
    const std::string& full_placeholder,
    const nlohmann::json& context,
    const Options& options,
    std::set<std::string>& active_paths
);

const nlohmann::json* resolve_path(
    const nlohmann::json& context,
    const std::string& path,
    const Options& options,
    const std::string& full_placeholder_for_error // Added for better error messages
);

std::string stringify_json(const nlohmann::json& value);


// --- RAII Guard for cycle detection ---
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
            throw ClavisCycleException("Cycle detected during substitution", std::move(cycle_str));
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
} // namespace clavis

#endif // CLAVIS_INTERNAL_HPP
