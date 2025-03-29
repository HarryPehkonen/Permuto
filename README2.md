# Clavis (C++ Implementation)

Clavis is a lightweight, flexible C++ library for generating JSON structures by substituting variables from a data context into a valid JSON template. It supports accessing nested data using dot notation and handles type conversions intelligently.

This C++ implementation uses the popular [nlohmann/json](https://github.com/nlohmann/json) library and is built using CMake.

Inspired by the need to dynamically create JSON payloads, such as those required for various LLM (Large Language Model) APIs, Clavis provides a focused templating mechanism. It is *not* a full programming language but excels at structured JSON generation based on input data.

## Features

*   **Valid JSON Templates:** Uses standard, parseable JSON (via nlohmann/json) as the template input format.
*   **Variable Substitution:** Replaces placeholders (e.g., `${variable}`) with values from a provided data context.
*   **Nested Data Access (Dot Notation):** Access values deep within the data context using dot notation (e.g., `${user.profile.email}`). Array indexing is not supported.
*   **Automatic Type Handling:** Intelligently handles data types. When a placeholder like `"${variable}"` or `"${path.to.variable}"` is the *entire* string value in the template, and the corresponding data is a number, boolean, array, object, or null, the quotes are effectively removed in the output, preserving the correct JSON type. String data results in a standard JSON string output.
*   **String Interpolation:** Substitutes variables (including nested ones) within larger strings (e.g., `"Hello, ${user.name}!"`). Non-string values are converted to their compact JSON string representation (`null`, `true`/`false`, numbers, `"[1,2]"`, `{"k":"v"}`).
*   **Recursive Substitution:** Recursively processes substituted values that themselves contain placeholders.
*   **Cycle Detection:** Automatically detects and prevents infinite recursion loops caused by cyclical references (e.g., `{"a": "${b}", "b": "${a}"}` or involving paths), throwing a `ClavisCycleException` instead.
*   **Configurable Missing Key Handling:** Choose whether to `ignore` missing keys/paths (leaving the placeholder) or `error` out (throwing `ClavisMissingKeyException`). This applies to top-level keys and keys accessed during path traversal.
*   **Customizable Delimiters:** Define custom start (`--start`) and end (`--end`) markers for variables instead of the default `${` and `}`.
*   **Modern C++:** Implemented in C++17, using RAII, smart pointers, and best practices.
*   **CMake Build System:** Uses CMake for building, testing, and installation. Dependencies (nlohmann/json, GoogleTest) are handled via `FetchContent` if not found locally.
*   **Command-Line Tool:** Includes a `clavis` CLI tool for easy testing and usage from the command line.

## Building

You will need:

*   A C++17 compliant compiler (GCC, Clang, MSVC)
*   CMake (version 3.15 or higher recommended)
*   Git (for downloading dependencies via FetchContent if needed)
*   An internet connection (for FetchContent if dependencies aren't installed)

**Steps:**

1.  **Clone the repository:**
    ```bash
    git clone <repository-url> clavis
    cd clavis
    ```

2.  **Configure using CMake:** Create a build directory and run CMake.
    ```bash
    # Create a build directory (standard practice)
    cmake -S . -B build -DCLAVIS_ENABLE_TESTING=ON
    # Options:
    # -S . : Specifies the source directory (where the root CMakeLists.txt is)
    # -B build : Specifies the build directory (can be any name)
    # -DCLAVIS_ENABLE_TESTING=ON : Explicitly enable building tests (default is ON)
    ```
    CMake will automatically try to find nlohmann/json and GoogleTest. If not found, it will download them using `FetchContent`.

3.  **Build the code:**
    ```bash
    cmake --build build
    # To build in parallel (faster):
    # cmake --build build --parallel <number_of_jobs>
    # e.g., cmake --build build --parallel 4
    ```

This will build:

*   The static library `libclavis-lib.a` (or `clavis-lib.lib` on Windows) inside the `build/src/` directory.
*   The command-line executable `clavis` (or `clavis.exe`) inside the `build/cli/` directory.
*   The test executable `clavis_tests` (or `clavis_tests.exe`) inside the `build/tests/` directory (if testing is enabled).

## Testing

The project uses GoogleTest for unit testing.

**Steps:**

1.  **Build the project** with tests enabled (ensure `-DCLAVIS_ENABLE_TESTING=ON` was used during the CMake configuration step).

2.  **Run the tests** using CTest from the *build* directory:
    ```bash
    cd build
    ctest
    # For more detailed output, especially on failure:
    # ctest --output-on-failure
    # Or run the test executable directly:
    # ./tests/clavis_tests
    ```

All tests defined in `tests/clavis_test.cpp` should pass.

## Installation

You can install the library headers and the compiled library file for use in other CMake projects.

**Steps:**

1.  **Build the project** as described above.

2.  **Install using CMake:**
    ```bash
    # From the project root directory:
    cmake --install build
    # Or specify a custom installation prefix (location):
    # cmake --install build --prefix /path/to/custom/install/location
    ```
    By default, this installs files to standard system locations (e.g., `/usr/local` on Linux/macOS, `C:\Program Files` on Windows). Using `--prefix` is recommended for local installations or testing.

**Using Clavis in another CMake project:**

After installing Clavis, you can use `find_package` in your other project's `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.15)
project(MyClavisConsumer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find the installed Clavis package
# Add the install prefix to CMAKE_PREFIX_PATH if you installed locally:
# list(APPEND CMAKE_PREFIX_PATH "/path/to/custom/install/location")
find_package(clavis 0.1.0 REQUIRED) # Specify version if needed

add_executable(my_app main.cpp)

# Link against the imported Clavis library target
target_link_libraries(my_app PRIVATE Clavis::clavis-lib)
```

Your `main.cpp` can then include the headers:

```c++
#include <clavis/clavis.hpp>
#include <clavis/exceptions.hpp>
#include <iostream>

int main() {
    nlohmann::json template_json = R"({"msg": "Hello, ${name}"})"_json;
    nlohmann::json context = R"({"name": "World"})"_json;
    try {
        nlohmann::json result = clavis::process(template_json, context);
        std::cout << result.dump(2) << std::endl;
    } catch (const clavis::ClavisException& e) {
        std::cerr << "Clavis Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
```

## Usage

### Library Usage (C++)

Include the necessary headers and call the `clavis::process` function.

```c++
#include <clavis/clavis.hpp>
#include <clavis/exceptions.hpp>
#include <nlohmann/json.hpp>
#include <iostream>
#include <string>

int main() {
    using json = nlohmann::json;

    json template_json = R"(
        {
            "user_email": "${user.email}",
            "greeting": "Welcome, ${user.name}!",
            "primary_setting": "${settings.primary}",
            "secondary_enabled": "${settings.secondary.enabled}",
            "deep_value": "${a.b.c.d}",
            "maybe_present": "${user.optional_field}"
        }
    )"_json;

    json context = R"(
        {
            "user": {
                "name": "Alice",
                "email": "alice@example.com"
            },
            "settings": {
                "primary": "ThemeA",
                "secondary": {
                    "enabled": true
                }
            },
            "a": {
                "b": {
                    "c": {
                        "d": 12345
                    }
                }
            }
        }
    )"_json;

    try {
        // Configure options (optional)
        clavis::Options options;
        options.onMissingKey = clavis::MissingKeyBehavior::Ignore; // Default
        // options.onMissingKey = clavis::MissingKeyBehavior::Error;
        // options.variableStartMarker = "<<";
        // options.variableEndMarker = ">>";

        json result = clavis::process(template_json, context, options);

        // Output the result (pretty-printed)
        std::cout << result.dump(4) << std::endl;

    } catch (const clavis::ClavisMissingKeyException& e) {
        std::cerr << "Error: Missing Key! " << e.what() << " Path: " << e.get_key_path() << std::endl;
        return 1;
    } catch (const clavis::ClavisCycleException& e) {
        std::cerr << "Error: Cycle Detected! " << e.what() << " Cycle: " << e.get_cycle_path() << std::endl;
        return 1;
    } catch (const clavis::ClavisException& e) { // Catch base Clavis exceptions
        std::cerr << "Clavis Error: " << e.what() << std::endl;
        return 1;
    } catch (const std::exception& e) { // Catch other potential errors (e.g., option validation)
        std::cerr << "Standard Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
```

**Output of the example:**

```json
{
    "deep_value": 12345,
    "greeting": "Welcome, Alice!",
    "maybe_present": "${user.optional_field}",
    "primary_setting": "ThemeA",
    "secondary_enabled": true,
    "user_email": "alice@example.com"
}
```

### Command-Line Tool Usage (`clavis`)

The `clavis` executable provides a command-line interface.

**Synopsis:**

```
clavis <template_file> <context_file> [options]
```

**Arguments:**

*   `<template_file>`: Path to the input JSON template file.
*   `<context_file>`: Path to the input JSON data context file.

**Options:**

*   `--on-missing-key=<value>`: Behavior for missing keys.
    *   `ignore`: (Default) Leave placeholders unresolved.
    *   `error`: Print an error and exit if a key/path is missing.
*   `--start=<string>`: Set the variable start delimiter (Default: `${`).
*   `--end=<string>`: Set the variable end delimiter (Default: `}`).
*   `--help`: Display usage information.

**Example:**

Assume `template.json`:
```json
{
  "message": "Hello, ${name}!",
  "setting": "${config.enabled}"
}
```

Assume `context.json`:
```json
{
  "name": "CLI User",
  "config": { "enabled": true }
}
```

**Command:**
```bash
# Assuming 'clavis' executable is in the PATH or build/cli directory
./build/cli/clavis template.json context.json
```

**Output (Pretty-printed JSON to stdout):**
```json
{
    "message": "Hello, CLI User!",
    "setting": true
}
```

**Example with missing key (error mode):**

Assume `template_missing.json`:
```json
{ "value": "${missing.key}" }
```

**Command:**
```bash
./build/cli/clavis template_missing.json context.json --on-missing-key=error
```

**Output (Error message to stderr, non-zero exit code):**
```
Clavis Error: Missing Key Error: Key or path not found (implementation pending) Path: [missing.key]
```
*(Note: Exact error message details depend on implementation)*

### Key Concepts & Examples

*(This section remains largely the same as your original specification, detailing type handling, interpolation, dot notation, recursion, missing keys, custom delimiters, and cycle detection with examples. Ensure examples align with C++ string conversion rules for interpolation: `null`->"null", `true`->"true", arrays/objects use compact JSON strings.)*

#### 1. Data Type Handling (Exact Match Substitution)
*(Identical table as original)*

#### 2. String Interpolation
*(Example updated to reflect C++ string conversion)*

**Template:**
```json
{
  "message": "User ${user.name} (ID: ${user.id}) is ${status.active}. Count: ${data.count}. Settings: ${settings}"
}
```
**Data:**
```json
{
  "user": {"name": "Alice", "id": 123},
  "status": {"active": true},
  "data": {"count": null},
  "settings": {"theme": "dark", "notify": false}
}
```
**Output:**
```json
{
  "message": "User Alice (ID: 123) is true. Count: null. Settings: {\"notify\":false,\"theme\":\"dark\"}"
}
```
*(Note: Compact JSON string for the object value)*

#### 3. Nested Data Access (Dot Notation)
*(Identical to original)*

#### 4. Recursive Substitution
*(Identical to original)*

#### 5. Missing Key / Path Handling
*(Identical concept, refers to C++ exceptions)*
*   **`onMissingKey: 'ignore'` (Default) / `--on-missing-key=ignore`**: Output leaves placeholder.
*   **`onMissingKey: 'error'` / `--on-missing-key=error`**: Throws `clavis::ClavisMissingKeyException`.

#### 6. Custom Delimiters
*(Identical concept, refers to C++ options or CLI flags)*

#### 7. Cycle Detection
*(Identical concept, throws `clavis::ClavisCycleException`)*

## API (C++)

The primary C++ API is defined in `<clavis/clavis.hpp>` and `<clavis/exceptions.hpp>`.

*   **`clavis::Options` struct:** Configures processing.
    *   `variableStartMarker` (std::string, default: `${`)
    *   `variableEndMarker` (std::string, default: `}`)
    *   `onMissingKey` (clavis::MissingKeyBehavior enum, default: `Ignore`)
*   **`clavis::process(const nlohmann::json& template_json, const nlohmann::json& context, const clavis::Options& options = {})` function:** The main entry point. Returns the processed `nlohmann::json`.
*   **Exception Classes:**
    *   `clavis::ClavisException` (base class, inherits `std::runtime_error`)
    *   `clavis::ClavisParseException`
    *   `clavis::ClavisCycleException` (provides `.get_cycle_path()`)
    *   `clavis::ClavisMissingKeyException` (provides `.get_key_path()`)

## Contributing

Contributions are welcome! Please follow standard C++ best practices, ensure code is well-formatted, and add corresponding unit tests for any new features or bug fixes. Consider opening an issue first to discuss significant changes. (If a `CONTRIBUTING.md` file exists, refer to it for more detail).

## License

This work is released into the **Public Domain** using the **Unlicense**.

You are free to use, modify, distribute, and sublicense this work without any restrictions. See the accompanying `UNLICENSE` file for the full text. While attribution is not legally required, it is appreciated.
