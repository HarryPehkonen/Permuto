# Permuto Requirements

## Project Overview

Permuto is a lightweight C++ library for JSON template processing. It enables declarative transformation of data by substituting variables from a context object into JSON templates. The primary use case is generating API payloads dynamically without hardcoding JSON structures in application code.

## Core Value Proposition

**Problem**: Applications often need to generate different JSON structures for various APIs (e.g., different LLM providers) using the same underlying data.

**Traditional Solution**: Write code for each API that programmatically builds JSON and performs data lookups.

**Permuto Solution**: Define the target JSON structure declaratively in template files using placeholder syntax, then apply data context at runtime.

## Functional Requirements

### 1. Template Processing (Core Feature)

**FR-1.1**: Process JSON templates containing variable placeholders
- Templates must be valid JSON documents
- Placeholders use `${path}` syntax by default
- Support exact-match substitution (entire string value is a placeholder)
- Preserve JSON data types (numbers, booleans, arrays, objects, null)

**FR-1.2**: Context data lookup
- Support nested data access using JSON Pointer (RFC 6901) notation
- Handle missing paths gracefully (configurable behavior)
- Support array indexing

**FR-1.3**: String interpolation (optional mode)
- Enable placeholders within larger strings: `"Hello ${/user/name}!"`
- Convert non-string values to appropriate string representation
- Configurable via options

### 2. Reverse Operations

**FR-2.1**: Reverse template generation
- Analyze forward template to create reverse mapping
- Map context paths to result JSON locations (JSON Pointers)
- Only supported when interpolation is disabled

**FR-2.2**: Context reconstruction
- Extract original context data from result JSON using reverse template
- Preserve original data types and structure
- Round-trip guarantee: apply(template, context) -> result, then reverse(template, result) -> context

### 3. Path Resolution

**FR-3.1**: JSON Pointer syntax (RFC 6901) exclusively
- Path format: `/user/name`, `/items/0`, `/user~1role` (for keys with slashes)
- Handle special characters in keys (proper escaping: `~0` for `~`, `~1` for `/`)
- Empty path `${}` refers to root context
- Array indexing: `/items/0`, `/matrix/1/2`

### 4. Configuration and Options

**FR-4.1**: Customizable delimiters
- Allow custom start/end markers (default `${` and `}`)
- Validation of delimiter uniqueness and non-empty values

**FR-4.2**: Missing key behavior
- Ignore: Leave placeholder as-is (default)
- Error: Throw exception for missing paths

**FR-4.3**: Interpolation mode
- Exact-match only (default): Only process `"${path}"` placeholders
- Full interpolation: Process placeholders within strings

### 5. Safety and Robustness

**FR-5.1**: Cycle detection
- Detect infinite recursion in template variables
- Throw meaningful exception with cycle information

**FR-5.2**: Recursion depth limiting
- Configurable maximum depth (default: reasonable limit)
- Prevent stack overflow from deep but legitimate recursion

**FR-5.3**: Error handling
- Clear, actionable error messages
- Structured exception hierarchy
- Preserve context information in errors

### 6. Library Interface

**FR-6.1**: Simple C++ API
- Primary functions: `apply(template_json, context, options)`
- Reverse functions: `create_reverse_template(template, options)`, `apply_reverse(reverse_template, result)`
- Header-only or simple static library
- Modern C++ idioms (RAII, const-correctness)

**FR-6.2**: CMake integration
- Standard CMake targets for easy consumption
- Dependency management (find_package or FetchContent)

### 7. Command-Line Tool

**FR-7.1**: CLI demonstration tool
- Basic usage: `permuto template.json context.json`
- Reverse operations: `permuto --reverse template.json result.json`
- Support all library options via command-line flags
- Pretty-printed JSON output
- Clear error reporting

**FR-7.2**: CLI options
- `--interpolation` / `--no-interpolation`
- `--missing-key=ignore|error`
- `--start=marker` and `--end=marker`
- `--reverse` for reverse operations
- `--help` and `--version`

### 8. Testing Requirements

**FR-8.1**: Comprehensive test coverage
- Unit tests for all core functionality
- Round-trip testing for reverse operations
- Integration tests for CLI tool
- Performance tests for reasonable-sized templates
- Error condition testing

**FR-8.2**: Example documentation
- Working code examples in documentation
- Template/context pairs demonstrating features
- Reverse operation examples
- Common use case examples

## Non-Functional Requirements

### Performance
- Handle templates with hundreds of placeholders efficiently
- Minimal memory allocation/deallocation during processing
- O(n) complexity where n is template size

### Maintainability
- Clean separation of concerns
- Single responsibility classes
- Minimal interdependencies
- Self-documenting code with clear naming

### Portability
- C++17 standard compliance
- Cross-platform (Linux, Windows, macOS)
- Major compiler support (GCC, Clang, MSVC)

### Dependencies
- Minimize external dependencies
- JSON library dependency is acceptable (nlohmann/json recommended)
- Testing framework acceptable for test code only

## Success Criteria

1. **Developer Experience**: New users can process their first template in under 5 minutes
2. **Performance**: Process 1000-placeholder template in under 10ms on modern hardware
3. **Reliability**: Zero crashes or undefined behavior on malformed input
4. **Maintainability**: New features can be added without modifying existing core classes
5. **Integration**: Library can be added to existing C++ projects with minimal friction
6. **Round-trip Integrity**: Perfect reconstruction of context data through reverse operations

## Example Usage

### Forward Operation
```cpp
#include <permuto/permuto.hpp>

// Template (could be loaded from file)
json template_data = R"({
  "user_id": "${/user/id}",
  "message": "Hello ${/user/name}!",
  "settings": "${/preferences}"
})"_json;

// Context data
json context = R"({
  "user": {"id": 123, "name": "Alice"},
  "preferences": {"theme": "dark", "notifications": true}
})"_json;

// Apply template
permuto::Options opts;
opts.enable_interpolation = true;
json result = permuto::apply(template_data, context, opts);
```

### Reverse Operation
```cpp
// Create reverse template (interpolation must be disabled)
permuto::Options reverse_opts;
reverse_opts.enable_interpolation = false;

json result = permuto::apply(template_data, context, reverse_opts);
json reverse_template = permuto::create_reverse_template(template_data, reverse_opts);
json reconstructed = permuto::apply_reverse(reverse_template, result);

// reconstructed == context (perfect round-trip)
```