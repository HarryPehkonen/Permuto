// permuto/include/permuto/exceptions.hpp
#ifndef PERMUTO_EXCEPTIONS_HPP
#define PERMUTO_EXCEPTIONS_HPP

#include <stdexcept>
#include <string>

namespace permuto {

/**
 * @brief Base class for all Permuto exceptions.
 */
class PermutoException : public std::runtime_error {
public:
    explicit PermutoException(const std::string& message)
        : std::runtime_error(message) {}
};

/**
 * @brief Exception thrown for errors parsing the input template JSON.
 */
class PermutoParseException : public PermutoException {
public:
    explicit PermutoParseException(const std::string& message)
        : PermutoException("Parse Error: " + message) {}
};

/**
 * @brief Exception thrown when a cyclical dependency is detected during substitution.
 */
class PermutoCycleException : public PermutoException {
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
class PermutoMissingKeyException : public PermutoException {
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

} // namespace permuto

#endif // PERMUTO_EXCEPTIONS_HPP
