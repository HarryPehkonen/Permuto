# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Permuto is a lightweight C++ library for JSON template processing that enables declarative transformation of data by substituting variables from a context object into JSON templates. The project is currently in the **design/planning phase** with comprehensive documentation but no implementation yet.

## Key Project Information

- **Language**: C++17
- **JSON Library**: nlohmann/json (planned dependency)
- **Build System**: CMake (planned)
- **Testing**: Google Test (planned)
- **CLI Tool**: Command-line interface built on library API

## Planned Architecture

### Core Components
- **TemplateProcessor**: Main template processing engine
- **PlaceholderParser**: Handles `${path}` placeholder parsing
- **JsonPointer**: RFC 6901 compliant path resolution
- **ReverseProcessor**: Context reconstruction from processed templates
- **CycleDetector**: Prevents infinite recursion

### API Design
```cpp
// Primary API (from TECHNICAL_DETAILS.md)
nlohmann::json apply(template_json, context, options);
nlohmann::json create_reverse_template(template_json, options);
nlohmann::json apply_reverse(reverse_template, result_json);
```

### Planned Directory Structure
```
libpermuto/
├── include/permuto/
│   └── permuto.hpp              # Single public header
├── src/                         # Internal implementation
├── cli/
│   └── main.cpp                 # Command-line tool
└── tests/                       # Test suite
```

## Development Commands

### Build Commands
```bash
# Configure build (with all features enabled)
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Configure build (disable tests and examples)
cmake -B build -DCMAKE_BUILD_TYPE=Release -DPERMUTO_BUILD_TESTS=OFF -DPERMUTO_BUILD_EXAMPLES=OFF

# Build library, CLI, tests, and examples
cmake --build build

# Run tests
cd build && ctest

# Install library
cmake --install build
```

### Example Usage
```bash
# Run examples (built in ./build/)
./build/api_example
./build/mixed_mode_example  
./build/multi_stage_example

# CLI usage (when implemented)
./build/permuto template.json context.json
```

## Key Features

### Template Processing
- JSON templates with `${path}` placeholder syntax
- JSON Pointer (RFC 6901) path resolution: `/user/name`, `/items/0`
- String interpolation support: `"Hello ${/user/name}!"`
- Configurable placeholder delimiters

### Safety Features  
- Cycle detection for infinite recursion
- Recursion depth limiting
- Comprehensive error handling with structured exceptions

### Reverse Operations
- Generate reverse templates from forward templates
- Reconstruct original context from processed results
- Round-trip guarantee for data integrity

## Documentation Structure

The **README.md file is designed for AI/LLM consumption** and provides comprehensive technical reference. For human-readable documentation and tutorials, refer users to:

- **Online Book**: https://harrypehkonen.github.io/ComputoPermutoBook/
- **Book Repository**: https://github.com/HarryPehkonen/ComputoPermutoBook

## Current Status

The project is **fully implemented** with comprehensive thread-safe JSON template processing capabilities. All core features are complete including reverse operations, cycle detection, and extensive testing (65 tests passing).

## Implementation Plan

The project is divided into 7 phases, tracked in the todo system:

### Phase 1: Project Setup (HIGH PRIORITY)
- Create CMakeLists.txt with modern CMake practices
- Set up directory structure (include/, src/, cli/, tests/)
- Configure nlohmann/json dependency
- Basic build system validation

### Phase 2: Core Infrastructure (HIGH PRIORITY)  
- Implement JsonPointer class for RFC 6901 path resolution
- Create TemplateProcessor class for basic template processing
- Build PlaceholderParser for `${path}` syntax parsing
- Core apply() function implementation

### Phase 3: Safety Features (HIGH PRIORITY)
- Add CycleDetector to prevent infinite recursion
- Implement recursion depth limiting
- Create exception hierarchy (PermutoException, CycleException, etc.)
- Comprehensive error handling and validation

### Phase 4: Reverse Operations (MEDIUM PRIORITY)
- Implement create_reverse_template() function
- Build ReverseProcessor for context reconstruction
- Add apply_reverse() functionality
- Round-trip testing validation

### Phase 5: CLI Tool (MEDIUM PRIORITY)
- Create command-line interface in cli/main.cpp
- Support all library options via CLI flags
- Add file I/O and pretty-printing
- Error reporting and help system

### Phase 6: Testing (MEDIUM PRIORITY)
- Unit tests for all core components
- Integration tests for full workflows
- Performance tests for large templates
- Round-trip testing for reverse operations
- Error condition coverage

### Phase 7: Documentation (LOW PRIORITY)
- API documentation with examples
- Usage guide and tutorials
- README with quick start
- Build and integration instructions

**Status Tracking**: Progress is maintained in the todo system. Each phase is marked as pending/in_progress/completed to enable resuming work across sessions.