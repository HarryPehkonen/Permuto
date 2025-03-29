# Clavis

A lightweight, flexible library for generating JSON structures by substituting variables from a data context into a valid JSON template. It supports accessing nested data using dot notation and handles type conversions intelligently.

Inspired by the need to dynamically create JSON payloads, such as those required for various LLM (Large Language Model) APIs, Clavis provides a focused templating mechanism. It is *not* a full programming language but excels at structured JSON generation based on input data.

## Features

*   **Valid JSON Templates:** Uses standard, parseable JSON as the template input format.
*   **Variable Substitution:** Replaces placeholders (e.g., `${variable}`) with values from a provided data context.
*   **Nested Data Access (Dot Notation):** Access values deep within the data context using dot notation (e.g., `${user.profile.email}`). Array indexing is not supported.
*   **Automatic Type Handling:** Intelligently handles data types. When a placeholder like `"${variable}"` or `"${path.to.variable}"` is the *entire* string value in the template, and the corresponding data is a number, boolean, array, object, or null, the quotes are effectively removed in the output, preserving the correct JSON type. String data results in a standard JSON string output.
*   **String Interpolation:** Substitutes variables (including nested ones) within larger strings (e.g., `"Hello, ${user.name}!"`). Non-string values are converted to their string representation in this case.
*   **Recursive Substitution:** Recursively processes substituted values that themselves contain placeholders.
*   **Cycle Detection:** Automatically detects and prevents infinite recursion loops caused by cyclical references in the data context (e.g., `{"a": "${b}", "b": "${a}"}` or involving paths), throwing an error instead.
*   **Configurable Missing Key Handling:** Choose whether to `ignore` missing keys (or paths) (leaving the placeholder in the output) or `error` out. This applies to both top-level keys and keys accessed during path traversal.
*   **Customizable Delimiters:** Define custom start and end markers for variables instead of the default `${` and `}`.
*   **Language Agnostic Specification:** This document defines the behavior; implementations can exist in various programming languages.

## Installation

Installation instructions will depend on the specific language implementation (e.g., `npm install clavis-js`, `pip install pyclavis`, `composer require your-vendor/clavis-php`, etc.). Please refer to the documentation for the specific language package of Clavis.

## Usage

### Basic Example

The core operation involves a JSON template and a data object (context).

**Template (Input - Must be Valid JSON):**

```json
{
    "user_email": "${user.email}",
    "greeting": "Welcome, ${user.name}!",
    "primary_setting": "${settings.primary}",
    "secondary_enabled": "${settings.secondary.enabled}",
    "deep_value": "${a.b.c.d}",
    "maybe_present": "${user.optional_field}"
}
```

**Data (Context):**

```json
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
```

**Processing (Conceptual):**

```javascript
// Pseudo-code
// Assuming 'clavis' is the imported library object/module
result = clavis.process(templateJson, dataContext, { onMissingKey: 'ignore' });
```

**Output (Resulting JSON Structure):**

```json
{
    "user_email": "alice@example.com",
    "greeting": "Welcome, Alice!",
    "primary_setting": "ThemeA",
    "secondary_enabled": true,
    "deep_value": 12345,
    "maybe_present": "${user.optional_field}"
}
```

### Key Concepts & Examples

#### 1. Data Type Handling (Exact Match Substitution)

When a template string value consists *only* of a single placeholder (which can include dot notation), Clavis substitutes the raw data type found at that path.

| Template Fragment       | Data Context                                | Output Fragment         | Notes                                     |
| :---------------------- | :------------------------------------------ | :---------------------- | :---------------------------------------- |
| `"num": "${data.val}"`  | `{"data": {"val": 123}}`                    | `"num": 123`            | Number type preserved, quotes removed.    |
| `"bool": "${opts.on}"`  | `{"opts": {"on": true}}`                    | `"bool": true`          | Boolean type preserved, quotes removed.   |
| `"arr": "${items.list}"` | `{"items": {"list": [1, "a"]}}`             | `"arr": [1, "a"]`       | Array type preserved, quotes removed.     |
| `"obj": "${cfg.section}"`| `{"cfg": {"section": {"k": "v"}}}`         | `"obj": {"k": "v"}`     | Object type preserved, quotes removed.    |
| `"null_val": "${maybe}"`| `{"maybe": null}`                           | `"null_val": null`      | Null type preserved, quotes removed.      |
| `"str_val": "${msg}"`   | `{"msg": "hello"}`                          | `"str_val": "hello"`    | String type preserved, quotes remain.     |
| `"str_num": "${v.s}"`   | `{"v": {"s": "123"}}`                       | `"str_num": "123"`      | Input data was string, output is string. |

