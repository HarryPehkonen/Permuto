# Permuto Technical Design

## Project Products

This project produces two primary deliverables:

1. **libpermuto** - A C++ library for JSON template processing that can be integrated into other projects
2. **permuto** - A command-line tool for testing and demonstrating all library functionality

## Library Design Goals

### Single Header Interface
- **One include file**: `#include <permuto/permuto.hpp>` provides complete API
- **Minimal dependencies**: Only requires nlohmann/json in addition to standard library
- **Clean integration**: Drop libpermuto into any CMake project with minimal configuration

### Library Interface (permuto/permuto.hpp)

```cpp
#pragma once
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <optional>
#include <stdexcept>

namespace permuto {
    // Configuration
    enum class MissingKeyBehavior {
        Ignore,  // Leave placeholder as-is (default)
        Error,   // Throw exception
        Remove   // Remove the containing key/element
    };

    struct Options {
        std::string start_marker = "${";
        std::string end_marker = "}";
        bool enable_interpolation = false;
        MissingKeyBehavior missing_key_behavior = MissingKeyBehavior::Ignore;
        size_t max_recursion_depth = 64;
        
        void validate() const;  // Throws std::invalid_argument if invalid
    };

    // Exception hierarchy
    class PermutoException : public std::runtime_error {
    public:
        explicit PermutoException(const std::string& message);
    };

    class CycleException : public PermutoException {
        std::vector<std::string> cycle_path_;
    public:
        CycleException(const std::string& message, std::vector<std::string> cycle_path);
        const std::vector<std::string>& cycle_path() const;
    };

    class MissingKeyException : public PermutoException {
        std::string key_path_;
    public:
        MissingKeyException(const std::string& message, std::string key_path);
        const std::string& key_path() const;
    };

    class InvalidTemplateException : public PermutoException {
    public:
        explicit InvalidTemplateException(const std::string& message);
    };

    class RecursionLimitException : public PermutoException {
        size_t depth_;
    public:
        RecursionLimitException(const std::string& message, size_t depth);
        size_t depth() const;
    };

    // Primary API functions
    nlohmann::json apply(
        const nlohmann::json& template_json,
        const nlohmann::json& context,
        const Options& options = {}
    );
    
    nlohmann::json create_reverse_template(
        const nlohmann::json& template_json,
        const Options& options = {}
    );
    
    nlohmann::json apply_reverse(
        const nlohmann::json& reverse_template,
        const nlohmann::json& result_json
    );
}
```

## Architecture Overview

### Core Design Principles

1. **Single Responsibility**: Each internal class has one clear purpose
2. **Library-First Design**: All functionality accessible through clean C++ API
3. **Immutable Operations**: Template processing doesn't modify inputs
4. **Fail Fast**: Validate inputs early and provide clear error messages
5. **Resource Safety**: RAII for all resource management

## Internal Module Structure

```
libpermuto/
├── include/permuto/
│   └── permuto.hpp              # Single public header
├── src/
│   ├── template_processor.hpp   # Core template processing (internal)
│   ├── template_processor.cpp
│   ├── placeholder_parser.hpp   # Placeholder parsing (internal)
│   ├── placeholder_parser.cpp
│   ├── json_pointer.hpp         # JSON Pointer utilities (internal)
│   ├── json_pointer.cpp
│   ├── reverse_processor.hpp    # Reverse operations (internal)
│   ├── reverse_processor.cpp
│   ├── cycle_detector.hpp       # Cycle detection (internal)
│   └── cycle_detector.cpp
├── cli/
│   └── main.cpp                 # Command-line tool
└── tests/
    ├── test_api.cpp             # Public API tests
    ├── test_template_processing.cpp
    ├── test_reverse_operations.cpp
    ├── test_json_pointer.cpp
    ├── test_error_handling.cpp
    └── integration_tests.cpp    # CLI integration tests
```

## Core Internal Classes

### 1. Template Processor (Internal)

```cpp
// src/template_processor.hpp
namespace permuto::internal {
    class TemplateProcessor {
        Options options_;
        std::unique_ptr<PlaceholderParser> parser_;
        std::unique_ptr<CycleDetector> cycle_detector_;
        
    public:
        explicit TemplateProcessor(const Options& options);
        
        nlohmann::json process(
            const nlohmann::json& template_json,
            const nlohmann::json& context
        );
        
    private:
        nlohmann::json process_node(
            const nlohmann::json& node,
            const nlohmann::json& context,
            size_t depth = 0
        );
        
        nlohmann::json process_string(
            const std::string& template_str,
            const nlohmann::json& context,
            size_t depth
        );
        
        nlohmann::json resolve_placeholder(
            const std::string& path,
            const nlohmann::json& context,
            size_t depth
        );
    };
}
```

