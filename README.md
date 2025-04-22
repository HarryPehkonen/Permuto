# Permuto (C++ Implementation)

Permuto is a lightweight, flexible C++ library for generating JSON structures by substituting variables from a data context into a valid JSON template using `apply()`. It supports accessing nested data using dot notation and handles type conversions intelligently.

This C++ implementation uses the popular [nlohmann/json](https://github.com/nlohmann/json) library for JSON handling and is built using CMake.

Inspired by the need to dynamically create JSON payloads, such as those required for various LLM (Large Language Model) APIs, Permuto provides a focused templating mechanism. It is *not* a full programming language but excels at structured JSON generation based on input data.

It also supports a **reverse operation** using `create_reverse_template()` and `apply_reverse()`: extracting the original data context from a result JSON, provided the result was generated using the corresponding template with string interpolation disabled.

## Features

*   **Valid JSON Templates:** Uses standard, parseable JSON (via nlohmann/json) as the template input format.
*   **Variable Substitution (`apply`):** Replaces placeholders in a template using values from a context.
    *   **Exact Match:** Replaces placeholders that constitute the *entire* string value (e.g., `"${variable}"`) with values from a provided data context. This works regardless of the string interpolation setting.
        *   **Nested Data Access (Dot Notation):** Access values deep within the data context using dot notation (e.g., `${user.profile.email}`). Array indexing is not supported. Keys containing literal dots (`.`), tildes (`~`), or slashes (`/`) are supported via standard JSON Pointer escaping logic.
        *   **Automatic Type Handling:** Intelligently handles data types. When a placeholder like `"${variable}"` or `"${path.to.variable}"` is the *entire* string value in the template, and the corresponding data is a number, boolean, array, object, or null, the quotes are effectively removed in the output, preserving the correct JSON type. String data results in a standard JSON string output.
    *   **Optional String Interpolation:** Control whether placeholders within larger strings are processed via `permuto::Options::enableStringInterpolation`.
        *   **Enabled:** Set `enableStringInterpolation = true` or use `--string-interpolation` in the CLI. Substitutes variables within larger strings (e.g., `"Hello, ${user.name}!"`). Non-string values are converted to their compact JSON string representation (`null`, `true`/`false`, numbers, `"[1,2]"`, `{"k":"v"}`).
        *   **Disabled:** Leave `enableStringInterpolation = false` (default) or omit `--string-interpolation` in the CLI. Only exact matches are substituted. Strings containing placeholders but not forming an exact match are treated as literals. **Required for reverse operations.**
*   **Recursive Substitution:** Recursively processes substituted values that themselves contain placeholders during the `apply` operation. The behavior within the resolved string depends on the `enableStringInterpolation` setting.
*   **Cycle Detection:** Automatically detects and prevents infinite recursion loops during the `apply` operation caused by cyclical references (e.g., `{"a": "${b}", "b": "${a}"}` or involving paths), throwing a `permuto::PermutoCycleException` instead. This works regardless of interpolation mode for lookups that are actually performed.
*   **Configurable Missing Key Handling:** Choose whether to `ignore` missing keys/paths (`permuto::MissingKeyBehavior::Ignore`, default) or `error` out (`permuto::MissingKeyBehavior::Error`, throwing `permuto::PermutoMissingKeyException`) during `apply`.
    *   Applies to lookups triggered by exact matches in both interpolation modes.
    *   Applies to lookups triggered during interpolation *only when* `enableStringInterpolation` is `true`.
*   **Reverse Template Extraction & Application:** When string interpolation is *disabled* (`enableStringInterpolation = false`):
    *   Generate a `reverse_template` using `permuto::create_reverse_template()`. This template maps context variable paths to their locations (JSON Pointers) within the expected result JSON. Throws `std::logic_error` if interpolation is enabled in options.
    *   Apply this `reverse_template` to a `result_json` using `permuto::apply_reverse()` to reconstruct the original data context based on the template's structure. Throws `std::runtime_error` or `nlohmann::json` exceptions if the reverse template is malformed or if pointers don't resolve in the result JSON.
*   **Customizable Delimiters:** Define custom start (`variableStartMarker`, CLI: `--start`) and end (`variableEndMarker`, CLI: `--end`) markers for variables instead of the default `${` and `}`.
*   **Modern C++:** Implemented in C++17, using RAII, standard library features, and exceptions for error handling.
*   **CMake Build System:** Uses CMake for building, testing, and installation. Dependencies (nlohmann/json, GoogleTest) are handled via `FetchContent` if not found locally.
*   **Command-Line Tool:** Includes a `permuto` CLI tool for easy testing and usage from the command line (supports controlling interpolation via `--string-interpolation`).
*   **Testing:** Includes a comprehensive suite of unit tests (over 100 tests) using GoogleTest, covering both interpolation modes, reverse operations, edge cases, and error handling.
*   **Installation Support:** Provides CMake installation targets for easy integration into other projects.
*   **Permissive License:** Released into the Public Domain (Unlicense).

### Why Use Permuto Templates Instead of Direct JSON Pointer Lookups?

While Permuto uses JSON Pointers internally for robust data lookups (handling special characters in keys automatically), its primary value comes from its **declarative templating approach**.

Consider the use case of transforming data from a central structure (the "context") into different JSON formats required by various APIs (like different LLM providers).

*   **Without Permuto:** You would typically write C++ code for each target API. This code would programmatically build the required JSON structure and use direct `nlohmann::json` pointer lookups (e.g., `context.at("/path/to/data")`) to fetch data from your central structure and assign it to the correct place in the payload being built. The transformation logic resides entirely *within your C++ code*.
*   **With Permuto:** You define the target structure for each API in a separate **JSON template file**. These templates use `${context.path}` placeholders to specify where data from the central context should be inserted. Your C++ code simply loads the appropriate template and context, then calls `permuto::apply()`.

**Permuto's Advantage:**

*   **Declarative & Maintainable:** The structure of the API payload and the mapping from the context are defined *declaratively* in the template files. Need to adjust the format for one API? Modify its template JSON, often without recompiling your C++ application. Adding a new API? Create a new template file. This significantly improves maintainability.
*   **Readability & Separation:** Templates clearly show the desired output structure, separating the structural definition from the C++ application logic and the context data.
*   **Reduced Boilerplate:** Permuto handles the details of traversing the template, performing lookups (using pointers internally), preserving data types on exact matches, optionally interpolating strings, handling missing keys gracefully, and detecting cycles – features you would otherwise need to implement manually in C++.

In essence, Permuto provides a convenient abstraction layer over raw JSON Pointer manipulation, making it easier to manage and define structure transformations through external template files rather than embedding that logic directly in compiled code.

## Requirements

*   **C++17 Compliant Compiler:** GCC, Clang, or MSVC.
*   **CMake:** Version 3.15 or higher.
*   **Git:** Required by CMake's `FetchContent` to download dependencies if not found locally.
*   **Internet Connection:** May be needed for the initial CMake configuration to fetch dependencies (nlohmann/json, GoogleTest).

## Building

1.  **Clone the repository:**
    ```bash
    git clone https://github.com/HarryPehkonen/Permuto.git permuto
    cd permuto
    ```

2.  **Configure using CMake:** Create a build directory and run CMake. Using a build type like `Release` is recommended for performance.
    ```bash
    # Create a build directory (standard practice)
    cmake -S . -B build -DPERMUTO_ENABLE_TESTING=ON -DCMAKE_BUILD_TYPE=Release

    # Options:
    # -S . : Specifies the source directory (where the root CMakeLists.txt is)
    # -B build : Specifies the build directory (can be any name)
    # -DPERMUTO_ENABLE_TESTING=ON : Explicitly enable building tests (default is ON)
    # -DCMAKE_BUILD_TYPE=Release : Build optimized code (other options: Debug, RelWithDebInfo)
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

*   The static library `libpermuto-lib.a` (or `permuto-lib.lib`) inside `build/src/`.
*   The command-line executable `permuto` (or `permuto.exe`) inside `build/cli/`.
*   The test executable `permuto_tests` (or `permuto_tests.exe`) inside `build/tests/` (if testing is enabled).

## Testing

The project uses GoogleTest for unit testing.

1.  **Build the project** with tests enabled (ensure `-DPERMUTO_ENABLE_TESTING=ON` was used during CMake configuration).

2.  **Run the tests** using CTest from the *build* directory:
    ```bash
    cd build
    ctest --output-on-failure
    # Or for more verbose output:
    # ctest -V --output-on-failure
    ```
    Alternatively, run the test executable directly:
    ```bash
    ./tests/permuto_tests
    ```
    All tests (including those for reverse operations) should pass.

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
    *   Headers to `<prefix>/include/permuto/`
    *   Library to `<prefix>/lib/` (`libpermuto-lib.a`)
    *   CMake package configuration files to `<prefix>/share/permuto/cmake/`
    *   The `permuto` executable to `<prefix>/bin/`

## Usage

### Library Usage (C++)

Include the necessary headers and call the `permuto::apply` function, optionally configuring interpolation and other settings via the `permuto::Options` struct. For reverse operations, use `permuto::create_reverse_template` and `permuto::apply_reverse`.

**Example 1: Interpolation Enabled**

```c++
#include <permuto/permuto.hpp>      // Main header for apply() and Options
#include <permuto/exceptions.hpp> // Header for specific exception types
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
            "user": { "name": "Alice", "email": "alice@example.com" },
            "settings": { "primary": "ThemeA", "secondary": { "enabled": true } },
            "a": { "b": { "c": { "d": 12345 } } }
        }
    )"_json;

    try {
        // Use default options: enableStringInterpolation = true, onMissingKey = Ignore
        permuto::Options options;
        options.enableStringInterpolation = true;
        // options.onMissingKey = permuto::MissingKeyBehavior::Error;
        // options.variableStartMarker = "<<";
        // options.variableEndMarker = ">>";
        options.validate(); // Good practice to validate

        // Perform the substitution using apply()
        json result = permuto::apply(template_json, context, options);

        // Output the result
        std::cout << "--- Result (Interpolation ON) ---" << std::endl;
        std::cout << result.dump(4) << std::endl;

    } catch (const permuto::PermutoException& e) {
        std::cerr << "Permuto Error: " << e.what() << std::endl; return 1;
    } catch (const std::invalid_argument& e) {
         std::cerr << "Error: Invalid Options: " << e.what() << std::endl; return 1;
    } catch (const std::exception& e) {
        std::cerr << "Standard Error: " << e.what() << std::endl; return 1;
    }
    return 0;
}
```
*Output (Interpolation ON):*
```json
--- Result (Interpolation ON) ---
{
    "deep_value": 12345,
    "greeting": "Welcome, Alice!",
    "maybe_present": "${user.optional_field}",
    "primary_setting": "ThemeA",
    "secondary_enabled": true,
    "user_email": "alice@example.com"
}
```

**Example 2: Interpolation Disabled (Default)**

```c++
#include <permuto/permuto.hpp>
#include <permuto/exceptions.hpp>
#include <nlohmann/json.hpp>
#include <iostream>
#include <string>
#include <cassert>

