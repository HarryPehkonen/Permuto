// permuto/include/permuto/exceptions.hpp
#ifndef PERMUTO_EXCEPTIONS_HPP
#define PERMUTO_EXCEPTIONS_HPP

// Export/Import macros for shared library support
#if defined(_WIN32) || defined(_WIN64)
    #if defined(PERMUTO_SHARED)
        #if defined(PERMUTO_EXPORTS)
            #define PERMUTO_API __declspec(dllexport)
        #else
            #define PERMUTO_API __declspec(dllimport)
        #endif
    #else
        #define PERMUTO_API
    #endif
#else
    #if defined(PERMUTO_SHARED)
        #define PERMUTO_API __attribute__((visibility("default")))
    #else
        #define PERMUTO_API
    #endif
#endif

#include <stdexcept>
#include <string>

namespace permuto {

/**
 * @brief Base class for all Permuto exceptions.
 */
class PERMUTO_API PermutoException : public std::runtime_error {
public:
    explicit PermutoException(const std::string& message)
        : std::runtime_error(message) {}
};

/**
 * @brief Exception thrown for errors parsing the input template JSON.
 */
class PERMUTO_API PermutoParseException : public PermutoException {
public:
    explicit PermutoParseException(const std::string& message)
        : PermutoException("Parse Error: " + message) {}
};

/**
 * @brief Exception thrown when a cyclical dependency is detected during substitution.
 */
class PERMUTO_API PermutoCycleException : public PermutoException {
private:
    std::string cycle_path_;
public:
    PermutoCycleException(const std::string& message, std::string cycle_path)
        : PermutoException("Cycle Error: " + message), cycle_path_(std::move(cycle_path)) {}

    /**
     * @brief Get the detected cycle path (e.g., "a -> b -> a").
     */
    const std::string& get_cycle_path() const noexcept { return cycle_path_; }

    // Override what() to potentially include the path
    const char* what() const noexcept override;
};

/**
 * @brief Exception thrown when a key or path is not found in the context
 *        and onMissingKey is set to 'error'.
 */
class PERMUTO_API PermutoMissingKeyException : public PermutoException {
private:
    std::string key_path_;
public:
    PermutoMissingKeyException(const std::string& message, std::string key_path)
        : PermutoException("Missing Key Error: " + message), key_path_(std::move(key_path)) {}

    /**
     * @brief Get the key or path that was not found.
     */
    const std::string& get_key_path() const noexcept { return key_path_; }

    // Override what() to potentially include the path
    const char* what() const noexcept override;
};

/**
 * @brief Exception thrown when the maximum recursion depth is exceeded.
 */
class PERMUTO_API PermutoRecursionDepthException : public PermutoException {
private:
    size_t current_depth_;
    size_t max_depth_;
public:
    PermutoRecursionDepthException(const std::string& message, size_t current_depth, size_t max_depth)
        : PermutoException("Recursion Depth Error: " + message), 
          current_depth_(current_depth), max_depth_(max_depth) {}

    /**
     * @brief Get the current recursion depth when the limit was exceeded.
     */
    size_t get_current_depth() const noexcept { return current_depth_; }

    /**
     * @brief Get the maximum allowed recursion depth.
     */
    size_t get_max_depth() const noexcept { return max_depth_; }

    // Override what() to include depth information
    const char* what() const noexcept override;
};

} // namespace permuto

#endif // PERMUTO_EXCEPTIONS_HPP