### 2. JSON Pointer Utilities (Internal)

```cpp
// src/json_pointer.hpp
namespace permuto::internal {
    class JsonPointer {
        std::string pointer_;
        std::vector<std::string> segments_;
        
    public:
        explicit JsonPointer(const std::string& pointer);
        
        // RFC 6901 compliance
        static std::string escape_segment(const std::string& segment);
        static std::string unescape_segment(const std::string& segment);
        
        // Validation
        bool is_valid() const;
        static bool is_valid_pointer(const std::string& pointer);
        
        // Access
        const std::string& as_string() const { return pointer_; }
        const std::vector<std::string>& segments() const { return segments_; }
        bool is_root() const { return pointer_.empty(); }
        
        // Lookup operations
        std::optional<std::reference_wrapper<const nlohmann::json>> 
            resolve(const nlohmann::json& context) const;
    };
}
```

### 3. Placeholder Parser (Internal)

```cpp
// src/placeholder_parser.hpp
namespace permuto::internal {
    struct Placeholder {
        std::string path;           // The JSON Pointer path
        std::string full_text;      // Complete placeholder text
        size_t start_pos;          // Position in original string
        size_t end_pos;            // End position in original string
        bool is_exact_match;       // True if placeholder is entire string
    };

    class PlaceholderParser {
        std::string start_marker_;
        std::string end_marker_;
        
    public:
        PlaceholderParser(const std::string& start, const std::string& end);
        
        // Check if string is exactly one placeholder
        bool is_exact_placeholder(const std::string& str) const;
        
        // Extract single placeholder from exact match
        std::optional<Placeholder> extract_exact_placeholder(const std::string& str) const;
        
        // Find all placeholders in string (for interpolation)
        std::vector<Placeholder> find_all_placeholders(const std::string& str) const;
        
    private:
        std::optional<std::string> extract_path(
            const std::string& str, 
            size_t start, 
            size_t end
        ) const;
    };
}
```

### 4. Cycle Detection (Internal)

```cpp
// src/cycle_detector.hpp
namespace permuto::internal {
    class CycleDetector {
        std::set<std::string> active_paths_;
        std::vector<std::string> path_stack_;
        
    public:
        class PathGuard {
            CycleDetector& detector_;
            std::string path_;
            bool active_;
            
        public:
            PathGuard(CycleDetector& detector, const std::string& path);
            ~PathGuard();
            
            // Non-copyable, movable
            PathGuard(const PathGuard&) = delete;
            PathGuard& operator=(const PathGuard&) = delete;
            PathGuard(PathGuard&& other) noexcept;
            PathGuard& operator=(PathGuard&& other) noexcept;
        };
        
        PathGuard enter_path(const std::string& path);  // Throws CycleException if cycle detected
        
    private:
        void add_path(const std::string& path);
        void remove_path(const std::string& path);
    };
}
```

### 5. Reverse Operations (Internal)

```cpp
// src/reverse_processor.hpp
namespace permuto::internal {
    class ReverseProcessor {
        Options options_;
        std::unique_ptr<PlaceholderParser> parser_;
        
    public:
        explicit ReverseProcessor(const Options& options);
        
        nlohmann::json create_reverse_template(const nlohmann::json& template_json);
        
        nlohmann::json apply_reverse(
            const nlohmann::json& reverse_template,
            const nlohmann::json& result_json
        );
        
    private:
        void scan_template_recursive(
            const nlohmann::json& node,
            const std::string& current_result_pointer,
            nlohmann::json& reverse_template
        );
        
        void reconstruct_recursive(
            const nlohmann::json& reverse_node,
            const nlohmann::json& result_json,
            nlohmann::json& context_node
        );
        
        void insert_context_mapping(
            nlohmann::json& reverse_template,
            const std::string& context_path,
            const std::string& result_pointer
        );
    };
}
```

## Public API Implementation

The three public functions in `permuto.hpp` are implemented as thin wrappers around the internal classes:

```cpp
// In permuto.cpp
namespace permuto {
    nlohmann::json apply(
        const nlohmann::json& template_json,
        const nlohmann::json& context,
        const Options& options
    ) {
        options.validate();
        internal::TemplateProcessor processor(options);
        return processor.process(template_json, context);
    }
    
    nlohmann::json create_reverse_template(
        const nlohmann::json& template_json,
        const Options& options
    ) {
        options.validate();
        if (options.enable_interpolation) {
            throw std::invalid_argument("Reverse templates require interpolation to be disabled");
        }
        internal::ReverseProcessor processor(options);
        return processor.create_reverse_template(template_json);
    }
    
    nlohmann::json apply_reverse(
        const nlohmann::json& reverse_template,
        const nlohmann::json& result_json
    ) {
        internal::ReverseProcessor processor({});  // Default options for reverse
        return processor.apply_reverse(reverse_template, result_json);
    }
}
```

