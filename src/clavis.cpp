// clavis/src/clavis.cpp
#include "clavis/clavis.hpp"
#include "clavis/exceptions.hpp"
#include "clavis_internal.hpp" // Include the internal header

#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <set>
#include <stack>
#include <sstream> // For splitting string

namespace clavis {

// --- Public API Implementation ---
// (process function remains the same for now)
nlohmann::json process(
    const nlohmann::json& template_json,
    const nlohmann::json& context,
    const Options& options)
{
    options.validate(); // Validate options first
    std::set<std::string> active_paths; // Initialize cycle detection set
    // Start recursive processing from the root of the template
    // TODO: Eventually call detail::process_node
    // For now, just return something simple to allow compilation
     // return detail::process_node(template_json, context, options, active_paths);
     return template_json; // Temporary return to allow compilation
}


// --- Detail Implementation ---
namespace detail {

// Forward declaration for process_string
nlohmann::json process_string(
    const std::string& template_str,
    const nlohmann::json& context,
    const Options& options,
    std::set<std::string>& active_paths
);

// Placeholder for the actual recursive logic
nlohmann::json process_node(
    const nlohmann::json& node,
    const nlohmann::json& context,
    const Options& options,
    std::set<std::string>& active_paths)
{
    // (Implementation unchanged for now)
    if (node.is_object()) {
        nlohmann::json result_obj = nlohmann::json::object();
        for (auto& [key, val] : node.items()) {
            result_obj[key] = process_node(val, context, options, active_paths);
        }
        return result_obj;
    } else if (node.is_array()) {
        nlohmann::json result_arr = nlohmann::json::array();
        for (const auto& element : node) {
            result_arr.push_back(process_node(element, context, options, active_paths));
        }
        return result_arr;
    } else if (node.is_string()) {
        return process_string(node.get<std::string>(), context, options, active_paths);
    } else {
        return node;
    }
}

// Placeholder for string processing logic
nlohmann::json process_string(
    const std::string& template_str,
    const nlohmann::json& context,
    const Options& options,
    std::set<std::string>& active_paths)
{
    // (Implementation unchanged for now)
    // TODO: Implement string scanning and substitution logic here
    // Use resolve_path(...)

    // TEMPORARY: Return original string until implemented
    return nlohmann::json(template_str);
}


// --- Implementation of resolve_path ---
const nlohmann::json* resolve_path(
    const nlohmann::json& context,
    const std::string& path,
    const Options& options,
    const std::string& full_placeholder_for_error) // Added parameter
{
    if (path.empty()) {
        if (options.onMissingKey == MissingKeyBehavior::Error) {
            throw ClavisMissingKeyException("Path cannot be empty", full_placeholder_for_error);
        }
        return nullptr;
    }

    if (path[0] == '.' || path.back() == '.' || path.find("..") != std::string::npos) {
         if (options.onMissingKey == MissingKeyBehavior::Error) {
            throw ClavisMissingKeyException("Invalid path format (leading/trailing/double dots)", path);
         }
         return nullptr;
    }

    std::vector<std::string> segments;
    std::stringstream ss(path);
    std::string segment;
    while (std::getline(ss, segment, '.')) {
        if (segment.empty()) { // Should be caught by ".." check above, but defensive
             if (options.onMissingKey == MissingKeyBehavior::Error) {
                throw ClavisMissingKeyException("Invalid path format (empty segment)", path);
             }
             return nullptr;
        }
        segments.push_back(segment);
    }

    if (segments.empty()) { // Path was just "."? Caught above. Path was empty? Caught above. Safety check.
        if (options.onMissingKey == MissingKeyBehavior::Error) {
            throw ClavisMissingKeyException("Path resulted in no segments", path);
        }
        return nullptr;
    }

    const nlohmann::json* current_node = &context;
    std::string current_path_str; // For error reporting

    for (size_t i = 0; i < segments.size(); ++i) {
        const std::string& current_segment = segments[i];

        // Append to current path for error messages
        if (!current_path_str.empty()) current_path_str += ".";
        current_path_str += current_segment;

        // Check if we can traverse deeper
        if (!current_node->is_object()) {
            if (options.onMissingKey == MissingKeyBehavior::Error) {
                throw ClavisMissingKeyException("Attempted to access key '" + current_segment + "' on a non-object value", current_path_str);
            }
            return nullptr;
        }

        // Check if the key exists
        if (!current_node->contains(current_segment)) {
            if (options.onMissingKey == MissingKeyBehavior::Error) {
                 // Use the full original path in the exception's key_path_ member for consistency?
                 // Or the path up to the failure point? Let's use the original full path for now.
                throw ClavisMissingKeyException("Key '" + current_segment + "' not found", path);
            }
            return nullptr;
        }

        // Move to the next node
        current_node = &(*current_node)[current_segment];
        // Or: current_node = ¤t_node->at(current_segment); // slightly safer if contains() check failed somehow
    }

    // If we successfully traversed all segments
    return current_node;
}


} // namespace detail
} // namespace clavis