#### 2. String Interpolation

When a placeholder (including dot notation) is part of a larger string, the data value found at that path is converted to its string representation and inserted.

**Template:**

```json
{
  "message": "User ${user.name} (ID: ${user.id}) is ${status.active}. Count: ${data.count}"
}
```

**Data:**

```json
{
  "user": {"name": "Alice", "id": 123},
  "status": {"active": true},
  "data": {"count": null}
}
```

**Output:**

```json
{
  "message": "User Alice (ID: 123) is true. Count: null"
}
```
*(Note: Exact string representation of `null`, arrays, objects might vary slightly between language implementations, but should be consistent.)*

#### 3. Nested Data Access (Dot Notation)

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

*   **Path Resolution:** Dots always indicate nesting. The engine splits `address.city` into `['address', 'city']` and traverses the data context accordingly.
*   **Keys with Dots:** Keys in the data context that contain literal dots (e.g., `"user.role": "admin"`) cannot be accessed reliably using simple dot notation because the dot will be interpreted as a path separator. If needed, such values must be accessed differently (e.g., by ensuring they are at the top level or pre-processing the context).
*   **Array Indexing:** Accessing array elements by index (e.g., `${my_array.0}`) is **not supported**.

#### 4. Recursive Substitution

Substitution is applied recursively, including within values accessed via dot notation.

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

#### 5. Missing Key / Path Handling

Configure how undefined variables or unresolved paths are treated using the `onMissingKey` option. This applies if the initial key is missing, or if any intermediate key in a path does not exist or points to a non-object value (like a string or number) when further traversal is expected.

**Template:**
```json
{"value": "${a.b.c}", "another": "${x.y.z}"}
```
**Data:**
```json
{"a": {"b": { "some_other_key": 1 } } }
```
*(`c` is missing from `a.b`; `x` is missing entirely)*

*   **`onMissingKey: 'ignore'` (Default)**
    *   **Output:** `{"value": "${a.b.c}", "another": "${x.y.z}"}`
    *   The placeholder remains untouched if *any* part of the path resolution fails. Useful for multi-pass templating.

*   **`onMissingKey: 'error'`**
    *   **Output:** *Throws an Error/Exception*
    *   Processing stops, indicating the first path that could not be fully resolved (e.g., "Key 'c' not found at path 'a.b'" or "Key 'x' not found").

#### 6. Custom Delimiters

Change the markers used to identify variables.

**Configuration:** `{ startMarker: '<<', endMarker: '>>' }`

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
*Validation:* The library must ensure `startMarker` and `endMarker` are not identical.

#### 7. Cycle Detection

Clavis prevents infinite loops, including those involving path lookups.

**Template:**
```json
{"a": "${path.to.b}"}
```
**Data:**
```json
{"path": {"to": {"b": "${ cycled.back.to.a}" } }, "cycled": {"back": {"to": {"a": "${path.to.b}"}}}}
```

**Processing:** Attempting to process this template and data will result in an **Error/Exception** being thrown due to the cyclical dependency.

## Configuration Options

These options are typically passed during the processing call:

*   `variableStartMarker` (String): The starting delimiter for placeholders. Default: `${`.
*   `variableEndMarker` (String): The ending delimiter for placeholders. Default: `}`.
*   `onMissingKey` (String): Defines behavior when a variable or path cannot be resolved in the data context.
    *   `'ignore'`: Leave the placeholder string as-is in the output. (Default)
    *   `'error'`: Throw an error/exception.

## API (Language Specific)

Please refer to the specific language implementation of Clavis for detailed API documentation, including:

*   Function/method names and signatures.
*   Parameter types and order.
*   Return values.
*   Specific error/exception types thrown.

**Example (Conceptual Pseudo-code):**

```javascript
// Example signature (might vary by language)
function process(
    template: string | object,  // Input JSON template (string or pre-parsed object)
    context: object,           // Data object for substitutions
    options?: {                // Optional configuration
        variableStartMarker?: string,
        variableEndMarker?: string,
        onMissingKey?: 'ignore' | 'error'
    }
): object | string // Output JSON (typically as parsed object, maybe string option)

// Throws errors on parsing failures, cycles, unresolved paths (if onMissingKey='error').
```

## Contributing

Contributions to specific language implementations of Clavis are welcome! Please refer to the `CONTRIBUTING.md` file (if available) in the specific implementation's repository.

## License

This work is released into the **Public Domain**.

You are free to use, modify, distribute, and sublicense this work without any restrictions. See the accompanying `LICENSE` file for more details (e.g., referencing the Unlicense or CC0). While attribution is not legally required, it is appreciated.