## Command-Line Tool Design

### CLI Interface
```bash
# Forward operation (default)
permuto template.json context.json [options]

# Reverse operations
permuto --create-reverse template.json [options]    # Create reverse template
permuto --apply-reverse reverse.json result.json    # Apply reverse template

# Options
--interpolation / --no-interpolation  # Enable/disable string interpolation (default: disabled)
--missing-key=ignore|error           # Missing key behavior (default: ignore)  
--start=MARKER                       # Custom start delimiter (default: ${)
--end=MARKER                         # Custom end delimiter (default: })
--help                               # Show usage
--version                           # Show version
```

### CLI Implementation
```cpp
// cli/main.cpp
int main(int argc, char* argv[]) {
    try {
        // Parse command line arguments
        CLI::Config config = CLI::parse_args(argc, argv);
        
        if (config.show_help) {
            CLI::print_usage();
            return 0;
        }
        
        if (config.show_version) {
            CLI::print_version();
            return 0;
        }
        
        // Load JSON files
        auto template_json = CLI::load_json(config.template_file);
        auto context_json = CLI::load_json(config.context_file);
        
        // Configure options
        permuto::Options options;
        options.enable_interpolation = config.enable_interpolation;
        options.missing_key_behavior = config.missing_key_behavior;
        options.start_marker = config.start_marker;
        options.end_marker = config.end_marker;
        
        // Execute operation
        nlohmann::json result;
        switch (config.operation) {
            case CLI::Operation::Apply:
                result = permuto::apply(template_json, context_json, options);
                break;
            case CLI::Operation::CreateReverse:
                result = permuto::create_reverse_template(template_json, options);
                break;
            case CLI::Operation::ApplyReverse:
                result = permuto::apply_reverse(template_json, context_json);
                break;
        }
        
        // Output result
        std::cout << result.dump(4) << std::endl;
        
    } catch (const permuto::PermutoException& e) {
        std::cerr << "Permuto error: " << e.what() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
```

## Build System Design

### CMake Structure
```cmake
# CMakeLists.txt
cmake_minimum_required(VERSION 3.15)
project(permuto VERSION 1.0.0 LANGUAGES CXX)

# Options
option(PERMUTO_BUILD_TESTS "Build tests" ON)
option(PERMUTO_BUILD_CLI "Build command-line tool" ON)

# Library target
add_library(permuto STATIC
    src/template_processor.cpp
    src/placeholder_parser.cpp
    src/json_pointer.cpp
    src/reverse_processor.cpp
    src/cycle_detector.cpp
)

target_include_directories(permuto 
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        src
)

target_compile_features(permuto PUBLIC cxx_std_17)

# Dependencies
find_package(nlohmann_json REQUIRED)
target_link_libraries(permuto PUBLIC nlohmann_json::nlohmann_json)

# CLI tool
if(PERMUTO_BUILD_CLI)
    add_executable(permuto-cli cli/main.cpp)
    target_link_libraries(permuto-cli PRIVATE permuto)
    set_target_properties(permuto-cli PROPERTIES OUTPUT_NAME permuto)
endif()

# Tests
if(PERMUTO_BUILD_TESTS)
    find_package(GTest REQUIRED)
    enable_testing()
    
    add_executable(permuto-tests
        tests/test_api.cpp
        tests/test_template_processing.cpp
        tests/test_reverse_operations.cpp
        tests/test_json_pointer.cpp
        tests/test_error_handling.cpp
        tests/integration_tests.cpp
    )
    
    target_link_libraries(permuto-tests PRIVATE permuto GTest::gtest_main)
    
    add_test(NAME permuto_unit_tests COMMAND permuto-tests)
endif()

# Installation
include(GNUInstallDirs)

install(TARGETS permuto
    EXPORT permuto-targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY include/permuto
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

if(PERMUTO_BUILD_CLI)
    install(TARGETS permuto-cli
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()

# Package configuration
include(CMakePackageConfigHelpers)

configure_package_config_file(
    cmake/permuto-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/permuto-config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/permuto
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/permuto-config.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/permuto
)

install(EXPORT permuto-targets
    FILE permuto-targets.cmake
    NAMESPACE permuto::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/permuto
)
```

## Usage Examples

### Library Integration
```cmake
# In another project's CMakeLists.txt
find_package(permuto REQUIRED)
target_link_libraries(my-app PRIVATE permuto::permuto)
```

