// permuto/src/permuto.cpp
#include "permuto/permuto.hpp"
#include "permuto/exceptions.hpp"
#include "permuto_internal.hpp" // Include the internal header

#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <set>
#include <stack>
#include <sstream> // For splitting string and building interpolated strings

namespace permuto {

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

// --- Forward Declarations are handled by permuto_internal.hpp ---

// --- Definition of process_node ---
nlohmann::json process_node(
    const nlohmann::json& node,
    const nlohmann::json& context,
    const Options& options,
    std::set<std::string>& active_paths) // Pass active_paths by reference
{
    if (node.is_object()) {
        nlohmann::json result_obj = nlohmann::json::object();
        for (auto& [key, val] : node.items()) {
            // Keys are NOT processed for variables. Values are.
            result_obj[key] = process_node(val, context, options, active_paths);
        }
        return result_obj;
    } else if (node.is_array()) {
        nlohmann::json result_arr = nlohmann::json::array();
        result_arr.get_ptr<nlohmann::json::array_t*>()->reserve(node.size()); // Optimization
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

// --- Definition of process_string ---
nlohmann::json process_string(
    const std::string& template_str,
    const nlohmann::json& context,
    const Options& options,
    std::set<std::string>& active_paths)
{
    const std::string& startMarker = options.variableStartMarker;
    const std::string& endMarker = options.variableEndMarker;
    const size_t startLen = startMarker.length();
    const size_t endLen = endMarker.length();
    const size_t templateLen = template_str.length();
    const size_t minPlaceholderLen = startLen + endLen + 1; // Need at least one char for path

    // Optimization: If string is too short to contain a placeholder, return early
    if (templateLen < minPlaceholderLen) {
        return nlohmann::json(template_str);
    }

     // Optimization: If no start marker exists, return early
    if (template_str.find(startMarker) == std::string::npos) {
        return nlohmann::json(template_str);
    }

    // --- 1. Check for Exact Match ---
    bool starts_with_marker = template_str.rfind(startMarker, 0) == 0;
    bool ends_with_marker = template_str.find(endMarker, templateLen - endLen) == (templateLen - endLen);

    if (starts_with_marker && ends_with_marker)
    {
        // Check if there are *other* markers inside (to ensure it's *only* one placeholder)
        size_t next_start_pos = template_str.find(startMarker, startLen);
        // Ensure next start marker is not found OR it's after where the end marker should start
        bool no_inner_start = (next_start_pos == std::string::npos || next_start_pos >= (templateLen - endLen));

        // No need to check inner end marker explicitly if no_inner_start is true and it ends with marker

        if (no_inner_start) {
             std::string path = template_str.substr(startLen, templateLen - startLen - endLen);
             // Path cannot be empty for a valid placeholder
             if (!path.empty()) {
                 // Directly call the helper. It handles cycles, resolution, recursion.
                 return resolve_and_process_placeholder(path, template_str, context, options, active_paths);
             }
             // If path is empty "${}", treat as literal string below
        }
        // If inner markers found or path was empty, fall through to interpolation logic
    }


    // --- 2. Handle Interpolation ---
    std::stringstream result_stream;
    size_t last_pos = 0;

    while (last_pos < templateLen) {
        size_t start_pos = template_str.find(startMarker, last_pos);

        if (start_pos == std::string::npos) {
            // No more start markers found, append the rest of the string
            result_stream << template_str.substr(last_pos);
            break; // Exit loop
        }

        // Append the literal text between the last placeholder (or start) and this one
        result_stream << template_str.substr(last_pos, start_pos - last_pos);

        size_t end_pos = template_str.find(endMarker, start_pos + startLen);

        if (end_pos == std::string::npos) {
            // Unterminated placeholder, treat the start marker and rest as literal
             result_stream << template_str.substr(start_pos);
             break; // Exit loop
        }

        // Extract path and full placeholder
        std::string path = template_str.substr(start_pos + startLen, end_pos - (start_pos + startLen));
        std::string full_placeholder = template_str.substr(start_pos, end_pos + endLen - start_pos);

        if (path.empty()) {
             // Empty placeholder "${}", treat as literal
             result_stream << full_placeholder;
        } else {
            // Resolve, handle cycles/recursion using the helper
            nlohmann::json resolved_json = resolve_and_process_placeholder(path, full_placeholder, context, options, active_paths);
            // Stringify the result for interpolation
            result_stream << stringify_json(resolved_json);
        }

        // Update position to search after the current placeholder
        last_pos = end_pos + endLen;
    }

    // Return the final interpolated string as a JSON string value
    return nlohmann::json(result_stream.str());
}


// --- Definition of resolve_and_process_placeholder ---
nlohmann::json resolve_and_process_placeholder(
    const std::string& path,
    const std::string& full_placeholder, // Use this for errors/missing return
    const nlohmann::json& context,
    const Options& options,
    std::set<std::string>& active_paths)
{
    // --- Cycle Detection & Path Resolution ---
    ActivePathGuard path_guard(active_paths, path); // Throws on cycle

    const nlohmann::json* resolved_ptr = resolve_path(context, path, options, full_placeholder);

    // --- Handle Result ---
    if (resolved_ptr) {
        const nlohmann::json& resolved_value = *resolved_ptr;

        // --- Handle Recursion for String Results ---
        if (resolved_value.is_string()) {
            // Recursively process the *resolved string* using the same active_paths
            return process_string(resolved_value.get<std::string>(), context, options, active_paths);
        } else {
            // For non-strings (null, bool, num, obj, arr), return the value directly.
            // The caller (process_string) decides whether to use this raw value
            // (for exact match) or stringify it (for interpolation).
            return resolved_value;
        }
    } else {
        // Not found (and onMissingKey == Ignore, because Error would have thrown in resolve_path)
        // Return the original placeholder string itself, wrapped in a JSON value.
        return nlohmann::json(full_placeholder);
    }
    // path_guard automatically removes 'path' from active_paths here upon scope exit
}

// --- Definition of stringify_json ---
std::string stringify_json(const nlohmann::json& value) {
    if (value.is_string()) {
        // String value itself needs to be returned
        return value.get<std::string>();
    } else if (value.is_null()) {
        return "null";
    } else if (value.is_boolean()) {
        return value.get<bool>() ? "true" : "false";
    } else if (value.is_number()) {
        // Dump numbers to string to handle floats/ints consistently
        return value.dump();
    } else if (value.is_structured()) { // Object or Array
        // Use compact dump for interpolation
        return value.dump();
    }
    // Should not happen for valid JSON types passed in
    return ""; // Or throw?
}


// --- Definition of resolve_path ---
const nlohmann::json* resolve_path(
    const nlohmann::json& context,
    const std::string& path,
    const Options& options,
    const std::string& full_placeholder_for_error) // Use this in error messages
{
    // Path should not be empty if called from resolve_and_process_placeholder,
    // but check defensively.
    if (path.empty()) {
        if (options.onMissingKey == MissingKeyBehavior::Error) {
            throw PermutoMissingKeyException("Path cannot be empty", full_placeholder_for_error);
        }
        return nullptr;
    }

    // Basic validation for path format
    if (path[0] == '.' || path.back() == '.' || path.find("..") != std::string::npos) {
         if (options.onMissingKey == MissingKeyBehavior::Error) {
            throw PermutoMissingKeyException("Invalid path format (leading/trailing/double dots): " + path, full_placeholder_for_error);
         }
         return nullptr;
    }

    // Use nlohmann::json::json_pointer for robust parsing and traversal
    // Need to prepend '/' and replace '.' with '/'
    try {
        std::string json_pointer_str = "/";
        size_t start_pos = 0;
        size_t dot_pos;
        while ((dot_pos = path.find('.', start_pos)) != std::string::npos) {
            std::string segment = path.substr(start_pos, dot_pos - start_pos);
            if (segment.empty()) { // Handled by ".." check mostly, but be sure
                 throw PermutoMissingKeyException("Invalid path format (empty segment): " + path, full_placeholder_for_error);
            }
            // JSON Pointer needs '~' escaped to '~0' and '/' escaped to '~1'
            std::string escaped_segment;
            for (char c : segment) {
                if (c == '~') escaped_segment += "~0";
                else if (c == '/') escaped_segment += "~1";
                else escaped_segment += c;
            }
            json_pointer_str += escaped_segment + "/";
            start_pos = dot_pos + 1;
        }
        // Add the last segment
        std::string last_segment = path.substr(start_pos);
         if (last_segment.empty()) { // Trailing dot case
             throw PermutoMissingKeyException("Invalid path format (trailing dot): " + path, full_placeholder_for_error);
         }
        std::string escaped_last_segment;
         for (char c : last_segment) {
             if (c == '~') escaped_last_segment += "~0";
             else if (c == '/') escaped_last_segment += "~1";
             else escaped_last_segment += c;
         }
        json_pointer_str += escaped_last_segment;


        nlohmann::json::json_pointer ptr(json_pointer_str);
        const nlohmann::json& result = context.at(ptr); // .at() throws if not found
        return &result;

    } catch (const nlohmann::json::parse_error& e) {
        // This might happen if our constructed path is bad somehow?
         if (options.onMissingKey == MissingKeyBehavior::Error) {
             throw PermutoMissingKeyException("Failed to parse generated JSON Pointer for path '" + path + "': " + e.what(), full_placeholder_for_error);
         }
         return nullptr;
    } catch (const nlohmann::json::out_of_range& e) {
         // This indicates the path was not found by .at()
         if (options.onMissingKey == MissingKeyBehavior::Error) {
            // Use the original dot-path for user clarity in the exception's path member
             throw PermutoMissingKeyException("Key or path not found: " + path, path);
         }
         return nullptr;
    } catch (const PermutoMissingKeyException& e) {
        // Re-throw exceptions from format checks if error mode is on
        if (options.onMissingKey == MissingKeyBehavior::Error) {
            throw;
        }
        return nullptr;
    }

    // Catch all for unexpected issues during resolution?
    // Should be covered by above catches.
    // return nullptr; // Failsafe
}


} // namespace detail
} // namespace permuto
