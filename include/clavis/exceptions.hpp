// clavis/include/clavis/exceptions.hpp
#ifndef CLAVIS_EXCEPTIONS_HPP
#define CLAVIS_EXCEPTIONS_HPP

#include <stdexcept>
#include <string>

namespace clavis {

/**
 * @brief Base class for all Clavis exceptions.
 */
class ClavisException : public std::runtime_error {
public:
    explicit ClavisException(const std::string& message)
        : std::runtime_error(message) {}
};

/**
 * @brief Exception thrown for errors parsing the input template JSON.
 */
class ClavisParseException : public ClavisException {
public:
    explicit ClavisParseException(const std::string& message)
        : ClavisException("Parse Error: " + message) {}
};

/**
 * @brief Exception thrown when a cyclical dependency is detected during substitution.
 */
class ClavisCycleException : public ClavisException {
private:
    std::string cycle_path_;
public:
    ClavisCycleException(const std::string& message, std::string cycle_path)
        : ClavisException("Cycle Error: " + message), cycle_path_(std::move(cycle_path)) {}

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
class ClavisMissingKeyException : public ClavisException {
private:
    std::string key_path_;
public:
    ClavisMissingKeyException(const std::string& message, std::string key_path)
        : ClavisException("Missing Key Error: " + message), key_path_(std::move(key_path)) {}

    /**
     * @brief Get the key or path that was not found.
     */
    const std::string& get_key_path() const noexcept { return key_path_; }

    // Override what() to potentially include the path
    const char* what() const noexcept override;
};

} // namespace clavis

#endif // CLAVIS_EXCEPTIONS_HPP