int main() {
    using json = nlohmann::json;
    json template_json = R"( {"output_name": "${user.name}", "greeting": "Hello ${user.name}"} )"_json;
    json context = R"( {"user": {"name": "Bob"}} )"_json;
    json expected_result = R"( {"output_name": "Bob", "greeting": "Hello ${user.name}"} )"_json;

    try {
        permuto::Options options;
        // options.enableStringInterpolation = false;
        options.validate();

        // Process with Interpolation OFF using apply()
        json result = permuto::apply(template_json, context, options);
        std::cout << "--- Result (Interpolation OFF) ---" << std::endl;
        std::cout << result.dump(4) << std::endl;
        assert(result == expected_result); // Verify result
        std::cout << "\nResult matches expected output when interpolation is off." << std::endl;

    } catch (const permuto::PermutoException& e) {
        std::cerr << "Permuto Error: " << e.what() << std::endl; return 1;
    } catch (const std::invalid_argument& e) {
         std::cerr << "Error: Invalid Options: " << e.what() << std::endl; return 1;
    } catch (const std::exception& e) {
        std::cerr << "Standard Error: " << e.what() << std::endl; return 1;
    }
    return 0;
}
```
*Output (Interpolation OFF):*
```json
--- Result (Interpolation OFF) ---
{
    "greeting": "Hello ${user.name}",
    "output_name": "Bob"
}

