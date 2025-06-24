// permuto/src/permuto.cpp
#include "permuto/permuto.hpp"
#include "permuto/exceptions.hpp"
#include "permuto_internal.hpp" // Include the internal header

#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <set>
#include <stack>
#include <sstream> // For string splitting and building
#include <stdexcept> // logic_error, runtime_error
#include <vector> // For splitting context path
#include <optional>
#include <functional> // For std::reference_wrapper

namespace permuto {

// --- JsonPointerUtils Implementation ---

std::string JsonPointerUtils::escape_segment(const std::string& segment) {
    std::string escaped;
    escaped.reserve(segment.length() + 4); // Reserve generously, assuming few escapes needed
    for (char c : segment) {
        if (c == '~') {
            escaped += "~0";
        } else if (c == '/') {
            escaped += "~1";
        } else {
            escaped += c;
        }
    }
    return escaped;
}

std::vector<std::string> JsonPointerUtils::parse_segments(const std::string& pointer) {
    std::vector<std::string> segments;
    std::string current_segment;
    bool escaping = false;
    
    // Skip the leading '/' and parse the rest
    for (size_t i = 1; i < pointer.length(); ++i) {
        char c = pointer[i];
        
        if (escaping) {
            // Handle escape sequences
            if (c == '0') {
                current_segment += '~';
            } else if (c == '1') {
                current_segment += '/';
            } else {
                throw std::runtime_error("Invalid escape sequence in JSON Pointer: ~" + std::string(1, c));
            }
            escaping = false;
        } else if (c == '~') {
            escaping = true;
        } else if (c == '/') {
            // End of segment
            segments.push_back(current_segment);
            current_segment.clear();
        } else {
            current_segment += c;
        }
    }
    
    // Don't forget the last segment
    if (!current_segment.empty() || pointer.back() == '/') {
        segments.push_back(current_segment);
    }
    
    return segments;
}

bool JsonPointerUtils::is_valid_pointer(const std::string& pointer) {
    // Empty pointer is valid (represents root)
    if (pointer.empty()) {
        return true;
    }
    
    // Non-empty pointers must start with '/'
    if (pointer[0] != '/') {
        return false;
    }
    
    // Validate JSON Pointer syntax
    try {
        nlohmann::json::json_pointer ptr(pointer);
        return true;
    } catch (const nlohmann::json::parse_error&) {
        return false;
    }
}

std::string JsonPointerUtils::create_pointer(const std::vector<std::string>& segments) {
    if (segments.empty()) {
        return "";
    }
    
    std::string result = "/";
    for (size_t i = 0; i < segments.size(); ++i) {
        if (i > 0) {
            result += "/";
        }
        result += escape_segment(segments[i]);
    }
    return result;
}

std::string JsonPointerUtils::dot_to_pointer(const std::string& dot_path) {
    if (dot_path.empty()) {
        return "";
    }
    
    // Split by dots
    std::vector<std::string> segments;
    std::string current_segment;
    
    for (char c : dot_path) {
        if (c == '.') {
            if (!current_segment.empty()) {
                segments.push_back(current_segment);
                current_segment.clear();
            }
        } else {
            current_segment += c;
        }
    }
    
    // Don't forget the last segment
    if (!current_segment.empty()) {
        segments.push_back(current_segment);
    }
    
    // Convert to JSON Pointer format
    return create_pointer(segments);
}

std::string JsonPointerUtils::unescape_segment(const std::string& escaped_segment) {
    std::string unescaped;
    unescaped.reserve(escaped_segment.length());
    
    for (size_t i = 0; i < escaped_segment.length(); ++i) {
        if (escaped_segment[i] == '~' && i + 1 < escaped_segment.length()) {
            if (escaped_segment[i + 1] == '0') {
                unescaped += '~';
                ++i; // Skip the next character
            } else if (escaped_segment[i + 1] == '1') {
                unescaped += '/';
                ++i; // Skip the next character
            } else {
                unescaped += escaped_segment[i];
            }
        } else {
            unescaped += escaped_segment[i];
        }
    }
    
    return unescaped;
}

// --- ContextPath Implementation ---

ContextPath::ContextPath(std::string path) 
    : path_(std::move(path)), is_valid_(validate()) {}

std::optional<ContextPath> ContextPath::from_string(const std::string& path) {
    ContextPath context_path(path);
    if (context_path.is_valid()) {
        return context_path;
    }
    return std::nullopt;
}

bool ContextPath::validate() const {
    return JsonPointerUtils::is_valid_pointer(path_);
}

// --- PlaceholderParser Implementation ---

bool PlaceholderParser::is_exact_match_placeholder(const std::string& template_str) const {
    const size_t templateLen = template_str.length();
    
    // Check for exact match structure
    bool starts_with_marker = (templateLen >= min_placeholder_length_) && 
                             (template_str.rfind(start_marker_, 0) == 0);
    bool ends_with_marker = (templateLen >= min_placeholder_length_) && 
                           (template_str.find(end_marker_, templateLen - end_marker_.length()) == (templateLen - end_marker_.length()));

    if (!starts_with_marker || !ends_with_marker) {
        return false;
    }

    // Check for inner markers to ensure *only* one placeholder
    size_t next_start_pos = template_str.find(start_marker_, start_marker_.length());
    bool no_inner_start = (next_start_pos == std::string::npos || 
                          next_start_pos >= (templateLen - end_marker_.length()));

    return no_inner_start;
}

PlaceholderInfo PlaceholderParser::extract_placeholder_info(const std::string& template_str) const {
    if (!is_exact_match_placeholder(template_str)) {
        return PlaceholderInfo{};
    }
    
    const size_t startLen = start_marker_.length();
    const size_t endLen = end_marker_.length();
    const size_t templateLen = template_str.length();
    
    std::string path_str = template_str.substr(startLen, templateLen - startLen - endLen);
    
    // Empty placeholders should be treated as invalid (returned as literals)
    if (path_str.empty()) {
        return PlaceholderInfo{
            ContextPath(""),  // Empty path
            template_str,     // Full placeholder
            0,               // Start pos
            templateLen,     // End pos
            false           // Invalid - should be treated as literal
        };
    }
    
    // If path_str starts with '/', treat as JSON Pointer, else convert from dot notation
    std::string pointer_path = (path_str[0] == '/') ? path_str : JsonPointerUtils::dot_to_pointer(path_str);
    ContextPath path(pointer_path);
    
    return PlaceholderInfo{
        std::move(path),
        template_str,
        0,
        templateLen,
        path.is_valid()
    };
}

std::vector<PlaceholderInfo> PlaceholderParser::find_all_placeholders(const std::string& template_str) const {
    std::vector<PlaceholderInfo> placeholders;
    const size_t templateLen = template_str.length();
    size_t current_pos = 0;

    while (current_pos < templateLen) {
        size_t start_pos = template_str.find(start_marker_, current_pos);

        if (start_pos == std::string::npos) {
            break;
        }

        size_t end_pos = template_str.find(end_marker_, start_pos + start_marker_.length());

        if (end_pos == std::string::npos) {
            break;
        }

        // Extract path and validate
        std::string path_str = template_str.substr(start_pos + start_marker_.length(), 
                                                  end_pos - (start_pos + start_marker_.length()));
        std::string full_placeholder = template_str.substr(start_pos, end_pos + end_marker_.length() - start_pos);
        
        // If path_str starts with '/', treat as JSON Pointer, else convert from dot notation
        std::string pointer_path = (!path_str.empty() && path_str[0] == '/') ? path_str : JsonPointerUtils::dot_to_pointer(path_str);
        
        // Empty placeholders should be treated as invalid (returned as literals)
        bool is_valid = !path_str.empty() && !pointer_path.empty();
        ContextPath path(pointer_path);

        placeholders.emplace_back(std::move(path), std::move(full_placeholder), 
                                 start_pos, end_pos + end_marker_.length(), is_valid);

        current_pos = end_pos + end_marker_.length();
    }

    return placeholders;
}

// --- Public API Implementation ---

nlohmann::json apply(
    const nlohmann::json& template_json,
    const nlohmann::json& context,
    const Options& options)
{
    options.validate(); // Validate options first

    std::set<std::string> active_paths; // Initialize cycle detection set

    // Start recursive processing from the root of the template
    return detail::process_node(template_json, context, options, active_paths, 0);
}


nlohmann::json create_reverse_template(
    const nlohmann::json& original_template,
    const Options& options)
{
    options.validate();

    if (options.enableStringInterpolation) {
        throw std::logic_error("Cannot create a reverse template when string interpolation is enabled in options.");
    }

    nlohmann::json reverse_template = nlohmann::json::object();
    std::string initial_pointer_str = ""; // Start with empty pointer string (represents root)

    // Start recursive building process
    detail::build_reverse_template_recursive(
        original_template,
        initial_pointer_str, // Pass pointer string
        reverse_template,
        options);

    return reverse_template;
}


nlohmann::json apply_reverse(
    const nlohmann::json& reverse_template,
    const nlohmann::json& result_json)
{
    nlohmann::json reconstructed_context = nlohmann::json::object();

    // Check if reverse_template is an object before starting
    if (!reverse_template.is_object()) {
         // Allow empty object, but not other types like array, string etc.
        if (!reverse_template.empty()) {
             throw std::runtime_error("Reverse template root must be a JSON object.");
        }
         return reconstructed_context; // Return empty object if reverse template is empty
    }


    detail::reconstruct_context_recursive(
        reverse_template,
        result_json,
        reconstructed_context // Pass the object to be filled
    );

    return reconstructed_context;
}


// --- Detail Implementation ---
namespace detail {

// Helper to insert a pointer string into the reverse template structure
// This function converts a JSON Pointer path into a nested object structure
// and inserts the result pointer at the appropriate location
void insert_pointer_at_context_path(
    nlohmann::json& reverse_template_node, // Root node to modify
    const std::string& context_path,
    const std::string& pointer_to_insert)
{
    if (context_path.empty() || context_path[0] != '/') {
        throw std::runtime_error("[Permuto Internal] Context path must be a JSON Pointer starting with '/': " + context_path);
    }
    
    std::vector<std::string> segments = JsonPointerUtils::parse_segments(context_path);
    
    // Build the nested structure
    nlohmann::json* current_node = &reverse_template_node;
    
    for (size_t i = 0; i < segments.size(); ++i) {
        const std::string& segment = segments[i];
        
        if (i == segments.size() - 1) {
            // Last segment: assign the pointer string
            if (current_node->contains(segment)) {
                // Conflict: already exists and is not the same as pointer_to_insert
                if (!(*current_node)[segment].is_string() || (*current_node)[segment] != pointer_to_insert) {
                    throw std::runtime_error("Context path conflict in reverse template: segment '" + segment + "' already assigned to a different pointer or is an object.");
                }
            } else {
                (*current_node)[segment] = pointer_to_insert;
            }
        } else {
            // Intermediate segment: ensure object exists
            if (!current_node->contains(segment)) {
                (*current_node)[segment] = nlohmann::json::object();
            } else if (!(*current_node)[segment].is_object()) {
                // Conflict: trying to create an object where a string already exists
                throw std::runtime_error("Context path conflict in reverse template: segment '" + segment + "' is already assigned as a value, cannot create nested object.");
            }
            current_node = &(*current_node)[segment];
        }
    }
}

// --- Definition of build_reverse_template_recursive ---
void build_reverse_template_recursive(
    const nlohmann::json& current_template_node,
    const std::string& current_result_pointer_str, // Pass pointer string
    nlohmann::json& reverse_template_ref, // Modifying this (root)
    const Options& options)
{
    if (current_template_node.is_object()) {
        process_template_object(current_template_node, current_result_pointer_str, reverse_template_ref, options);
    } else if (current_template_node.is_array()) {
        process_template_array(current_template_node, current_result_pointer_str, reverse_template_ref, options);
    } else if (current_template_node.is_string()) {
        process_template_string(current_template_node, current_result_pointer_str, reverse_template_ref, options);
    }
    // Ignore other types (null, bool, number) in the template
}

// --- Refactored reverse template building functions ---

void process_template_object(
    const nlohmann::json& template_node,
    const std::string& current_pointer_str,
    nlohmann::json& reverse_template_ref,
    const Options& options)
{
    for (auto& [key, val] : template_node.items()) {
        // Unescape the key first, then escape it for the JSON Pointer
        std::string unescaped_key = JsonPointerUtils::unescape_segment(key);
        std::string escaped_key = JsonPointerUtils::escape_segment(unescaped_key);
        std::string next_pointer_str = current_pointer_str + "/" + escaped_key;
        build_reverse_template_recursive(val, next_pointer_str, reverse_template_ref, options);
    }
}

void process_template_array(
    const nlohmann::json& template_node,
    const std::string& current_pointer_str,
    nlohmann::json& reverse_template_ref,
    const Options& options)
{
    for (size_t i = 0; i < template_node.size(); ++i) {
        // Array indices are segments in the pointer string
        std::string next_pointer_str = current_pointer_str + "/" + std::to_string(i);
        build_reverse_template_recursive(template_node[i], next_pointer_str, reverse_template_ref, options);
    }
}

void process_template_string(
    const nlohmann::json& template_node,
    const std::string& current_pointer_str,
    nlohmann::json& reverse_template_ref,
    const Options& options)
{
    // Check if this string is an exact match placeholder
    const std::string& template_str = template_node.get<std::string>();
    PlaceholderParser parser(options);
    
    PlaceholderInfo placeholder = parser.extract_placeholder_info(template_str);
    if (placeholder.is_valid) {
        try {
            insert_pointer_at_context_path(reverse_template_ref, placeholder.path.get_path(), current_pointer_str);
        } catch (const std::runtime_error& e) {
            // Add context to the error message
            throw std::runtime_error(std::string("Error building reverse template: ") + e.what());
        }
    }
    // If not an exact match placeholder, ignore (it's a literal or interpolated string)
}

// --- Definition of reconstruct_context_recursive ---
void reconstruct_context_recursive(
    const nlohmann::json& current_reverse_node,
    const nlohmann::json& result_json,
    nlohmann::json& current_context_node // Modifying this
    )
{
    if (current_reverse_node.is_object()) {
        process_reverse_object(current_reverse_node, result_json, current_context_node);
    } else if (current_reverse_node.is_string()) {
        process_reverse_pointer(current_reverse_node, result_json, current_context_node);
    } else {
        // Invalid type found in reverse template structure
        throw std::runtime_error("Invalid node type encountered in reverse template. Expected object or string (JSON Pointer), found: " + std::string(current_reverse_node.type_name()));
    }
}

// --- Refactored context reconstruction functions ---

void process_reverse_object(
    const nlohmann::json& reverse_node,
    const nlohmann::json& result_json,
    nlohmann::json& context_node)
{
    // Ensure the target context node is also an object
    if (!context_node.is_object()) {
        // If it's the initial call (target is likely the empty root object) or
        // if we are overwriting a previous string pointer, make it an object.
        context_node = nlohmann::json::object();
    }

    for (auto& [key, val] : reverse_node.items()) {
        // If key doesn't exist, operator[] will create a null node first.
        // We recursively call, and the recursive call will handle turning that null
        // into an object or assigning the final value as needed.
        reconstruct_context_recursive(val, result_json, context_node[key]);
    }
}

void process_reverse_pointer(
    const nlohmann::json& reverse_node,
    const nlohmann::json& result_json,
    nlohmann::json& context_node)
{
    // This should be a JSON Pointer string. Look up value in result_json.
    const std::string& pointer_str = reverse_node.get<std::string>();
    try {
        // Use the pointer string directly with at()
        const nlohmann::json& looked_up_value = result_json.at(nlohmann::json::json_pointer(pointer_str));

        // Assign the looked-up value to the current position in the context
        // This overwrites the intermediate object/null created by operator[] above if necessary.
        context_node = looked_up_value;

    } catch (const nlohmann::json::parse_error& e) {
        throw std::runtime_error("Invalid JSON Pointer string found in reverse template: '" + pointer_str + "'. Details: " + e.what());
    } catch (const nlohmann::json::out_of_range& e) {
        throw std::runtime_error("JSON Pointer '" + pointer_str + "' not found in result JSON. Details: " + e.what());
    } catch (const std::exception& e) { // Catch potential other errors during lookup/assignment
         throw std::runtime_error("Error processing pointer '" + pointer_str + "': " + e.what());
    }
}


// --- Existing process_node, process_string, resolve_and_process_placeholder, etc. ---

// --- Definition of process_node ---
nlohmann::json process_node(
    const nlohmann::json& node,
    const nlohmann::json& context,
    const Options& options,
    std::set<std::string>& active_paths, // Pass active_paths by reference
    size_t current_depth) // For recursion depth tracking
{
    // Recursion depth check (fix for RecursionDepthLimit test)
    if (current_depth >= options.maxRecursionDepth) {
        throw PermutoRecursionDepthException(
            "Maximum recursion depth exceeded during node processing",
            current_depth,
            options.maxRecursionDepth
        );
    }
    if (node.is_object()) {
        nlohmann::json result_obj = nlohmann::json::object();
        for (auto& [key, val] : node.items()) {
            // Keys are NOT processed for variables. Values are.
            result_obj[key] = process_node(val, context, options, active_paths, current_depth + 1);
        }
        return result_obj;
    } else if (node.is_array()) {
        nlohmann::json result_arr = nlohmann::json::array();
        for (const auto& element : node) {
            // Recursively process each element in the array
            result_arr.push_back(process_node(element, context, options, active_paths, current_depth + 1));
        }
        return result_arr;
    } else if (node.is_string()) {
        // Process string values for placeholders
        return process_string(node.get<std::string>(), context, options, active_paths, current_depth);
    } else {
        // For non-string primitives (null, bool, number), return as-is
        return node;
    }
}

// --- Definition of process_string ---
nlohmann::json process_string(
    const std::string& template_str,
    const nlohmann::json& context,
    const Options& options,
    std::set<std::string>& active_paths,
    size_t current_depth)
{
    // Check recursion depth limit
    if (current_depth >= options.maxRecursionDepth) {
        throw PermutoRecursionDepthException(
            "Maximum recursion depth exceeded during string processing",
            current_depth,
            options.maxRecursionDepth
        );
    }

    PlaceholderParser parser(options);
    
    // Check for exact match placeholder first
    PlaceholderInfo placeholder = parser.extract_placeholder_info(template_str);
    if (placeholder.is_valid) {
        return process_exact_match_placeholder(placeholder, context, options, active_paths, current_depth);
    }

    // Handle based on interpolation option
    if (!options.enableStringInterpolation) {
        return nlohmann::json(template_str);
    }

    // Perform interpolation
    return process_interpolated_string(template_str, context, options, active_paths, current_depth);
}

// --- Refactored string processing functions ---

nlohmann::json process_exact_match_placeholder(
    const PlaceholderInfo& placeholder,
    const nlohmann::json& context,
    const Options& options,
    std::set<std::string>& active_paths,
    size_t current_depth)
{
    nlohmann::json resolved_value = resolve_and_process_placeholder(
        placeholder.path.get_path(), 
        placeholder.full_placeholder, 
        context, 
        options, 
        active_paths, 
        current_depth
    );
    
    // If interpolation is enabled, stringify the result
    if (options.enableStringInterpolation) {
        return nlohmann::json(stringify_json(resolved_value));
    }
    
    // Otherwise, return the raw value (preserving type)
    return resolved_value;
}

nlohmann::json process_interpolated_string(
    const std::string& template_str,
    const nlohmann::json& context,
    const Options& options,
    std::set<std::string>& active_paths,
    size_t current_depth)
{
    // Check recursion depth limit
    if (current_depth >= options.maxRecursionDepth) {
        throw PermutoRecursionDepthException(
            "Maximum recursion depth exceeded during string interpolation",
            current_depth,
            options.maxRecursionDepth
        );
    }

    PlaceholderParser parser(options);
    std::vector<PlaceholderInfo> placeholders = parser.find_all_placeholders(template_str);
    
    if (placeholders.empty()) {
        return nlohmann::json(template_str);
    }

    std::stringstream result_stream;
    size_t current_pos = 0;

    for (const auto& placeholder : placeholders) {
        // Append the literal text between the last placeholder (or start) and this one
        result_stream << template_str.substr(current_pos, placeholder.start_pos - current_pos);

        if (placeholder.is_valid) {
            // Resolve the placeholder
            nlohmann::json resolved_json = resolve_and_process_placeholder(
                placeholder.path.get_path(), 
                placeholder.full_placeholder, 
                context, 
                options, 
                active_paths, 
                current_depth + 1
            );
            result_stream << stringify_json(resolved_json);
        } else {
            // Invalid placeholder, treat as literal
            result_stream << placeholder.full_placeholder;
        }

        current_pos = placeholder.end_pos;
    }

    // Append any remaining text after the last placeholder
    if (current_pos < template_str.length()) {
        result_stream << template_str.substr(current_pos);
    }

    return nlohmann::json(result_stream.str());
}

// --- Definition of resolve_and_process_placeholder ---
nlohmann::json resolve_and_process_placeholder(
    const std::string& path,
    const std::string& full_placeholder, // Use this for errors/missing return
    const nlohmann::json& context,
    const Options& options,
    std::set<std::string>& active_paths,
    size_t current_depth)
{
    ActivePathGuard path_guard(active_paths, path); // Throws on cycle

    auto resolved_opt = resolve_path(context, path, options);

    if (resolved_opt) {
        const nlohmann::json& resolved_value = resolved_opt->get();

        // If the resolved value is itself a string, it might contain more placeholders.
        // Recursively process it *using the current options* (which respects enableStringInterpolation).
        if (resolved_value.is_string()) {
            // The recursive call here correctly handles the interpolation mode.
            // If interpolation is disabled, it will only process exact matches within the resolved string.
            return process_string(resolved_value.get<std::string>(), context, options, active_paths, current_depth + 1);
        } else {
            // For non-strings (null, bool, num, obj, arr), return the value directly.
            // The caller (process_string) decides whether to use this raw value
            // (for exact match) or stringify it (for interpolation).
            return resolved_value;
        }
    } else {
        // Not found (and onMissingKey == Ignore)
        // Return the original placeholder string itself, wrapped in a JSON value.
        return nlohmann::json(full_placeholder);
    }
    // path_guard automatically removes 'path' from active_paths here upon scope exit
}

// --- Definition of stringify_json ---
std::string stringify_json(const nlohmann::json& value) {
    if (value.is_string()) {
        return value.get<std::string>(); // Already a string
    } else if (value.is_null()) {
        return "null";
    } else if (value.is_boolean()) {
        return value.get<bool>() ? "true" : "false";
    } else if (value.is_number()) {
        return value.dump(); // Consistent number stringification
    } else if (value.is_structured()) { // Object or Array
        // Use compact dump for interpolation: no indent, no spaces, no newlines
        return value.dump(-1, ' ', false, nlohmann::json::error_handler_t::strict);
    }
    return ""; // Should not happen for valid JSON
}


// --- Definition of resolve_path ---
// Uses JSON Pointer (RFC 6901) syntax exclusively for path resolution
std::optional<std::reference_wrapper<const nlohmann::json>> resolve_path(
    const nlohmann::json& context,
    const std::string& path,
    const Options& options)
{
    // Empty path represents the root document in JSON Pointer
    if (path.empty()) {
        return std::make_optional(std::ref(context));
    }
    
    // Non-empty JSON Pointers must start with '/'
    if (path[0] != '/') {
        if (options.onMissingKey == MissingKeyBehavior::Error) {
            throw PermutoMissingKeyException("Path must be a JSON Pointer starting with '/' or be empty for root: '" + path + "'", path);
        }
        return std::nullopt;
    }
    
    // Use JSON Pointer for lookup
    try {
        nlohmann::json::json_pointer ptr(path);
        const nlohmann::json& result = context.at(ptr);
        return std::make_optional(std::ref(result));
        
    } catch (const nlohmann::json::parse_error& e) {
        // Invalid JSON Pointer syntax
        if (options.onMissingKey == MissingKeyBehavior::Error) {
            throw PermutoMissingKeyException("Invalid JSON Pointer syntax: '" + path + "'. Details: " + e.what(), path);
        }
        return std::nullopt;
        
    } catch (const nlohmann::json::out_of_range& e) {
        // Path not found in context
        if (options.onMissingKey == MissingKeyBehavior::Error) {
            throw PermutoMissingKeyException("JSON Pointer not found in context: '" + path + "'", path);
        }
        return std::nullopt;
        
    } catch (const std::exception& e) {
        // Any other unexpected errors
        if (options.onMissingKey == MissingKeyBehavior::Error) {
            throw PermutoException("Unexpected error resolving path '" + path + "': " + e.what());
        }
        return std::nullopt;
    }
}


} // namespace detail
} // namespace permuto