```cpp
// In application code
#include <permuto/permuto.hpp>
#include <nlohmann/json.hpp>

int main() {
    nlohmann::json template_json = /* load template */;
    nlohmann::json context = /* load context */;
    
    permuto::Options options;
    options.enable_interpolation = true;
    
    auto result = permuto::apply(template_json, context, options);
    
    return 0;
}
```

### Command-Line Usage
```bash
# Basic forward operation
./permuto template.json context.json --interpolation

# Create reverse template
./permuto --create-reverse template.json --no-interpolation > reverse.json

# Apply reverse template
./permuto --apply-reverse reverse.json result.json > reconstructed.json
```

This design provides a clean, single-header library interface while maintaining excellent internal architecture and comprehensive CLI functionality for testing and demonstration.

## Remove Mode Feature

### Overview

The `Remove` mode is a specialized `MissingKeyBehavior` that automatically removes keys or array elements when their placeholders cannot be resolved from the context. This feature is particularly useful for generating clean API requests where optional parameters should be omitted entirely rather than set to null values.

### Behavior

When `missing_key_behavior = MissingKeyBehavior::Remove`:

- **Object Keys**: Missing placeholders cause the entire key-value pair to be removed from the resulting object
- **Array Elements**: Missing placeholders cause the array element to be removed, with remaining elements shifting positions
- **Nested Structures**: Removal applies locally - only the immediate containing unit is affected

### Constraints and Limitations

1. **String Interpolation Incompatibility**: Remove mode cannot be used with `enable_interpolation = true`. This limitation exists because partial string substitution would be ambiguous.

2. **Root-Level Templates**: Remove mode cannot be applied to root-level placeholders (e.g., `"${/some/path}"` as the entire template) since there is no containing structure to remove from.

3. **Reverse Operations**: Templates processed with Remove mode cannot be used with reverse operations (`create_reverse_template`, `apply_reverse`) because information about removed keys is lost, making round-trip reconstruction impossible.

### Usage Examples

#### LLM API Request (Primary Use Case)
```cpp
nlohmann::json api_template = R"({
    "model": "${/config/model}",
    "temperature": "${/config/temperature}",
    "max_tokens": "${/config/max_tokens}",
    "top_p": "${/config/top_p}"
})"_json;

nlohmann::json context = R"({
    "config": {
        "model": "gpt-4",
        "temperature": 0.7
    }
})"_json;

Options opts;
opts.missing_key_behavior = MissingKeyBehavior::Remove;

auto result = permuto::apply(api_template, context, opts);
// Result: {"model": "gpt-4", "temperature": 0.7}
// Note: max_tokens and top_p are completely omitted
```

#### Array Element Removal
```cpp
nlohmann::json middleware_template = R"({
    "pipeline": [
        "validate_input",
        "${/middleware/auth}",
        "${/middleware/rate_limiter}",
        "process_request"
    ]
})"_json;

nlohmann::json context = R"({
    "middleware": {
        "auth": "jwt_middleware"
    }
})"_json;

// Result: {"pipeline": ["validate_input", "jwt_middleware", "process_request"]}
// Note: rate_limiter element is removed, array indices shift
```

#### Mixed Mode with Different Markers
```cpp
// Use different placeholder markers for different behaviors
nlohmann::json mixed_template = R"({
    "required_field": "<</user/id>>",     // Error mode
    "optional_field": "${/user/email}"    // Remove mode
})"_json;

// Process in two passes:
// 1. Remove mode for ${} markers
// 2. Error mode for <<>> markers
Options remove_opts;
remove_opts.missing_key_behavior = MissingKeyBehavior::Remove;
remove_opts.start_marker = "${";
remove_opts.end_marker = "}";

auto step1 = permuto::apply(mixed_template, context, remove_opts);

Options error_opts;
error_opts.missing_key_behavior = MissingKeyBehavior::Error;
error_opts.start_marker = "<<";
error_opts.end_marker = ">>";

auto final_result = permuto::apply(step1, context, error_opts);
```

### CLI Support

```bash
# Use Remove mode via command line
./permuto template.json context.json --missing-key=remove

# Cannot combine with interpolation (will error)
./permuto template.json context.json --missing-key=remove --interpolation  # ERROR

# Mixed mode example using different markers
./permuto template.json context.json --missing-key=remove --start='${' --end='}'
./permuto intermediate.json context.json --missing-key=error --start='<<' --end='>>'
```

### Implementation Notes

The Remove mode is implemented at the processing level:
- `TemplateProcessor::process_object()` skips key-value pairs with unresolved placeholders
- `TemplateProcessor::process_array()` skips array elements with unresolved placeholders
- Validation occurs in `Options::validate()` and `permuto::apply()` to prevent incompatible configurations

This feature provides fine-grained control over JSON structure generation while maintaining the library's core design principles of safety and predictability.