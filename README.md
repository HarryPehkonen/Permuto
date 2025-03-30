# Clavis (C++ Implementation)

Clavis is a lightweight, flexible C++ library for generating JSON structures by substituting variables from a data context into a valid JSON template. It supports accessing nested data using dot notation and handles type conversions intelligently.

This C++ implementation uses the popular [nlohmann/json](https://github.com/nlohmann/json) library for JSON handling and is built using CMake.

Inspired by the need to dynamically create JSON payloads, such as those required for various LLM (Large Language Model) APIs, Clavis provides a focused templating mechanism. It is *not* a full programming language but excels at structured JSON generation based on input data.

## Features

*   **Valid JSON Templates:** Uses standard, parseable JSON (via nlohmann/json) as the template input format.
*   **Variable Substitution:** Replaces placeholders (e.g., `${variable}`) with values from a provided data context.
*   **Nested Data Access (Dot Notation):** Access values deep within the data context using dot notation (e.g., `${user.profile.email}`). Array indexing is not supported. Keys containing literal dots (`.`), tildes (`~`), or slashes (`/`) are supported via standard JSON Pointer escaping logic.
*   **Automatic Type Handling:** Intelligently handles data types. When a placeholder like `"${variable}"` or `"${path.to.variable}"` is the *entire* string value in the template, and the corresponding data is a number, boolean, array, object, or null, the quotes are effectively removed in the output, preserving the correct JSON type. String data results in a standard JSON string output.
*   **String Interpolation:** Substitutes variables (including nested ones) within larger strings (e.g., `"Hello, ${user.name}!"`). Non-string values are converted to their compact JSON string representation (`null`, `true`/`false`, numbers, `"[1,2]"`, `{"k":"v"}`).
*   **Recursive Substitution:** Recursively processes substituted values that themselves contain placeholders.
*   **Cycle Detection:** Automatically detects and prevents infinite recursion loops caused by cyclical references (e.g., `{"a": "${b}", "b": "${a}"}` or involving paths), throwing a `clavis::ClavisCycleException` instead.
*   **Configurable Missing Key Handling:** Choose whether to `ignore` missing keys/paths (leaving the placeholder) or `error` out (throwing `clavis::ClavisMissingKeyException`). This applies to top-level keys and keys accessed during path traversal.
*   **Customizable Delimiters:** Define custom start (`variableStartMarker`, CLI: `--start`) and end (`variableEndMarker`, CLI: `--end`) markers for variables instead of the default `${` and `}`.
*   **Modern C++:** Implemented in C++17, using RAII, standard library features, and exceptions for error handling.
*   **CMake Build System:** Uses CMake for building, testing, and installation. Dependencies (nlohmann/json, GoogleTest) are handled via `FetchContent` if not found locally.
*   **Command-Line Tool:** Includes a `clavis` CLI tool for easy testing and usage from the command line.
*   **Testing:** Includes unit tests using GoogleTest.
*   **Installation Support:** Provides CMake installation targets for easy integration into other projects.
*   **Permissive License:** Released into the Public Domain (Unlicense).

## Requirements

*   **C++17 Compliant Compiler:** GCC, Clang, or MSVC.
*   **CMake:** Version 3.15 or higher.
*   **Git:** Required by CMake's `FetchContent` to download dependencies if not found locally.
*   **Internet Connection:** May be needed for the initial CMake configuration to fetch dependencies (nlohmann/json, GoogleTest).

## Building

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
    # -DCMAKE_INSTALL_PREFIX=/path/to/install : Optionally set install location
    ```
    CMake will automatically try to find nlohmann/json and GoogleTest using `find_package`. If not found, it will download them using `FetchContent`.

3.  **Build the code:**
    ```bash
    cmake --build build
    # To build in parallel (faster):
    # cmake --build build --parallel $(nproc) # Linux example
    # cmake --build build --parallel <number_of_jobs>
    ```

This will build:

*   The static library `libclavis-lib.a` (or `clavis-lib.lib`) inside `build/src/`.
*   The command-line executable `clavis` (or `clavis.exe`) inside `build/cli/`.
*   The test executable `clavis_tests` (or `clavis_tests.exe`) inside `build/tests/` (if testing is enabled).

## Testing

The project uses GoogleTest for unit testing.

1.  **Build the project** with tests enabled (ensure `-DCLAVIS_ENABLE_TESTING=ON` was used during CMake configuration).

2.  **Run the tests** using CTest from the *build* directory:
    ```bash
    cd build
    ctest --output-on-failure
    ```
    Alternatively, run the test executable directly:
    ```bash
    ./tests/clavis_tests
    ```
    All tests should pass.

## Installation

You can install the library headers and the compiled library file for use in other CMake projects.

1.  **Configure and Build** as described above. You might want to set `CMAKE_INSTALL_PREFIX` during configuration if you don't want a system-wide install.

2.  **Install using CMake:**
    ```bash
    # From the project root directory:
    cmake --install build

    # Or if you specified a prefix during configuration:
    # cmake --install build --prefix /path/to/custom/install/location
    # (Note: The prefix in the install command overrides the configure-time prefix)
    ```
    This installs:
    *   Headers to `<prefix>/include/clavis/`
    *   Library to `<prefix>/lib/` (`libclavis-lib.a`)
    *   CMake package configuration files to `<prefix>/share/clavis/cmake/`
    *   The `clavis` executable to `<prefix>/bin/`

## Usage

### Library Usage (C++)

Include the necessary headers and call the `clavis::process` function.

```c++
#include <clavis/clavis.hpp>      // Main header for process() and Options
#include <clavis/exceptions.hpp> // Header for specific exception types
#include <nlohmann/json.hpp>     // JSON library
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
        options.onMissingKey = clavis::MissingKeyBehavior::Ignore; // Default is Ignore
        // options.onMissingKey = clavis::MissingKeyBehavior::Error;
        // options.variableStartMarker = "<<";
        // options.variableEndMarker = ">>";

        // Perform the substitution
        json result = clavis::process(template_json, context, options);

        // Output the result (pretty-printed with 4 spaces)
        std::cout << result.dump(4) << std::endl;

    } catch (const clavis::ClavisMissingKeyException& e) {
        std::cerr << "Error: Missing Key! " << e.what() << " Path: [" << e.get_key_path() << "]" << std::endl;
        return 1;
    } catch (const clavis::ClavisCycleException& e) {
        std::cerr << "Error: Cycle Detected! " << e.what() << " Cycle: [" << e.get_cycle_path() << "]" << std::endl;
        return 1;
    } catch (const clavis::ClavisException& e) { // Catch base Clavis exceptions
        std::cerr << "Clavis Error: " << e.what() << std::endl;
        return 1;
    } catch (const std::invalid_argument& e) { // Catch option validation errors
         std::cerr << "Error: Invalid Options: " << e.what() << std::endl;
        return 1;
    } catch (const std::exception& e) { // Catch other potential errors (e.g., JSON parsing)
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

The `clavis` executable provides a command-line interface for processing files.

**Synopsis:**

```bash
clavis <template_file> <context_file> [options]
```

**Arguments:**

*   `<template_file>`: Path to the input JSON template file.
*   `<context_file>`: Path to the input JSON data context file.

**Options:**

*   `--on-missing-key=<value>`: Behavior for missing keys/paths.
    *   `ignore`: (Default) Leave placeholders unresolved.
    *   `error`: Print an error message to stderr and exit with failure if a key/path is missing.
*   `--start=<string>`: Set the variable start delimiter (Default: `${`).
*   `--end=<string>`: Set the variable end delimiter (Default: `}`).
*   `--help`: Display usage information and exit.

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
# Assuming 'clavis' executable is in the PATH or relative path from build dir
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
Clavis Error: Missing Key Error: Key or path not found: missing.key Path: [missing.key]
```

## Key Concepts & Examples

### 1. Data Type Handling (Exact Match Substitution)

When a template string value consists *only* of a single placeholder (e.g., `"${var}"`, `"${path.to.val}"`), Clavis substitutes the actual JSON data type found at that path in the context. Quotes around the placeholder in the template are effectively removed for non-string types in the output.

| Template Fragment       | Data Context (`context`)                     | Output Fragment (`result`) | Notes                                     |
| :---------------------- | :------------------------------------------- | :------------------------- | :---------------------------------------- |
| `"num": "${data.val}"`  | `{"data": {"val": 123}}`                     | `"num": 123`               | Number type preserved.                    |
| `"bool": "${opts.on}"`  | `{"opts": {"on": true}}`                     | `"bool": true`             | Boolean type preserved.                   |
| `"arr": "${items.list}"` | `{"items": {"list": [1, "a"]}}`              | `"arr": [1, "a"]`          | Array type preserved.                     |
| `"obj": "${cfg.sec}"`   | `{"cfg": {"sec": {"k": "v"}}}`              | `"obj": {"k": "v"}`        | Object type preserved.                    |
| `"null_val": "${maybe}"`| `{"maybe": null}`                            | `"null_val": null`         | Null type preserved.                      |
| `"str_val": "${msg}"`   | `{"msg": "hello"}`                           | `"str_val": "hello"`       | String type preserved, quotes remain.     |
| `"str_num": "${v.s}"`   | `{"v": {"s": "123"}}`                        | `"str_num": "123"`         | Input data was string, output is string.  |

### 2. String Interpolation

When a placeholder is part of a larger string, the data value found at that path is converted to its string representation and inserted. In C++, non-string types are stringified as follows: `null` becomes `"null"`, booleans become `"true"` or `"false"`, numbers are converted to strings (e.g., `123`, `3.14`), and arrays/objects are converted using their compact JSON representation (e.g., `"[1,true]"`, `{"key":"value"}`).

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

### 3. Nested Data Access (Dot Notation)

Use dots (`.`) within placeholders to access nested properties in the data context.

**Template:**
```json
{"city": "${address.city}", "zip": "${address.postal_code}"}
```
**Data:**
```json
{"address": {"street": "123 Main St", "city": "Anytown", "postal_code": "12345"}}
```
**Output:**
```json
{"city": "Anytown", "zip": "12345"}
```

*   **Path Resolution:** The engine converts the dot-separated path (e.g., `address.city`) into a [JSON Pointer](https://datatracker.ietf.org/doc/html/rfc6901) (e.g., `/address/city`) for robust lookup within the nlohmann/json object. Special characters in keys (like `/` and `~`) are handled correctly according to JSON Pointer rules.
*   **Keys with Dots:** Keys in the data context that contain literal dots (e.g., `"user.role": "admin"`) are accessed by treating the dot as part of the key segment in the path (e.g., `${user.role}`).
*   **Array Indexing:** Accessing array elements by index (e.g., `${my_array.0}`) is **not supported** via dot notation. Use cases requiring array element access might need pre-processing of the context or template.

### 4. Recursive Substitution

Substitution is applied recursively. If a placeholder resolves to a string value that itself contains placeholders, that string is also processed.

**Template:**
```json
{
  "message": "${greeting.template}",
  "user_info": "${user.details}"
}
```
**Data:**
```json
{
  "greeting": {
    "template": "Hello, ${user.name}!"
  },
  "user": {
    "name": "Bob",
    "details": "Name: ${user.name}"
  }
}
```
**Output:**
```json
{
  "message": "Hello, Bob!",
  "user_info": "Name: Bob"
}
```

### 5. Missing Key / Path Handling

Configure how unresolved placeholders are treated using the `onMissingKey` option (or `--on-missing-key` CLI flag). This applies if the initial key is missing, or if any intermediate key in a path does not exist or points to a non-object value when further traversal is expected.

**Template:**
```json
{"value": "${a.b.c}", "another": "${x.y.z}"}
```
**Data:**
```json
{"a": {"b": { "some_other_key": 1 } } }
```
*(`c` is missing from `a.b`; `x` is missing entirely)*

*   **`clavis::MissingKeyBehavior::Ignore` / `--on-missing-key=ignore` (Default)**
    *   **Output:** `{"value": "${a.b.c}", "another": "${x.y.z}"}`
    *   The placeholder remains untouched if *any* part of the path resolution fails. Useful for multi-pass templating or optional values.

*   **`clavis::MissingKeyBehavior::Error` / `--on-missing-key=error`**
    *   **Output:** *Throws `clavis::ClavisMissingKeyException` (or prints error and exits for CLI)*
    *   Processing stops, indicating the first path that could not be fully resolved. The exception contains the unresolved path via `.get_key_path()`.

### 6. Custom Delimiters

Change the markers used to identify variables via the `Options` struct or CLI flags.

**C++ Configuration:**
```c++
clavis::Options options;
options.variableStartMarker = "<<";
options.variableEndMarker = ">>";
```
**CLI Configuration:** `--start="<<" --end=">>"`

**Template:**
```json
{
  "custom": "<<config.host>>",
  "mixed": "Value is <<config.port>>."
}
```
**Data:**
```json
{"config": {"host": "example.com", "port": 8080}}
```
**Output:**
```json
{
  "custom": "example.com",
  "mixed": "Value is 8080."
}
```
*Validation:* The library throws `std::invalid_argument` if `variableStartMarker` or `variableEndMarker` are empty or identical.

### 7. Cycle Detection

Clavis prevents infinite loops caused by placeholders resolving cyclically, including indirect cycles through multiple lookups.

**Template:**
```json
{"a": "${path.to.b}"}
```
**Data:**
```json
{"path": {"to": {"b": "${ cycled.back.to.a}" } }, "cycled": {"back": {"to": {"a": "${path.to.b}"}}}}
```

**Processing:** Attempting to process this template and data will throw a `clavis::ClavisCycleException`. The exception contains a representation of the detected cycle via `.get_cycle_path()`.

## Configuration Options

These options are passed via the `clavis::Options` struct in C++ or via command-line flags to the CLI tool.

| C++ Option Member       | CLI Flag             | Description                                          | Default Value | C++ Type/Enum                  |
| :---------------------- | :------------------- | :--------------------------------------------------- | :------------ | :----------------------------- |
| `variableStartMarker` | `--start=<string>`   | The starting delimiter for placeholders.             | `${`          | `std::string`                  |
| `variableEndMarker`   | `--end=<string>`     | The ending delimiter for placeholders.               | `}`           | `std::string`                  |
| `onMissingKey`        | `--on-missing-key=`  | Behavior for unresolved placeholders (`ignore`/`error`). | `ignore`      | `clavis::MissingKeyBehavior` |

## C++ API Details

The primary C++ API is defined in `<clavis/clavis.hpp>` and `<clavis/exceptions.hpp>`.

*   **`clavis::Options` struct:**
    *   `std::string variableStartMarker = "${";`
    *   `std::string variableEndMarker = "}";`
    *   `clavis::MissingKeyBehavior onMissingKey = clavis::MissingKeyBehavior::Ignore;`
    *   `void validate() const;` (Throws `std::invalid_argument` on bad options)

*   **`clavis::MissingKeyBehavior` enum class:**
    *   `Ignore`
    *   `Error`

*   **`clavis::process` function:**
    ```c++
    nlohmann::json process(
        const nlohmann::json& template_json,
        const nlohmann::json& context,
        const clavis::Options& options = {} // Default options used if omitted
    );
    ```
    Takes the template JSON and context JSON, applies substitutions based on options, and returns the resulting JSON. Throws exceptions on errors.

*   **Exception Classes (in `<clavis/exceptions.hpp>`):**
    *   `clavis::ClavisException` (Base class, inherits `std::runtime_error`)
    *   `clavis::ClavisParseException` (Currently unused, JSON parsing handled by nlohmann/json upstream)
    *   `clavis::ClavisCycleException` (Inherits `ClavisException`, provides `const std::string& get_cycle_path() const`)
    *   `clavis::ClavisMissingKeyException` (Inherits `ClavisException`, provides `const std::string& get_key_path() const`)
    *   `std::invalid_argument` is thrown by `clavis::process` or `options.validate()` if options are invalid.

## Using Clavis in another CMake Project

After installing Clavis (using `cmake --install`), you can easily integrate it into your own CMake project using `find_package`.

1.  **Update your `CMakeLists.txt`:**

    ```cmake
    cmake_minimum_required(VERSION 3.15)
    project(MyClavisConsumer LANGUAGES CXX)

    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)

    # Find the installed Clavis package
    # If you installed Clavis to a custom prefix, you might need to hint CMake:
    # list(APPEND CMAKE_PREFIX_PATH "/path/to/custom/install/location")
    # Or set CMAKE_PREFIX_PATH environment variable or CMake argument.
    find_package(clavis 0.1.0 REQUIRED) # Request version 0.1.0 or compatible newer

    add_executable(my_app main.cpp)

    # Link against the imported Clavis library target.
    # This automatically handles include directories and dependencies (like nlohmann_json).
    target_link_libraries(my_app PRIVATE Clavis::clavis-lib)
    ```

2.  **Include and use in your C++ code:**

    ```c++
    #include <clavis/clavis.hpp>
    #include <clavis/exceptions.hpp>
    #include <nlohmann/json.hpp> // You'll need nlohmann::json too
    #include <iostream>

    int main() {
        nlohmann::json template_json = R"({"msg": "Hello, ${name}"})"_json;
        nlohmann::json context = R"({"name": "World from My Project"})"_json;
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

## Contributing

Contributions are welcome! Please adhere to the following guidelines:

*   Follow standard C++17 best practices.
*   Maintain code style and formatting consistency.
*   Add unit tests (in `tests/`) for any new features or bug fixes.
*   Ensure all tests pass (`ctest --output-on-failure` in the build directory).
*   Consider opening an issue first to discuss significant changes or new features.
*   Submit changes via Pull Requests.

## License

This work is released into the **Public Domain** (Unlicense).

You are free to copy, modify, publish, use, compile, sell, or distribute this software, either in source code form or as a compiled binary, for any purpose, commercial or non-commercial, and by any means.

See the accompanying `LICENSE` file for the full Unlicense text. While attribution is not legally required, it is appreciated.
