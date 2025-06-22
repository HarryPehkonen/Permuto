// permuto/include/permuto/permuto.hpp
#ifndef PERMUTO_PERMUTO_HPP
#define PERMUTO_PERMUTO_HPP

#include <nlohmann/json.hpp>
#include <string>
#include <stdexcept> // For runtime_error, logic_error
#include <optional>

namespace permuto {

// Forward declaration from exceptions.hpp - include exceptions.hpp for use.
class PermutoException;
class PermutoCycleException;
class PermutoMissingKeyException;
class PermutoParseException;
class PermutoRecursionDepthException;

/**
 * @brief Defines behavior when a variable or path cannot be resolved during apply().
 */
enum class MissingKeyBehavior {
    Ignore, ///< Leave the placeholder string as-is in the output.
    Error   ///< Throw a PermutoMissingKeyException.
};

/**
 * @brief Configuration options for the Permuto processing engine.
 */
struct Options {
    std::string variableStartMarker = "${";
    std::string variableEndMarker = "}";
    MissingKeyBehavior onMissingKey = MissingKeyBehavior::Ignore;
    bool enableStringInterpolation = false;
    size_t maxRecursionDepth = 64; ///< Maximum recursion depth for safety (default: 64)

    /**
     * @brief Validates the options. Throws std::invalid_argument on failure.
     * Checks for empty delimiters or identical start/end delimiters.
     */
    void validate() const {
        if (variableStartMarker.empty()) {
            throw std::invalid_argument("variableStartMarker cannot be empty.");
        }
        if (variableEndMarker.empty()) {
            throw std::invalid_argument("variableEndMarker cannot be empty.");
        }
        if (variableStartMarker == variableEndMarker) {
            throw std::invalid_argument("variableStartMarker and variableEndMarker cannot be identical.");
        }
        // Future checks could go here (e.g., markers containing invalid chars?)
    }
};

/**
 * @brief Utility class for JSON Pointer operations.
 */
class JsonPointerUtils {
public:
    /**
     * @brief Escapes a string segment for use in JSON Pointer.
     * Escapes '~' to '~0' and '/' to '~1' according to RFC 6901.
     */
    static std::string escape_segment(const std::string& segment);
    
    /**
     * @brief Parses a JSON Pointer into its component segments.
     * @throws std::runtime_error if the pointer has invalid syntax.
     */
    static std::vector<std::string> parse_segments(const std::string& pointer);
    
    /**
     * @brief Validates if a string is a valid JSON Pointer.
     */
    static bool is_valid_pointer(const std::string& pointer);
    
    /**
     * @brief Creates a JSON Pointer from segments.
     */
    static std::string create_pointer(const std::vector<std::string>& segments);
};

/**
 * @brief Represents a JSON Pointer path with validation and error handling.
 */
class ContextPath {
private:
    std::string path_;
    bool is_valid_;
    
public:
    ContextPath() : path_(""), is_valid_(true) {} // Default constructor for empty path (root)
    explicit ContextPath(std::string path);
    
    /**
     * @brief Creates a ContextPath from a string, returning nullopt if invalid.
     */
    static std::optional<ContextPath> from_string(const std::string& path);
    
    /**
     * @brief Gets the underlying path string.
     */
    const std::string& get_path() const { return path_; }
    
    /**
     * @brief Checks if this is a valid JSON Pointer path.
     */
    bool is_valid() const { return is_valid_; }
    
    /**
     * @brief Checks if this path is empty (represents root).
     */
    bool is_empty() const { return path_.empty(); }
    
    /**
     * @brief Validates the path format.
     */
    bool validate() const;
};

/**
 * @brief Represents a parsed placeholder with its metadata.
 */
struct PlaceholderInfo {
    ContextPath path;           ///< The extracted path (e.g., "/user/name")
    std::string full_placeholder; ///< The complete placeholder (e.g., "${/user/name}")
    size_t start_pos;           ///< Start position in the original string
    size_t end_pos;             ///< End position in the original string
    bool is_valid;              ///< Whether this is a valid JSON Pointer path
    
    PlaceholderInfo() : start_pos(0), end_pos(0), is_valid(false) {}
    
    PlaceholderInfo(ContextPath p, std::string fp, size_t sp, size_t ep, bool valid)
        : path(std::move(p)), full_placeholder(std::move(fp)), start_pos(sp), end_pos(ep), is_valid(valid) {}
};

/**
 * @brief Parses and validates placeholders in template strings.
 */
class PlaceholderParser {
private:
    std::string start_marker_;
    std::string end_marker_;
    size_t min_placeholder_length_;
    
public:
    explicit PlaceholderParser(const Options& options)
        : start_marker_(options.variableStartMarker)
        , end_marker_(options.variableEndMarker)
        , min_placeholder_length_(start_marker_.length() + end_marker_.length() + 1) {}
    
    /**
     * @brief Checks if a string is an exact match placeholder.
     */
    bool is_exact_match_placeholder(const std::string& template_str) const;
    
    /**
     * @brief Extracts placeholder information from a string.
     */
    PlaceholderInfo extract_placeholder_info(const std::string& template_str) const;
    
    /**
     * @brief Finds all placeholders in a string for interpolation.
     */
    std::vector<PlaceholderInfo> find_all_placeholders(const std::string& template_str) const;
    
    /**
     * @brief Gets the minimum length required for a valid placeholder.
     */
    size_t get_min_placeholder_length() const { return min_placeholder_length_; }
    
