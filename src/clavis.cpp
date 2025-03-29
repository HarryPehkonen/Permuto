// clavis/src/clavis.cpp
#include "clavis/clavis.hpp"
#include "clavis/exceptions.hpp"

#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <set> // For cycle detection
#include <stack> // Potentially for recursive processing state

namespace clavis {

// Helper function declarations (consider moving to an internal header if complex)
namespace detail {

// Recursive processing function
nlohmann::json process_node(
    const nlohmann::json& node,
    const nlohmann::json& context,
    const Options& options,
    std::set<std::string>& active_paths // For cycle detection
);

// Function to handle string substitution
nlohmann::json process_string(
    const std::string& template_str,
    const nlohmann::json& context,
    const Options& options,
    std::set<std::string>& active_paths
);

// Function to resolve dot notation path
const nlohmann::json* resolve_path(
    const nlohmann::json& context,
    const std::string& path,
    const Options& options // Needed for onMissingKey
    // Potentially add the original full placeholder string for error messages
);

// Helper to split dot notation string? (or handle within resolve_path)

} // namespace detail

// --- Public API Implementation ---

nlohmann::json process(
    const nlohmann::json& template_json,
    const nlohmann::json& context,
    const Options& options)
{
    options.validate(); // Validate options first

    std::set<std::string> active_paths; // Initialize cycle detection set

    // Start recursive processing from the root of the template
    return detail::process_node(template_json, context, options, active_paths);
}


// --- Detail Implementation ---
namespace detail {

// Placeholder for the actual recursive logic
nlohmann::json process_node(
    const nlohmann::json& node,
    const nlohmann::json& context,
    const Options& options,
    std::set<std::string>& active_paths)
{
    if (node.is_object()) {
        nlohmann::json result_obj = nlohmann::json::object();
        for (auto& [key, val] : node.items()) {
            // Recursively process the value associated with the key
            result_obj[key] = process_node(val, context, options, active_paths);
        }
        return result_obj;
    } else if (node.is_array()) {
        nlohmann::json result_arr = nlohmann::json::array();
        for (const auto& element : node) {
            // Recursively process each element in the array
            result_arr.push_back(process_node(element, context, options, active_paths));
        }
        return result_arr;
    } else if (node.is_string()) {
        // Process strings for variable substitution
        return process_string(node.get<std::string>(), context, options, active_paths);
    } else {
        // Numbers, booleans, nulls are returned as is
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
    // TODO: Implement string scanning and substitution logic here
    // 1. Check if the entire string is *exactly* one placeholder.
    //    e.g., template_str == options.variableStartMarker + path + options.variableEndMarker
    // 2. If exact match:
    //    a. Extract the path.
    //    b. Check for cycles using active_paths. Add path, check, remove on exit.
    //    c. Resolve the path using resolve_path().
    //    d. If resolved:
    //       i. If the resolved value is a string, recursively call process_string on it.
    //       ii. If resolved value is non-string (num, bool, obj, arr, null), return it directly (type preserved).
    //    e. If not resolved (and onMissingKey=ignore), return the original template_str as a JSON string.
    // 3. If not exact match (interpolation):
    //    a. Scan the string for start/end markers.
    //    b. For each found placeholder:
    //       i. Extract path.
    //       ii. Check cycles (as above).
    //       iii. Resolve path.
    //       iv. If resolved, convert value to string (json.dump() for obj/arr, "null", "true"/"false", num as string). Recursively process if the resolved value was *itself* a string containing placeholders? Yes, spec implies recursion applies everywhere.
    //       v. If not resolved (ignore), keep the placeholder text.
    //    c. Build the final interpolated string.
    //    d. Return the final string as a JSON string.

    // TEMPORARY: Return original string until implemented
    return nlohmann::json(template_str);
}


// Placeholder for path resolution logic
const nlohmann::json* resolve_path(
    const nlohmann::json& context,
    const std::string& path,
    const Options& options)
{
    // TODO: Implement path resolution logic
    // 1. Split path by '.' into segments.
    // 2. Traverse context object using segments.
    // 3. If any intermediate segment is not found OR is not an object when more segments follow:
    //    a. If options.onMissingKey == Error, throw ClavisMissingKeyException.
    //    b. If options.onMissingKey == Ignore, return nullptr.
    // 4. If final segment found, return pointer to the nlohmann::json value.
    // 5. If final segment not found:
    //    a. If options.onMissingKey == Error, throw ClavisMissingKeyException.
    //    b. If options.onMissingKey == Ignore, return nullptr.

    // TEMPORARY: Return nullptr until implemented
    if (context.contains(path)) {
        // This only handles top-level, non-dotted paths for now
         return &context.at(path);
    } else {
        if (options.onMissingKey == MissingKeyBehavior::Error) {
             throw ClavisMissingKeyException("Key or path not found (implementation pending)", path);
        }
        return nullptr;
    }
}

} // namespace detail
} // namespace clavis