Result matches expected output when interpolation is off.
```

**Example 3: Full Reverse Loop (Interpolation Disabled)**

This example shows generating a result, creating a reverse template, and using it to reconstruct the original context. This only works when interpolation is disabled for the initial `apply` call.

```c++
#include <permuto/permuto.hpp>
#include <permuto/exceptions.hpp>
#include <nlohmann/json.hpp>
#include <iostream>
#include <string>
#include <cassert> // For assert

int main() {
    using json = nlohmann::json;

    // 1. Define Template and Context
    json original_template = R"(
        {
            "userName": "${user.name}",
            "details": { "isActive": "${user.active}" },
            "ids": [ "${sys.id}", "${user.id}" ]
        }
    )"_json;

    json original_context = R"(
        {
            "user": { "name": "Charlie", "active": true, "id": 999 },
            "sys": { "id": "XYZ" }
        }
    )"_json;

    // 2. Set Options:
    permuto::Options options;
    // options.enableStringInterpolation = false;
    options.validate();

    try {
        // 3. Generate Result using apply()
        json result = permuto::apply(original_template, original_context, options);
        std::cout << "--- Generated Result (Interpolation OFF) ---" << std::endl;
        std::cout << result.dump(4) << std::endl;
        // Expected result: { "userName": "Charlie", "details": { "isActive": true }, "ids": [ "XYZ", 999 ] }

        // 4. Create the Reverse Template
        json reverse_template = permuto::create_reverse_template(original_template, options);
        std::cout << "\n--- Generated Reverse Template ---" << std::endl;
        std::cout << reverse_template.dump(4) << std::endl;
        // Expected: {"user": {"name": "/userName", "active": "/details/isActive", "id": "/ids/1"}, "sys": {"id": "/ids/0"}}

        // 5. Apply Reverse Template to Reconstruct Context
        json reconstructed_context = permuto::apply_reverse(reverse_template, result);
        std::cout << "\n--- Reconstructed Context ---" << std::endl;
        std::cout << reconstructed_context.dump(4) << std::endl;

        // 6. Verify Reconstruction
        assert(reconstructed_context == original_context);
        std::cout << "\nSuccessfully reconstructed original context!" << std::endl;

    } catch (const permuto::PermutoException& e) {
        std::cerr << "Permuto Error: " << e.what() << std::endl; return 1;
    } catch (const std::logic_error& e) { // Catch config errors for create_reverse_template
         std::cerr << "Configuration Error: " << e.what() << std::endl; return 1;
    } catch (const std::runtime_error& e) { // Catch errors from apply_reverse
         std::cerr << "Runtime Error (apply_reverse?): " << e.what() << std::endl; return 1;
    } catch (const std::invalid_argument& e) { // Catch option validation errors
         std::cerr << "Error: Invalid Options: " << e.what() << std::endl; return 1;
    } catch (const std::exception& e) { // Catch other potential errors
        std::cerr << "Standard Error: " << e.what() << std::endl; return 1;
    }
    return 0;
}
```
*Output (Full Reverse Loop):*
```json
--- Generated Result (Interpolation OFF) ---
{
    "details": {
        "isActive": true
    },
    "ids": [
        "XYZ",
        999
    ],
    "userName": "Charlie"
}

