// permuto/src/exceptions.cpp
#include "permuto/exceptions.hpp" // Use quotes for local includes
#include <string>

namespace permuto {

// Using thread_local for buffer might be overkill, but safer than static if used across threads.
// However, since these exceptions are typically constructed and immediately thrown/caught,
// simple member storage and returning c_str() might suffice. Let's keep it simple.
// Revisit if profiling shows issues or complex exception usage scenarios arise.

// Store the full message generated at construction time.
thread_local std::string tls_exception_buffer;

const char* PermutoCycleException::what() const noexcept {
    try {
        // Simple concatenation for now. Could be more sophisticated.
        tls_exception_buffer = std::runtime_error::what(); // Get base message
        tls_exception_buffer += " Cycle: [";
        tls_exception_buffer += get_cycle_path();
        tls_exception_buffer += "]";
        return tls_exception_buffer.c_str();
    } catch (...) {
        // std::terminate or return base message if buffer allocation fails
        return std::runtime_error::what();
    }
}

const char* PermutoMissingKeyException::what() const noexcept {
     try {
        tls_exception_buffer = std::runtime_error::what(); // Get base message
        tls_exception_buffer += " Path: [";
        tls_exception_buffer += get_key_path();
        tls_exception_buffer += "]";
        return tls_exception_buffer.c_str();
    } catch (...) {
        return std::runtime_error::what();
    }
}

const char* PermutoRecursionDepthException::what() const noexcept {
    try {
        tls_exception_buffer = std::runtime_error::what(); // Get base message
        tls_exception_buffer += " Depth: [";
        tls_exception_buffer += std::to_string(get_current_depth());
        tls_exception_buffer += "/";
        tls_exception_buffer += std::to_string(get_max_depth());
        tls_exception_buffer += "]";
        return tls_exception_buffer.c_str();
    } catch (...) {
        return std::runtime_error::what();
    }
}

} // namespace permuto