    /**
     * @brief Gets the start marker.
     */
    const std::string& get_start_marker() const { return start_marker_; }
    
    /**
     * @brief Gets the end marker.
     */
    const std::string& get_end_marker() const { return end_marker_; }
};

/**
 * @brief Processes a JSON template by substituting variables from a context.
 *
 * Traverses the template JSON structure. When a string value is encountered,
 * it checks for placeholders defined by options.variableStartMarker and
 * options.variableEndMarker.
 * - If a string is an *exact match* for a placeholder (e.g., "${path.to.var}"),
 *   it's replaced by the corresponding value from the context, preserving the type
 *   (number, boolean, null, array, object, string). This happens regardless of
 *   the enableStringInterpolation setting.
 * - If `enableStringInterpolation` is true, placeholders within larger
 *   strings are substituted with their stringified representation. Non-string
 *   values are converted to compact JSON strings.
 * - If `enableStringInterpolation` is false (default), non-exact-match strings containing
 *   placeholders are treated as literals.
 *
 * Supports dot notation for nested access (e.g., "user.address.city"). Keys containing
 * literal dots (.), tildes (~), or slashes (/) are handled via JSON Pointer escaping.
 * Detects and throws on cyclical dependencies. Enforces a maximum recursion depth
 * (default: 64 levels) to prevent stack overflow from legitimate deep recursion.
 *
 * @param template_json The input JSON template structure (must be valid JSON).
 * @param context The data context object used for variable lookups.
 * @param options Configuration for processing (delimiters, missing key behavior, interpolation).
 *                Defaults are used if not provided ({}, meaning interpolation OFF, ignore missing).
 * @return nlohmann::json The resulting JSON structure after substitutions.
 * @throws permuto::PermutoCycleException If a cyclical dependency is detected during lookup.
 * @throws permuto::PermutoMissingKeyException If a key/path is not found and options.onMissingKey is Error.
 * @throws permuto::PermutoRecursionDepthException If the maximum recursion depth is exceeded.
 * @throws std::invalid_argument If the provided Options are invalid (e.g., empty/identical delimiters).
 * @throws nlohmann::json::exception For JSON parsing or access errors within the context/template structure itself.
 */
nlohmann::json apply(
    const nlohmann::json& template_json,
    const nlohmann::json& context,
    const Options& options = {} // Default constructs Options
);


/**
 * @brief Creates a "reverse template" from an original template.
 *
 * This reverse template maps context variable paths (from exact-match placeholders
 * like "${var.path}" in the original template) to their location (JSON Pointer)
 * within the *result* JSON that would be generated by `apply()` *if string
 * interpolation were disabled*.
 *
 * **Requires `options.enableStringInterpolation` to be `false`.**
 *
 * Example:
 *   original_template = {"userName": "${user.name}", "data": ["${sys.id}"]}
 *   (with options.enableStringInterpolation = false)
 *   Resulting reverse_template =
 *     {"user": {"name": "/userName"}, "sys": {"id": "/data/0"}}
 *
 * @param original_template The original JSON template used for the `apply` operation.
 * @param options The *same* options structure used for the original `apply` call.
 *                Crucially, `enableStringInterpolation` must be `false`. Default options work.
 * @return nlohmann::json The generated reverse template.
 * @throws std::logic_error If `options.enableStringInterpolation` is `true`.
 * @throws std::invalid_argument If the provided Options are invalid (e.g., empty/identical delimiters).
 * @throws std::runtime_error If errors occur during processing, such as conflicting context paths.
 * @throws nlohmann::json::exception For issues traversing the original_template.
 */
nlohmann::json create_reverse_template(
    const nlohmann::json& original_template,
    const Options& options = {} // Default options are interp=false, ignore_missing
);

/**
 * @brief Reconstructs the original context data using a reverse template and result JSON.
 *
 * This function takes a `reverse_template` (generated by `create_reverse_template`)
 * and a `result_json` (the output of the original `apply` call, which MUST have
 * been generated with `enableStringInterpolation = false`) and reconstructs the
 * portion of the original context that was referenced by exact-match placeholders
 * in the original template.
 *
 * It traverses the `reverse_template`. When it finds a string value (which is
 * expected to be a JSON Pointer), it uses that pointer to look up the corresponding
 * value in `result_json` and places it in the reconstructed context according
 * to the structure defined by the `reverse_template`.
 *
 * Example:
 *   reverse_template = {"user": {"name": "/userName"}, "sys": {"id": "/data/0"}}
 *   result_json = {"userName": "Alice", "data": [123]}
 *   Resulting reconstructed_context = {"user": {"name": "Alice"}, "sys": {"id": 123}}
 *
 * @param reverse_template The reverse template generated by `create_reverse_template`.
 * @param result_json The result JSON generated by `apply()` with interpolation disabled.
 * @return nlohmann::json The reconstructed context data.
 * @throws std::runtime_error If the `reverse_template` contains invalid structure
 *         (e.g., non-object nodes where nesting is expected, non-string leaf nodes,
 *         invalid JSON pointer strings) or if a JSON pointer from the
 *         `reverse_template` cannot be resolved within `result_json`.
 * @throws nlohmann::json::exception For JSON parsing errors (e.g., invalid pointer syntax)
 *         or access errors (`out_of_range` if pointer lookup fails).
 */
nlohmann::json apply_reverse(
    const nlohmann::json& reverse_template,
    const nlohmann::json& result_json
);

} // namespace permuto

#endif // PERMUTO_PERMUTO_HPP