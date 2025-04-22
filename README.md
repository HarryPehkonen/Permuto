# Permuto (C++ Implementation)

Permuto is a lightweight, flexible C++ library for generating JSON structures by substituting variables from a data context into a valid JSON template. It supports accessing nested data using dot notation and handles type conversions intelligently.

This C++ implementation uses the popular [nlohmann/json](https://github.com/nlohmann/json) library for JSON handling and is built using CMake.

Inspired by the need to dynamically create JSON payloads, such as those required for various LLM (Large Language Model) APIs, Permuto provides a focused templating mechanism. It is *not* a full programming language but excels at structured JSON generation based on input data.

It also optionally supports a **reverse operation** (extracting context from a result and template) by disabling string interpolation.

## Features

*   **Valid JSON Templates:** Uses standard, parseable JSON (via nlohmann/json) as the template input format.
*   **Variable Substitution (Exact Match):** Replaces placeholders that constitute the *entire* string value (e.g., `"${variable}"`) with values from a provided data context.
    *   **Nested Data Access (Dot Notation):** Access values deep within the data context using dot notation (e.g., `${user.profile.email}`). Array indexing is not supported. Keys containing literal dots (`.`), tildes (`~`), or slashes (`/`) are supported via standard JSON Pointer escaping logic.
    *   **Automatic Type Handling:** Intelligently handles data types. When a placeholder like `"${variable}"` or `"${path.to.variable}"` is the *entire* string value in the template, and the corresponding data is a number, boolean, array, object, or null, the quotes are effectively removed in the output, preserving the correct JSON type. String data results in a standard JSON string output.
*   **Optional String Interpolation:** By default (`enableStringInterpolation = true`), substitutes variables (including nested ones) within larger strings (e.g., `"Hello, ${user.name}!"`). Non-string values are converted to their compact JSON string representation (`null`, `true`/`false`, numbers, `"[1,2]"`, `{"k":"v"}`). This behavior can be disabled (`enableStringInterpolation = false`) for stricter processing or to enable reverse operations.
*   **Recursive Substitution:** Recursively processes substituted values that themselves contain placeholders (applies in both interpolation modes, but primarily affects exact matches when interpolation is off).
*   **Cycle Detection:** Automatically detects and prevents infinite recursion loops caused by cyclical references (e.g., `{"a": "${b}", "b": "${a}"}` or involving paths), throwing a `permuto::PermutoCycleException` instead.
*   **Configurable Missing Key Handling:** Choose whether to `ignore` missing keys/paths (leaving the placeholder for exact matches, or leaving the part of the string unsubstituted during interpolation) or `error` out (throwing `permuto::PermutoMissingKeyException`). This applies to top-level keys and keys accessed during path traversal. When interpolation is disabled, this only applies to failed exact-match lookups.
*   **Reverse Template Extraction (Experimental):** When string interpolation is *disabled* (`enableStringInterpolation = false`), you can:
    *   Generate a `reverse_template` using `permuto::create_reverse_template()`. This template maps context variable paths to their locations (JSON Pointers) within the expected result JSON.
    *   Apply this `reverse_template` to a `result_json` using `permuto::apply_reverse_template()` to reconstruct the original data context based on the template's structure. This is useful for extracting structured data from a result JSON based on a known template format.
*   **Customizable Delimiters:** Define custom start (`variableStartMarker`, CLI: `--start`) and end (`variableEndMarker`, CLI: `--end`) markers for variables instead of the default `${` and `}`.
*   **Modern C++:** Implemented in C++17, using RAII, standard library features, and exceptions for error handling.
*   **CMake Build System:** Uses CMake for building, testing, and installation. Dependencies (nlohmann/json, GoogleTest) are handled via `FetchContent` if not found locally.
*   **Command-Line Tool:** Includes a `permuto` CLI tool for easy testing and usage from the command line (supports controlling interpolation).
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
    git clone https://github.com/HarryPehkonen/Permuto.git permuto
    cd permuto
    ```

2.  **Configure using CMake:** Create a build directory and run CMake.
    ```bash
    # Create a build directory (standard practice)
    cmake -S . -B build -DPERMUTO_ENABLE_TESTING=ON

    # Options:
    # -S . : Specifies the source directory (where the root CMakeLists.txt is)
    # -B build : Specifies the build directory (can be any name)
    # -DPERMUTO_ENABLE_TESTING=ON : Explicitly enable building tests (default is ON)
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
    ```
    Alternatively, run the test executable directly:
    ```bash
    ./tests/permuto_tests
    ```
    All tests should pass. Tests cover both interpolation modes and the reverse template functionality.

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

Include the necessary headers and call the `permuto::process` function, optionally configuring interpolation and other settings. For reverse operations, use `permuto::create_reverse_template` and `permuto::apply_reverse_template`.

**Example 1: Default Behavior (Interpolation Enabled)**

```c++
#include <permuto/permuto.hpp>      // Main header for process() and Options
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
        // Default options: enableStringInterpolation = true, onMissingKey = Ignore
        permuto::Options options;
        // options.onMissingKey = permuto::MissingKeyBehavior::Error;
        // options.variableStartMarker = "<<";
        // options.variableEndMarker = ">>";

        // Perform the substitution
        json result = permuto::process(template_json, context, options);

        // Output the result (pretty-printed with 4 spaces)
        std::cout << "--- Result (Interpolation ON) ---" << std::endl;
        std::cout << result.dump(4) << std::endl;

    } catch (const permuto::PermutoException& e) {
        std::cerr << "Permuto Error: " << e.what() << std::endl;
        return 1;
    } catch (const std::invalid_argument& e) {
         std::cerr << "Error: Invalid Options: " << e.what() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Standard Error: " << e.what() << std::endl;
        return 1;
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

**Example 2: Interpolation Disabled & Reverse Template Extraction**

```c++
#include <permuto/permuto.hpp>
#include <permuto/exceptions.hpp>
#include <nlohmann/json.hpp>
#include <iostream>
#include <string>

int main() {
    using json = nlohmann::json;

    // Template with only exact matches and one non-exact string
    json template_json = R"(
        {
            "output_name": "${user.name}",
            "metadata": { "city": "${user.address.city}" },
            "ids": [ "${user.id}", "${system.id}" ],
            "greeting": "Hello ${user.name}" // This will become literal
        }
    )"_json;

    json context = R"(
        {
            "user": { "name": "Bob", "address": { "city": "Metroville" }, "id": 101 },
            "system": { "id": 202 }
        }
    )"_json;

    json expected_result_interpolation_off = R"(
        {
            "output_name": "Bob",
            "metadata": { "city": "Metroville" },
            "ids": [ 101, 202 ],
            "greeting": "Hello ${user.name}" // Treated as literal
        }
    )"_json;

    try {
        permuto::Options options;
        options.enableStringInterpolation = false; // Disable interpolation
        options.onMissingKey = permuto::MissingKeyBehavior::Error; // Be strict for exact matches

        // --- Process with Interpolation OFF ---
        json result = permuto::process(template_json, context, options);
        std::cout << "--- Result (Interpolation OFF) ---" << std::endl;
        std::cout << result.dump(4) << std::endl;
        assert(result == expected_result_interpolation_off); // Verify result

        // --- Create Reverse Template ---
        // Requires options with enableStringInterpolation = false
        json reverse_template = permuto::create_reverse_template(template_json, options);
        std::cout << "\n--- Generated Reverse Template ---" << std::endl;
        std::cout << reverse_template.dump(4) << std::endl;

        // Expected reverse template structure:
        // { "user": {"name": "/output_name", "address": {"city": "/metadata/city"}, "id": "/ids/0"},
        //   "system": {"id": "/ids/1"} }

        // --- Apply Reverse Template to Extract Context ---
        json reconstructed_context = permuto::apply_reverse_template(reverse_template, result);
        std::cout << "\n--- Reconstructed Context ---" << std::endl;
        std::cout << reconstructed_context.dump(4) << std::endl;
        assert(reconstructed_context == context); // Verify context reconstruction

    } catch (const permuto::PermutoException& e) { // Catch base Permuto exceptions
        std::cerr << "Permuto Error: " << e.what() << std::endl;
        return 1;
    } catch (const std::logic_error& e) { // Catch config errors for create_reverse_template
         std::cerr << "Configuration Error: " << e.what() << std::endl;
        return 1;
    } catch (const std::invalid_argument& e) { // Catch option validation errors
         std::cerr << "Error: Invalid Options: " << e.what() << std::endl;
        return 1;
    } catch (const std::exception& e) { // Catch other potential errors (e.g., JSON parsing/lookup)
        std::cerr << "Standard Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
```
*Output (Interpolation OFF & Reverse Ops):*
```json
--- Result (Interpolation OFF) ---
{
    "greeting": "Hello ${user.name}",
    "ids": [
        101,
        202
    ],
    "metadata": {
        "city": "Metroville"
    },
    "output_name": "Bob"
}

--- Generated Reverse Template ---
{
    "system": {
        "id": "/ids/1"
    },
    "user": {
        "address": {
            "city": "/metadata/city"
        },
        "id": "/ids/0",
        "name": "/output_name"
    }
}

--- Reconstructed Context ---
{
    "system": {
        "id": 202
    },
    "user": {
        "address": {
            "city": "Metroville"
        },
        "id": 101,
        "name": "Bob"
    }
}
```


### Command-Line Tool Usage (`permuto`)

The `permuto` executable provides a command-line interface for processing files.

**Synopsis:**

```bash
permuto <template_file> <context_file> [options]
```

**Arguments:**

*   `<template_file>`: Path to the input JSON template file.
*   `<context_file>`: Path to the input JSON data context file.

**Options:**

*   `--on-missing-key=<value>`: Behavior for missing keys/paths.
    *   `ignore`: (Default) Leave placeholders unresolved (or uninterpolated).
    *   `error`: Print an error message to stderr and exit with failure if a key/path lookup fails during exact match resolution.
*   `--no-string-interpolation`: Disable string interpolation. Only exact matches (`"${var}"`) will be substituted. Strings containing interpolation markers but not matching exactly are treated as literals. If omitted, interpolation is enabled (default).
*   `--start=<string>`: Set the variable start delimiter (Default: `${`).
*   `--end=<string>`: Set the variable end delimiter (Default: `}`).
*   `--help`: Display usage information and exit.

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
# Assuming 'permuto' executable is in the PATH or relative path from build dir
./build/cli/permuto template.json context.json
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
./build/cli/permuto template.json context.json --no-string-interpolation
```

**Output (Pretty-printed JSON to stdout):**
```json
{
    "message": "Hello, ${name}!", // Treated as literal
    "setting": true               // Exact match still works
}
```

## Key Concepts & Examples

### 1. Data Type Handling (Exact Match Substitution)

This is the **primary** substitution mechanism when string interpolation is disabled (`--no-string-interpolation` or `enableStringInterpolation = false`), and it also applies to exact matches when interpolation is enabled.

When a template string value consists *only* of a single placeholder (e.g., `"${var}"`, `"${path.to.val}"`), Permuto substitutes the actual JSON data type found at that path in the context. Quotes around the placeholder in the template are effectively removed for non-string types in the output.

| Template Fragment       | Data Context (`context`)                     | Output Fragment (`result`) | Notes                                     |
| :---------------------- | :------------------------------------------- | :------------------------- | :---------------------------------------- |
| `"num": "${data.val}"`  | `{"data": {"val": 123}}`                     | `"num": 123`               | Number type preserved.                    |
| `"bool": "${opts.on}"`  | `{"opts": {"on": true}}`                     | `"bool": true`             | Boolean type preserved.                   |
| `"arr": "${items.list}"` | `{"items": {"list": [1, "a"]}}`              | `"arr": [1, "a"]`          | Array type preserved.                     |
| `"obj": "${cfg.sec}"`   | `{"cfg": {"sec": {"k": "v"}}}`              | `"obj": {"k": "v"}`        | Object type preserved.                    |
| `"null_val": "${maybe}"`| `{"maybe": null}`                            | `"null_val": null`         | Null type preserved.                      |
| `"str_val": "${msg}"`   | `{"msg": "hello"}`                           | `"str_val": "hello"`       | String type preserved, quotes remain.     |
| `"str_num": "${v.s}"`   | `{"v": {"s": "123"}}`                        | `"str_num": "123"`         | Input data was string, output is string.  |

### 2. String Interpolation (Optional Feature)

This behavior only occurs when interpolation is **enabled** (`enableStringInterpolation = true`, the default, and `--no-string-interpolation` is **not** used in CLI).

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
**Output (Interpolation Enabled):**
```json
{
  "message": "User Alice (ID: 123) is true. Count: null. Settings: {\"notify\":false,\"theme\":\"dark\"}"
}
```
**Output (Interpolation Disabled):**
```json
{
  "message": "User ${user.name} (ID: ${user.id}) is ${status.active}. Count: ${data.count}. Settings: ${settings}"
}
```

### 3. Nested Data Access (Dot Notation)

Use dots (`.`) within placeholders to access nested properties in the data context. This works for both exact matches and interpolation (when enabled).

**Template:**
```json
{"city": "${address.city}", "zip": "${address.postal_code}"}
```
**Data:**
```json
{"address": {"street": "123 Main St", "city": "Anytown", "postal_code": "12345"}}
```
**Output (Either Mode):**
```json
{"city": "Anytown", "zip": "12345"}
```

*   **Path Resolution:** The engine converts the dot-separated path (e.g., `address.city`) into a [JSON Pointer](https://datatracker.ietf.org/doc/html/rfc6901) (e.g., `/address/city`) for robust lookup within the nlohmann/json object. Special characters in keys (like `/` and `~`) are handled correctly according to JSON Pointer rules.
*   **Keys with Dots:** Keys in the data context that contain literal dots (e.g., `"user.role": "admin"`) are accessed by treating the dot as part of the key segment in the path (e.g., `${user.role}`).
*   **Array Indexing:** Accessing array elements by index (e.g., `${my_array.0}`) is **not supported** via dot notation.

### 4. Recursive Substitution

Substitution is applied recursively. If a placeholder resolves (via exact match) to a string value that itself contains placeholders, that string is also processed according to the current interpolation mode.

**Template:**
```json
{ "message": "${greeting.template}", "user_info": "${user.details}" }
```
**Data:**
```json
{
  "greeting": { "template": "Hello, ${user.name}!" },
  "user": { "name": "Bob", "details": "Name: ${user.name}" }
}
```
**Output (Interpolation Enabled):**
```json
{ "message": "Hello, Bob!", "user_info": "Name: Bob" }
```
**Output (Interpolation Disabled):**
```json
{ "message": "Hello, ${user.name}!", "user_info": "Name: ${user.name}" }
```
*(Because the resolved values `Hello, ${user.name}!` and `Name: ${user.name}` are not exact matches, they are treated as literals in this mode).*

### 5. Missing Key / Path Handling

Configure how unresolved placeholders are treated using the `onMissingKey` option (or `--on-missing-key` CLI flag). This applies if the initial key is missing, or if any intermediate key in a path does not exist or points to a non-object value when further traversal is expected.

**Template:**
```json
{"value": "${a.b.c}", "another": "${x.y.z}", "interpolated": "Maybe: ${a.b.c}!"}
```
**Data:**
```json
{"a": {"b": { "some_other_key": 1 } } }
```
*(`c` is missing from `a.b`; `x` is missing entirely)*

*   **`permuto::MissingKeyBehavior::Ignore` / `--on-missing-key=ignore` (Default)**
    *   **Output (Interpolation ON):** `{"value": "${a.b.c}", "another": "${x.y.z}", "interpolated": "Maybe: ${a.b.c}!"}` (Placeholder remains untouched in exact match or interpolation).
    *   **Output (Interpolation OFF):** `{"value": "${a.b.c}", "another": "${x.y.z}", "interpolated": "Maybe: ${a.b.c}!"}` (Placeholder remains untouched in exact match; interpolated string treated as literal anyway).

*   **`permuto::MissingKeyBehavior::Error` / `--on-missing-key=error`**
    *   **Output (Either Mode):** *Throws `permuto::PermutoMissingKeyException`* when resolving `"${a.b.c}"`. Processing stops. The exception contains the unresolved path via `.get_key_path()`. Note: When interpolation is OFF, this error only triggers for *failed exact match lookups*. Strings containing placeholders but not matching exactly are treated as literals and do not trigger lookups or errors.

### 6. Custom Delimiters

Change the markers used to identify variables via the `Options` struct or CLI flags. This applies regardless of interpolation mode.

**C++ Configuration:**
```c++
permuto::Options options;
options.variableStartMarker = "<<";
options.variableEndMarker = ">>";
```
**CLI Configuration:** `--start="<<" --end=">>"`

### 7. Cycle Detection

Permuto prevents infinite loops caused by placeholders resolving cyclically, including indirect cycles through multiple lookups. This works regardless of interpolation mode. Throws `permuto::PermutoCycleException`.

### 8. Reverse Template Extraction (Interpolation Disabled Only)

This feature allows you to extract the original context data if you have the `result_json` and the `original_template`, but **only** if the result was generated with `enableStringInterpolation = false`.

**Concept:** The `original_template` defines where context variables *should* appear in the `result_json`. A `reverse_template` is generated that maps the context variable paths to JSON Pointers indicating their location in the `result_json`.

**Process:**

1.  **Generate Reverse Template:**
    *   Call `permuto::create_reverse_template(original_template, options)` where `options.enableStringInterpolation` is `false`.
    *   This function scans `original_template` for exact-match placeholders (`"${context.path}"`).
    *   It builds a JSON structure mirroring the expected *context*, where leaf values are JSON Pointer strings pointing to the corresponding location in the *result*.
    *   Throws `std::logic_error` if `options.enableStringInterpolation` is `true`.

2.  **Apply Reverse Template:**
    *   Call `permuto::apply_reverse_template(reverse_template, result_json)`.
    *   This function traverses the `reverse_template`.
    *   For each leaf node (JSON Pointer string), it looks up the value at that pointer location within `result_json`.
    *   It builds the `reconstructed_context` JSON, placing the extracted values at the correct paths defined by the `reverse_template` structure.

**Example:**

*   **Original Template `T` (Interpolation OFF must be used):**
    ```json
    {
      "output_name": "${user.name}",
      "metadata": { "city": "${user.address.city}" },
      "ids": [ "${user.id}" ]
    }
    ```
*   **Generated Reverse Template `RT = create_reverse_template(T, opts_interp_off)`:**
    ```json
    {
      "user": {
        "name": "/output_name",
        "address": { "city": "/metadata/city" },
        "id": "/ids/0"
      }
    }
    ```
*   **Result JSON `R` (produced by `process(T, C, opts_interp_off)`):**
    ```json
    {
      "output_name": "Alice",
      "metadata": { "city": "Wonderland" },
      "ids": [ 99 ]
    }
    ```
*   **Reconstructed Context `C_new = apply_reverse_template(RT, R)`:**
    ```json
    {
      "user": {
        "name": "Alice",
        "address": { "city": "Wonderland" },
        "id": 99
      }
    }
    ```

**Limitations:** This extraction only works if interpolation was disabled during the original `process` call and only recovers context variables referenced by exact-match placeholders in the template.

## Configuration Options

These options are passed via the `permuto::Options` struct in C++ or via command-line flags to the CLI tool.

| C++ Option Member           | CLI Flag                    | Description                                                                   | Default Value | C++ Type/Enum                  |
| :-------------------------- | :-------------------------- | :---------------------------------------------------------------------------- | :------------ | :----------------------------- |
| `variableStartMarker`     | `--start=<string>`          | The starting delimiter for placeholders.                                      | `${`          | `std::string`                  |
| `variableEndMarker`       | `--end=<string>`            | The ending delimiter for placeholders.                                        | `}`           | `std::string`                  |
| `onMissingKey`            | `--on-missing-key=`         | Behavior for unresolved placeholders (`ignore`/`error`).                      | `ignore`      | `permuto::MissingKeyBehavior` |
| `enableStringInterpolation` | `--no-string-interpolation` | If `true` (default), interpolates variables in strings. If `false` (CLI flag present), only exact matches (`"${var}"`) are substituted. Reverse templates require `false`. | `true`        | `bool`                         |

## C++ API Details

The primary C++ API is defined in `<permuto/permuto.hpp>` and `<permuto/exceptions.hpp>`.

*   **`permuto::Options` struct:**
    *   `std::string variableStartMarker = "${";`
    *   `std::string variableEndMarker = "}";`
    *   `permuto::MissingKeyBehavior onMissingKey = permuto::MissingKeyBehavior::Ignore;`
    *   `bool enableStringInterpolation = true;` **(New)**
    *   `void validate() const;` (Throws `std::invalid_argument` on bad options)

*   **`permuto::MissingKeyBehavior` enum class:**
    *   `Ignore`
    *   `Error`

*   **`permuto::process` function:**
    ```c++
    nlohmann::json process(
        const nlohmann::json& template_json,
        const nlohmann::json& context,
        const permuto::Options& options = {} // Default options used if omitted
    );
    ```
    Processes the template based on options (including interpolation mode). Throws exceptions on errors.

*   **`permuto::create_reverse_template` function (New):**
    ```c++
    nlohmann::json create_reverse_template(
        const nlohmann::json& original_template,
        const permuto::Options& options = {}
    );
    ```
    Generates the reverse template mapping. Requires `options.enableStringInterpolation` to be `false`. Throws `std::logic_error` if interpolation is enabled. Throws `std::invalid_argument` if options are invalid.

*   **`permuto::apply_reverse_template` function (New):**
    ```c++
    nlohmann::json apply_reverse_template(
        const nlohmann::json& reverse_template,
        const nlohmann::json& result_json
    );
    ```
    Uses the `reverse_template` and `result_json` to reconstruct the context data. Throws `nlohmann::json` exceptions (e.g., `out_of_range`) if `result_json` doesn't match the structure expected by the pointers in `reverse_template`.

*   **Exception Classes (in `<permuto/exceptions.hpp>`):**
    *   `permuto::PermutoException` (Base class, inherits `std::runtime_error`)
    *   `permuto::PermutoParseException` (Currently unused, JSON parsing handled by nlohmann/json upstream)
    *   `permuto::PermutoCycleException` (Inherits `PermutoException`, provides `const std::string& get_cycle_path() const`)
    *   `permuto::PermutoMissingKeyException` (Inherits `PermutoException`, provides `const std::string& get_key_path() const`)
    *   `std::invalid_argument` is thrown by `permuto::process` or `permuto::create_reverse_template` if `options` are invalid.
    *   `std::logic_error` (or similar) is thrown by `permuto::create_reverse_template` if `enableStringInterpolation` is `true`.

## Using Permuto in another CMake Project

After installing Permuto (using `cmake --install`), you can easily integrate it into your own CMake project using `find_package`.

1.  **Update your `CMakeLists.txt`:** (No changes needed here compared to before)
    ```cmake
    # ... (find_package, add_executable, etc.)
    target_link_libraries(my_app PRIVATE Permuto::permuto-lib)
    ```

2.  **Include and use in your C++ code:** Use the updated API as shown in the examples above.

## Contributing

Contributions are welcome! Please adhere to the following guidelines:

*   Follow standard C++17 best practices.
*   Maintain code style and formatting consistency.
*   Add unit tests (in `tests/`) for any new features or bug fixes, covering both interpolation modes and reverse template functionality.
*   Ensure all tests pass (`ctest --output-on-failure` in the build directory).
*   Consider opening an issue first to discuss significant changes or new features.
*   Submit changes via Pull Requests.

## License

This work is released into the **Public Domain** (Unlicense).

You are free to copy, modify, publish, use, compile, sell, or distribute this software, either in source code form or as a compiled binary, for any purpose, commercial or non-commercial, and by any means.

See the accompanying `LICENSE` file for the full Unlicense text. While attribution is not legally required, it is appreciated.
```

