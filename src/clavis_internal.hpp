// src/clavis_internal.hpp
#ifndef CLAVIS_INTERNAL_HPP
#define CLAVIS_INTERNAL_HPP

#include <nlohmann/json.hpp>
#include <string>
#include <set> // Keep this here as process_node/process_string might move here later if needed

// Include public headers needed for types used in internal functions
#include "clavis/clavis.hpp" // Brings in Options, MissingKeyBehavior
#include "clavis/exceptions.hpp" // Brings in exception types

namespace clavis {
namespace detail {

/**
 * @brief Resolves a dot-separated path string within a JSON context.
 *
 * Traverses the context according to the segments in the path.
 * Handles missing keys based on options.onMissingKey.
 *
 * @param context The JSON object to search within.
 * @param path The dot-separated path string (e.g., "user.profile.email").
 * @param options Configuration options, primarily used for onMissingKey behavior.
 * @param full_placeholder_for_error If an error occurs, this string is used in the exception message (helps debugging).
 * @return const nlohmann::json* Pointer to the found JSON node, or nullptr if not found and onMissingKey is Ignore.
 * @throws ClavisMissingKeyException If a key/path segment is not found or traversal enters a non-object, and onMissingKey is Error.
 */
const nlohmann::json* resolve_path(
    const nlohmann::json& context,
    const std::string& path,
    const Options& options,
    const std::string& full_placeholder_for_error // Added for better error messages
);

// --- Keep other detail declarations if we decide to move them later ---
// nlohmann::json process_node(...)
// nlohmann::json process_string(...)


} // namespace detail
} // namespace clavis

#endif // CLAVIS_INTERNAL_HPP
