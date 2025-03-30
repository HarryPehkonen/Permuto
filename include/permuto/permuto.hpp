// permuto/include/permuto/permuto.hpp
#ifndef PERMUTO_PERMUTO_HPP
#define PERMUTO_PERMUTO_HPP

#include <nlohmann/json.hpp>
#include <string>
#include <stdexcept> // For runtime_error in option validation

namespace permuto {

// Forward declaration
class PermutoException; // Include exceptions.hpp if needed here, or rely on user include

/**
 * @brief Defines behavior when a variable or path cannot be resolved.
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

    /**
     * @brief Validates the options. Throws std::invalid_argument on failure.
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
    }
};

/**
 * @brief Processes a JSON template by substituting variables from a context.
 *
 * Traverses the template JSON structure. When a string value is encountered,
 * it checks for placeholders defined by options.variableStartMarker and
 * options.variableEndMarker. Placeholders are replaced with values looked up
 * in the context object, supporting dot notation for nested access.
 *
 * @param template_json The input JSON template structure (must be valid JSON).
 * @param context The data context object used for variable lookups.
 * @param options Configuration for processing (delimiters, missing key behavior).
 *                Defaults are used if not provided.
 * @return nlohmann::json The resulting JSON structure after substitutions.
 * @throws PermutoCycleException If a cyclical dependency is detected.
 * @throws PermutoMissingKeyException If a key/path is not found and options.onMissingKey is Error.
 * @throws PermutoParseException If the input template_json has issues (though basic validity is often checked beforehand).
 * @throws std::invalid_argument If the provided Options are invalid (e.g., empty delimiters).
 */
nlohmann::json process(
    const nlohmann::json& template_json,
    const nlohmann::json& context,
    const Options& options = {}
);

} // namespace permuto

#endif // PERMUTO_PERMUTO_HPP