--- Generated Reverse Template ---
{
    "sys": {
        "id": "/ids/0"
    },
    "user": {
        "active": "/details/isActive",
        "id": "/ids/1",
        "name": "/userName"
    }
}

--- Reconstructed Context ---
{
    "sys": {
        "id": "XYZ"
    },
    "user": {
        "active": true,
        "id": 999,
        "name": "Charlie"
    }
}

Successfully reconstructed original context!
```

### Command-Line Tool Usage (`permuto`)

The `permuto` executable provides a command-line interface. *(Note: The CLI currently only performs the forward `apply` operation, not the reverse operations).*

**Synopsis:**

```bash
permuto <template_file> <context_file> [options]
```

**Arguments:**

*   `<template_file>`: Path to the input JSON template file.
*   `<context_file>`: Path to the input JSON data context file.

**Options:**

*   `--on-missing-key=<value>`: Behavior for missing keys (`ignore` or `error`). Default: `ignore`.
*   `--string-interpolation`: **Enable** string interpolation. If omitted, non-exact-match strings are treated as literals (default). If present, interpolation is **enabled**.
*   `--start=<string>`: Set the variable start delimiter (Default: `${`).
*   `--end=<string>`: Set the variable end delimiter (Default: `}`).
*   `--help`: Display usage information and exit.

**(CLI Examples conceptually use `apply` internally)**

**Example (Interpolation Enabled - Default):**

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
# Interpolation is OFF by default
./build/cli/permuto template.json context.json --string-interpolation
```

