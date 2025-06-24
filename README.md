# Permuto

[![Build Status](https://img.shields.io/badge/build-passing-brightgreen.svg)](.) [![Tests](https://img.shields.io/badge/tests-49%2F49-brightgreen.svg)](.) 

Permuto is a lightweight C++ library for JSON template processing that enables declarative transformation of data by substituting variables from a context object into JSON templates.

## Features

- **Simple Template Processing**: Use `${path}` syntax to substitute values from context data
- **Type Preservation**: Maintains JSON data types (numbers, booleans, objects, arrays)
- **String Interpolation**: Optional support for placeholders within strings
- **Reverse Operations**: Reconstruct original context from processed templates
- **Safety Features**: Cycle detection, recursion limits, comprehensive error handling
- **Modern C++**: Written in C++17 with clean, modern idioms
- **Minimal Dependencies**: Only requires nlohmann/json library
- **Cross-Platform**: Works on Linux, Windows, and macOS

## Quick Start

### Installation

Using CMake FetchContent:
```cmake
include(FetchContent)
FetchContent_Declare(
    Permuto
    GIT_REPOSITORY https://github.com/yourusername/permuto.git
    GIT_TAG main
)
FetchContent_MakeAvailable(Permuto)

target_link_libraries(your_target PRIVATE Permuto::permuto)
```

### Basic Usage

```cpp
#include <permuto/permuto.hpp>
#include <iostream>

int main() {
    // Template with placeholders
    nlohmann::json template_json = R"({
        "user_id": "${/user/id}",
        "greeting": "Hello ${/user/name}!",
        "settings": "${/preferences}"
    })"_json;
    
    // Context data
    nlohmann::json context = R"({
        "user": {
            "id": 123,
            "name": "Alice"
        },
        "preferences": {
            "theme": "dark",
            "notifications": true
        }
    })"_json;
    
    // Process template
    permuto::Options opts;
    opts.enable_interpolation = true;  // Enable string interpolation
    
    auto result = permuto::apply(template_json, context, opts);
    std::cout << result.dump(2) << std::endl;
    
    return 0;
}
```

Output:
```json
{
  "user_id": 123,
  "greeting": "Hello Alice!",
  "settings": {
    "theme": "dark",
    "notifications": true
  }
}
```

## API Reference

### Core Functions

#### `apply(template_json, context, options)`
Process a template with the given context.

**Parameters:**
- `template_json`: JSON template containing placeholders
- `context`: JSON object with data to substitute
- `options`: Processing options (optional)

**Returns:** Processed JSON with substituted values

#### `create_reverse_template(template_json, options)`
Create a reverse template for extracting context from results.

**Parameters:**
- `template_json`: Original JSON template
- `options`: Processing options (interpolation must be disabled)

**Returns:** Reverse template mapping

#### `apply_reverse(reverse_template, result_json)`
Extract original context from processed result using reverse template.

**Parameters:**
- `reverse_template`: Reverse template from `create_reverse_template`
- `result_json`: Processed JSON result

**Returns:** Reconstructed context object

### Options

```cpp
struct Options {
    std::string start_marker = "${";     // Placeholder start marker
    std::string end_marker = "}";        // Placeholder end marker
    bool enable_interpolation = false;   // Enable string interpolation
    MissingKeyBehavior missing_key_behavior = MissingKeyBehavior::Ignore;
    size_t max_recursion_depth = 64;     // Maximum nesting depth
};
```

### Path Syntax

Permuto uses JSON Pointer (RFC 6901) syntax for paths:

- `/user/name` - Access object property
- `/items/0` - Access array element
- `/user/settings/theme` - Nested object access
- `/special~0key` - Escape `~` as `~0`
- `/key~1with~1slashes` - Escape `/` as `~1`

### Error Handling

Permuto provides structured exception hierarchy:

- `PermutoException` - Base exception class
- `MissingKeyException` - Missing key in context
- `CycleException` - Infinite recursion detected
- `RecursionLimitException` - Maximum depth exceeded
- `InvalidTemplateException` - Malformed template

## Advanced Usage

### String Interpolation

```cpp
permuto::Options opts;
opts.enable_interpolation = true;

nlohmann::json template_json = R"({
    "message": "Welcome ${/user/name}! You have ${/notifications} messages."
})"_json;
```

### Custom Delimiters

```cpp
permuto::Options opts;
opts.start_marker = "{{";
opts.end_marker = "}}";

nlohmann::json template_json = R"({
    "value": "{{/data/field}}"
})"_json;
```

### Reverse Operations

```cpp
// Forward processing
auto result = permuto::apply(template_json, context);

// Create reverse template
auto reverse_template = permuto::create_reverse_template(template_json);

// Reconstruct original context
auto reconstructed = permuto::apply_reverse(reverse_template, result);

// Perfect round-trip: context == reconstructed
assert(context == reconstructed);
```

## Command Line Tool

Permuto includes a CLI tool for testing and demonstration:

```bash
# Basic usage
permuto template.json context.json

# With string interpolation
permuto --interpolation template.json context.json

# Reverse operation
permuto --reverse template.json result.json

# Custom options
permuto --missing-key=error --max-depth=32 template.json context.json
```

### CLI Options

- `--help` - Show help message
- `--version` - Show version information
- `--reverse` - Perform reverse operation
- `--interpolation` / `--no-interpolation` - Control string interpolation
- `--missing-key=MODE` - Set missing key behavior (`ignore` or `error`)
- `--start=MARKER` - Set custom start marker
- `--end=MARKER` - Set custom end marker
- `--max-depth=N` - Set maximum recursion depth

## Building from Source

### Prerequisites

- C++17 compatible compiler (GCC, Clang, MSVC)
- CMake 3.15 or later
- nlohmann/json library
- Google Test (for tests)

### Build Instructions

```bash
# Clone repository
git clone https://github.com/yourusername/permuto.git
cd permuto

# Configure build
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build library and CLI
cmake --build build

# Run tests
cd build && ctest

# Install (optional)
cmake --install build
```

### Build Options

- `PERMUTO_BUILD_TESTS` - Build test suite (default: ON)
- `CMAKE_BUILD_TYPE` - Build type (Debug, Release, RelWithDebInfo)

## Examples

### API Payload Generation

```cpp
// Template for different LLM providers
nlohmann::json openai_template = R"({
    "model": "${/config/model}",
    "messages": [{"role": "user", "content": "${/prompt}"}],
    "max_tokens": "${/config/max_tokens}",
    "temperature": "${/config/temperature}"
})"_json;

nlohmann::json anthropic_template = R"({
    "model": "${/config/model}",
    "prompt": "Human: ${/prompt}\n\nAssistant:",
    "max_tokens_to_sample": "${/config/max_tokens}"
})"_json;

// Shared context
nlohmann::json context = R"({
    "config": {"model": "gpt-4", "max_tokens": 1000, "temperature": 0.7},
    "prompt": "Explain quantum computing"
})"_json;

// Generate different API payloads
auto openai_payload = permuto::apply(openai_template, context);
auto anthropic_payload = permuto::apply(anthropic_template, context);
```

### Configuration Templates

```cpp
nlohmann::json config_template = R"({
    "database": {
        "host": "${/env/DB_HOST}",
        "port": "${/env/DB_PORT}",
        "name": "${/app/database_name}"
    },
    "redis": {
        "url": "redis://${/env/REDIS_HOST}:${/env/REDIS_PORT}"
    },
    "logging": {
        "level": "${/app/log_level}",
        "file": "/logs/${/app/name}.log"
    }
})"_json;
```

## Performance

Permuto is designed for efficiency:

- **Linear complexity**: O(n) where n is template size
- **Minimal allocations**: Reuses memory where possible
- **Fast processing**: Handles 1000+ placeholders in <10ms
- **Small footprint**: Static library under 1MB

## Contributing

Contributions are welcome! Please read our [Contributing Guide](CONTRIBUTING.md) for details on:

- Code style guidelines
- Testing requirements
- Pull request process
- Development setup

## License

This project is in the public domain - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- [nlohmann/json](https://github.com/nlohmann/json) for excellent JSON library
- [Google Test](https://github.com/google/googletest) for testing framework
- JSON Pointer specification ([RFC 6901](https://tools.ietf.org/html/rfc6901))

## Roadmap

- [ ] YAML template support
- [ ] Template validation tools
- [ ] Performance optimizations
- [ ] Additional output formats
- [ ] Template composition features