**Output (Pretty-printed JSON to stdout):**
```json
{
    "message": "Hello, CLI User!",
    "setting": true
}
```

**Example (Interpolation Disabled):**

Using the same `template.json` and `context.json`.

**Command:**
```bash
# No interpolation by default
./build/cli/permuto template.json context.json
```

**Output (Pretty-printed JSON to stdout):**
```json
{
    "message": "Hello, ${name}!", # Treated as literal because interpolation is off
    "setting": true               # Exact match still works
}
```

## Key Concepts & Examples

### 1. Data Type Handling (Exact Match Substitution)

This substitution mechanism applies when a template string value consists *only* of a single placeholder (e.g., `"${var}"`, `"${path.to.val}"`). It works identically whether string interpolation is enabled or disabled during `apply`. Permuto substitutes the actual JSON data type found at that path in the context, effectively removing the quotes from the template for non-string types.

| Template Fragment       | Data Context (`context`)                     | Output Fragment (`result`) | Notes                                     |
| :---------------------- | :------------------------------------------- | :------------------------- | :---------------------------------------- |
| `"num": "${data.val}"`  | `{"data": {"val": 123}}`                     | `"num": 123`               | Number type preserved.                    |
| `"bool": "${opts.on}"`  | `{"opts": {"on": true}}`                     | `"bool": true`             | Boolean type preserved.                   |
| `"arr": "${items.list}"` | `{"items": {"list": [1, "a"]}}`              | `"arr": [1, "a"]`          | Array type preserved.                     |
| `"obj": "${cfg.sec}"`   | `{"cfg": {"sec": {"k": "v"}}}`              | `"obj": {"k": "v"}`        | Object type preserved.                    |
| `"null_val": "${maybe}"`| `{"maybe": null}`                            | `"null_val": null`         | Null type preserved.                      |
| `"str_val": "${msg}"`   | `{"msg": "hello"}`                           | `"str_val": "hello"`       | String type preserved, quotes remain.     |

### 2. String Interpolation (Optional Feature)

This behavior only occurs during `apply` when interpolation is **enabled** (`enableStringInterpolation = true`, or `--string-interpolation` used in CLI).

When a placeholder is part of a larger string *and* interpolation is enabled, the data value found at that path is converted to its string representation and inserted. Non-string types are stringified using their compact JSON representation (`null`, `true`/`false`, numbers, arrays like `[1,true]`, objects like `{"key":"value"}`).

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
**Output (Interpolation Enabled):**
```json
{
  "message": "User Alice (ID: 123) is true. Count: null. Settings: {\"notify\":false,\"theme\":\"dark\"}"
}
```
**Output (Interpolation Disabled - default):**
```json
{
  "message": "User ${user.name} (ID: ${user.id}) is ${status.active}. Count: ${data.count}. Settings: ${settings}"
}
```
*(The entire string is treated as a literal because it's not an exact match and interpolation is disabled).*

### 3. Nested Data Access (Dot Notation)

Use dots (`.`) within placeholders to access nested properties. This works for exact matches (both modes) and during interpolation (when enabled).

**Template:** `{"city": "${address.city}", "zip": "${address.postal_code}"}`
**Data:** `{"address": {"city": "Anytown", "postal_code": "12345"}}`
**Output (Either Mode):** `{"city": "Anytown", "zip": "12345"}`

*   **Path Resolution:** Converts dot-paths (`a.b`) to JSON Pointers (`/a/b`), handling `/` and `~` escaping.
*   **Keys with Dots:** Access keys like `"user.role"` using `${user.role}`.
*   **Array Indexing:** Not supported via dot notation (`${arr.0}` won't work).

### 4. Recursive Substitution

If an exact match during `apply` resolves to a string containing placeholders, that string is recursively processed using the *same* `enableStringInterpolation` setting.

**Template:** ` { "msg": "${greeting.template}", "info": "${user.details}" } `
**Data:** ` { "greeting": { "template": "Hello, ${user.name}!" }, "user": { "name": "Bob", "details": "${user.name}" } } `

*   **Output (Interpolation Enabled):**
    *   `"${greeting.template}"` -> `"Hello, ${user.name}!"` -> (Interpolation applied) -> `"Hello, Bob!"`
    *   `"${user.details}"` -> `"${user.name}"` -> (Exact Match) -> `"Bob"`
    *   Result: `{ "msg": "Hello, Bob!", "info": "Bob" }`
*   **Output (Interpolation Disabled):**
    *   `"${greeting.template}"` -> `"Hello, ${user.name}!"` -> (No interpolation) -> Literal `"Hello, ${user.name}!"`
    *   `"${user.details}"` -> `"${user.name}"` -> (Exact Match) -> `"Bob"`
    *   Result: `{ "msg": "Hello, ${user.name}!", "info": "Bob" }`

### 5. Missing Key / Path Handling

Use `onMissingKey` (`Ignore` or `Error`) or `--on-missing-key` (`ignore` or `error`) to control behavior during `apply`.

*   **`Ignore` (Default):** Unresolved placeholders remain literally (e.g., `"${missing}"` or `"Text ${missing} text"`).
*   **`Error`:** Throws `permuto::PermutoMissingKeyException`.
    *   Triggered by failed lookups for **exact matches** (in both interpolation modes).
    *   Triggered by failed lookups during **interpolation** *only when* `enableStringInterpolation` is `true`. Lookups are not attempted for non-exact-match strings when interpolation is disabled.

**Template:** `{"value": "${a.b.c}", "interpolated": "Maybe: ${x.y.z}!"}`
**Data:** `{"a": {"b": {}}}` (`c` missing; `x` missing)

*   **`Ignore` Mode Output (Either Interpolation Mode):** `{"value": "${a.b.c}", "interpolated": "Maybe: ${x.y.z}!"}`
*   **`Error` Mode, Interpolation ON:** Throws exception when `apply` processes `"Maybe: ${x.y.z}!"`.
*   **`Error` Mode, Interpolation OFF:** Throws exception when `apply` processes `"${a.b.c}"` (exact match). Does *not* throw for `"Maybe: ${x.y.z}!"` because interpolation is off.

### 6. Custom Delimiters

Use `Options::variableStartMarker`/`EndMarker` or `--start`/`--end`. Applies in both `apply` and reverse operations.

### 7. Cycle Detection

Detects cycles like `${a}` -> `${b}` -> `${a}` during the resolution phase of `apply`. Throws `permuto::PermutoCycleException`. Applies in both interpolation modes for lookups that are actually performed.

### 8. Reverse Template Extraction & Application (Interpolation Disabled Only)

This feature allows you to extract the original context data if you have the `result_json` and the `original_template`, but **only** if the result was generated using `apply()` with `enableStringInterpolation = false`.

**Concept:** The `original_template` defines where context variables *should* appear in the `result_json`. A `reverse_template` is generated that maps the context variable paths to JSON Pointers indicating their location in the `result_json`.

**Process:**

1.  **Generate Reverse Template:**
    *   Call `permuto::create_reverse_template(original_template, options)` where `options.enableStringInterpolation` is `false`.
    *   This function scans `original_template` for exact-match placeholders (`"${context.path}"`).
    *   It builds a JSON structure mirroring the expected *context*, where leaf values are JSON Pointer strings pointing to the corresponding location in the *result*.
    *   Throws `std::logic_error` if `options.enableStringInterpolation` is `true`.

2.  **Apply Reverse Template:**
    *   Call `permuto::apply_reverse(reverse_template, result_json)`.
    *   This function traverses the `reverse_template`.
    *   For each leaf node (JSON Pointer string), it looks up the value at that pointer location within `result_json`.
    *   It builds the `reconstructed_context` JSON, placing the extracted values at the correct paths defined by the `reverse_template` structure.
    *   Throws `std::runtime_error` or `nlohmann::json` exceptions if the reverse template is malformed or pointers don't resolve.

**Example:** (See C++ Example 3 above for a runnable demonstration)

*   **Original Template `T`:** `{"output_name": "${user.name}", "ids": ["${sys.id}"]}`
*   **Context `C`:** `{"user": {"name": "Alice"}, "sys": {"id": 1}}`
*   **Options `O`:** `enableStringInterpolation = false`
*   **Result `R = apply(T, C, O)`:** `{"output_name": "Alice", "ids": [1]}`
*   **Reverse Template `RT = create_reverse_template(T, O)`:** `{"user": {"name": "/output_name"}, "sys": {"id": "/ids/0"}}`
*   **Reconstructed Context `C_new = apply_reverse(RT, R)`:** `{"user": {"name": "Alice"}, "sys": {"id": 1}}` (Matches `C`)

**Limitations:** This extraction only works if interpolation was disabled during the original `apply()` call and only recovers context variables referenced by exact-match placeholders in the template. Any context data not used in an exact-match placeholder in the original template cannot be recovered.

## Configuration Options

These options are passed via the `permuto::Options` struct in C++ or via command-line flags to the CLI tool.

| C++ Option Member           | CLI Flag                    | Description                                                                   | Default Value | C++ Type/Enum                  |
| :-------------------------- | :-------------------------- | :---------------------------------------------------------------------------- | :------------ | :----------------------------- |
| `variableStartMarker`     | `--start=<string>`          | The starting delimiter for placeholders.                                      | `${`          | `std::string`                  |
| `variableEndMarker`       | `--end=<string>`            | The ending delimiter for placeholders.                                        | `}`           | `std::string`                  |
| `onMissingKey`            | `--on-missing-key=`         | Behavior for unresolved placeholders during `apply` (`Ignore`/`Error`).         | `Ignore`      | `permuto::MissingKeyBehavior` |
| `enableStringInterpolation` | `--string-interpolation` | If `true`, interpolates variables in non-exact-match strings during `apply`. If `false`, non-exact-match strings are treated as literals. Reverse operations require `false`. | `true`        | `bool`                         |

## C++ API Details

The primary C++ API is defined in `<permuto/permuto.hpp>` and `<permuto/exceptions.hpp>`.

*   **`permuto::Options` struct:**
    *   `std::string variableStartMarker = "${";`
    *   `std::string variableEndMarker = "}";`
    *   `permuto::MissingKeyBehavior onMissingKey = permuto::MissingKeyBehavior::Ignore;`
    *   `bool enableStringInterpolation = true;` *(Default is false)*
    *   `void validate() const;` (Throws `std::invalid_argument` on bad options like empty/identical delimiters)

*   **`permuto::MissingKeyBehavior` enum class:**
    *   `Ignore`
    *   `Error`

*   **`permuto::apply` function:**
    ```c++
    nlohmann::json apply(
        const nlohmann::json& template_json,
        const nlohmann::json& context,
        const permuto::Options& options = {} // Default options used if omitted
    );
    ```
    Applies the template to the context based on options. Throws exceptions on errors (cycle, missing key if `Error`, invalid options).

*   **`permuto::create_reverse_template` function:**
    ```c++
    nlohmann::json create_reverse_template(
        const nlohmann::json& original_template,
        const permuto::Options& options = {}
    );
    ```
    Generates the reverse template mapping. Requires `options.enableStringInterpolation` to be `false` (default). Throws `std::logic_error` if interpolation is enabled. Throws `std::invalid_argument` if options are invalid.

*   **`permuto::apply_reverse` function:**
    ```c++
    nlohmann::json apply_reverse(
        const nlohmann::json& reverse_template,
        const nlohmann::json& result_json
    );
    ```
    Uses the `reverse_template` and `result_json` to reconstruct the context data. Throws `std::runtime_error` or `nlohmann::json` exceptions if `reverse_template` is invalid or pointers don't resolve in `result_json`.

*   **Exception Classes (in `<permuto/exceptions.hpp>`):**
    *   `permuto::PermutoException` (Base class, inherits `std::runtime_error`)
    *   `permuto::PermutoParseException` (Currently unused)
    *   `permuto::PermutoCycleException` (Inherits `PermutoException`, provides `const std::string& get_cycle_path() const`)
    *   `permuto::PermutoMissingKeyException` (Inherits `PermutoException`, provides `const std::string& get_key_path() const`)
    *   `std::invalid_argument` is thrown by `apply` or `create_reverse_template` if `options` are invalid.
    *   `std::logic_error` is thrown by `create_reverse_template` if `enableStringInterpolation` is `true`.
    *   `std::runtime_error` (or `nlohmann::json` exceptions) can be thrown by `apply_reverse` for malformed reverse templates or unresolved JSON pointers.

## Using Permuto in another CMake Project

After installing Permuto (using `cmake --install`), you can easily integrate it into your own CMake project using `find_package`.

1.  **Update your `CMakeLists.txt`:**
    ```cmake
    cmake_minimum_required(VERSION 3.15)
    project(MyProject)

    find_package(Permuto REQUIRED) # Find the installed package

    add_executable(my_app main.cpp)

    # Link the Permuto library
    target_link_libraries(my_app PRIVATE Permuto::permuto-lib)

    # Permuto::permuto-lib will automatically bring in include directories
    # and dependencies like nlohmann_json if they were linked publicly/interface.
    ```

2.  **Include and use in your C++ code:**
    ```c++
    #include <permuto/permuto.hpp>
    #include <permuto/exceptions.hpp>
    #include <nlohmann/json.hpp>
    #include <iostream>

    int main() {
        nlohmann::json my_template = /* ... */;
        nlohmann::json my_context = /* ... */;
        permuto::Options opts;
        // opts.enableStringInterpolation = false; // Example: Disable for reverse
        try {
            // Forward operation
            nlohmann::json result = permuto::apply(my_template, my_context, opts);
            std::cout << "Result: " << result.dump(2) << std::endl;

            // Reverse operation (if interpolation was false)
            if (!opts.enableStringInterpolation) {
                nlohmann::json rev_template = permuto::create_reverse_template(my_template, opts);
                nlohmann::json reconstructed = permuto::apply_reverse(rev_template, result);
                std::cout << "Reconstructed: " << reconstructed.dump(2) << std::endl;
            }

        } catch (const permuto::PermutoException& e) {
            std::cerr << "Permuto error: " << e.what() << std::endl;
        } catch (const std::exception& e) { // Catch logic_error, runtime_error etc.
            std::cerr << "Error: " << e.what() << std::endl;
        }
        return 0;
    }
    ```

## Contributing

Contributions are welcome! Please adhere to the following guidelines:

*   Follow standard C++17 best practices.
*   Maintain code style and formatting consistency.
*   Add unit tests (in `tests/`) for any new features or bug fixes, covering both interpolation modes and reverse template functionality.
*   Ensure all tests pass (`ctest --output-on-failure` in the build directory).
*   Consider opening an issue first to discuss significant changes or new features.
*   Submit changes via Pull Requests.
*   WebKit-style commit messages are preferred.


## License

This work is released into the **Public Domain** (Unlicense).

You are free to copy, modify, publish, use, compile, sell, or distribute this software, either in source code form or as a compiled binary, for any purpose, commercial or non-commercial, and by any means.

See the accompanying `LICENSE` file for the full Unlicense text. While attribution is not legally required, it is appreciated.
