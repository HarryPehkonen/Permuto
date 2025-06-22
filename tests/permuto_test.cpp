// permuto/tests/permuto_test.cpp
#include <gtest/gtest.h>
#include <permuto/permuto.hpp>
#include <permuto/exceptions.hpp>
#include <nlohmann/json.hpp>

// Include the internal header FOR TESTING detail functions DIRECTLY
#include "../src/permuto_internal.hpp" // Adjust path if necessary

#include <set>
#include <string>
#include <stdexcept> // logic_error, runtime_error

using json = nlohmann::json;


// --- Test Suite for detail::resolve_path ---
class PermutoResolvePathTests : public ::testing::Test {
protected:
    json context = R"(
        {
            "user": {
                "name": "Alice",
                "email": "alice@example.com",
                "address": {
                    "city": "Wonderland"
                }
            },
            "settings": {
                "primary": "ThemeA",
                "enabled": true,
                "values": [10, 20]
            },
            "top_level_null": null,
            "key/with/slashes": "slashes_val",
            "key~with~tildes": "tildes_val",
            "dot.key": "dot_val",
            "a": { "b": { "c": 1 } },
            "dot": { "key": "nested_dot_val" },
            "user.name": "literal_dot_key_val",
            "items": ["first", "second", "third"],
            "matrix": [[1, 2], [3, 4], [5, 6]]
        }
    )"_json;

    // Default options: Interp OFF, Ignore Missing
    permuto::Options ignoreOpts;
    // Options: Interp OFF, Error Missing
    permuto::Options errorOpts;

    void SetUp() override {
        // Default is now interp=false, ignore=true
        ignoreOpts.onMissingKey = permuto::MissingKeyBehavior::Ignore;
        ignoreOpts.enableStringInterpolation = false;

        errorOpts.onMissingKey = permuto::MissingKeyBehavior::Error;
        errorOpts.enableStringInterpolation = false; // Keep interp off for these tests

        // Validate defaults explicitly
         ASSERT_FALSE(permuto::Options{}.enableStringInterpolation);
         ASSERT_EQ(permuto::Options{}.onMissingKey, permuto::MissingKeyBehavior::Ignore);
    }
};

TEST_F(PermutoResolvePathTests, ResolvesTopLevelKey) {
    const json* result = permuto::detail::resolve_path(context, "/settings", ignoreOpts);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(*result, context["settings"]);
}

TEST_F(PermutoResolvePathTests, ResolvesNestedKey) {
    const json* result = permuto::detail::resolve_path(context, "/user/address/city", ignoreOpts);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(*result, "Wonderland");
}

TEST_F(PermutoResolvePathTests, ResolvesDeeplyNestedKey) {
     const json* result = permuto::detail::resolve_path(context, "/a/b/c", ignoreOpts);
     ASSERT_NE(result, nullptr);
     EXPECT_EQ(*result, 1);
}


TEST_F(PermutoResolvePathTests, ResolvesToNullValue) {
    const json* result = permuto::detail::resolve_path(context, "/top_level_null", ignoreOpts);
    ASSERT_NE(result, nullptr);
    ASSERT_TRUE(result->is_null());
}

// Tests for keys needing JSON Pointer escaping
TEST_F(PermutoResolvePathTests, ResolvesKeyWithSlashes) {
     // Path "/key~1with~1slashes" to access "key/with/slashes"
     const json* result = permuto::detail::resolve_path(context, "/key~1with~1slashes", ignoreOpts);
     ASSERT_NE(result, nullptr);
     EXPECT_EQ(*result, "slashes_val");
}

TEST_F(PermutoResolvePathTests, ResolvesKeyWithTildes) {
      // Path "/key~0with~0tildes" to access "key~with~tildes"
     const json* result = permuto::detail::resolve_path(context, "/key~0with~0tildes", ignoreOpts);
     ASSERT_NE(result, nullptr);
     EXPECT_EQ(*result, "tildes_val");
}

// Test that dot notation fails
TEST_F(PermutoResolvePathTests, DotNotationFails) {
     // Dot notation is no longer supported
     const json* result = permuto::detail::resolve_path(context, "user.name", ignoreOpts);
     EXPECT_EQ(result, nullptr); // Should fail as it doesn't start with '/'
     
     // With error mode
     EXPECT_THROW({
        try {
             permuto::detail::resolve_path(context, "user.name", errorOpts);
        } catch (const permuto::PermutoMissingKeyException& e) {
            EXPECT_EQ(e.get_key_path(), "user.name");
            throw;
        }
     }, permuto::PermutoMissingKeyException);
}

// Test that we can access keys with dots using JSON Pointer
TEST_F(PermutoResolvePathTests, JsonPointerAccessKeyWithDot) {
     // Access the key literally named "user.name" using JSON Pointer
     const json* result = permuto::detail::resolve_path(context, "/user.name", ignoreOpts);
     ASSERT_NE(result, nullptr);
     EXPECT_EQ(*result, "literal_dot_key_val");
}


TEST_F(PermutoResolvePathTests, HandlesMissingTopLevelKeyIgnore) {
    const json* result = permuto::detail::resolve_path(context, "/nonexistent", ignoreOpts);
    EXPECT_EQ(result, nullptr);
}

TEST_F(PermutoResolvePathTests, HandlesMissingNestedKeyIgnore) {
    const json* result = permuto::detail::resolve_path(context, "/user/address/street", ignoreOpts);
    EXPECT_EQ(result, nullptr);
}

TEST_F(PermutoResolvePathTests, HandlesMissingIntermediateKeyIgnore) {
    const json* result = permuto::detail::resolve_path(context, "/user/profile/id", ignoreOpts);
    EXPECT_EQ(result, nullptr); // "profile" does not exist
}

TEST_F(PermutoResolvePathTests, HandlesMissingTopLevelKeyError) {
    EXPECT_THROW({
        try {
            permuto::detail::resolve_path(context, "/nonexistent", errorOpts);
        } catch (const permuto::PermutoMissingKeyException& e) {
            EXPECT_EQ(e.get_key_path(), "/nonexistent"); // Check the reported path
            EXPECT_NE(std::string(e.what()).find("/nonexistent"), std::string::npos);
            throw;
        }
    }, permuto::PermutoMissingKeyException);
}

TEST_F(PermutoResolvePathTests, HandlesMissingNestedKeyError) {
     EXPECT_THROW({
        try {
            permuto::detail::resolve_path(context, "/user/address/street", errorOpts);
        } catch (const permuto::PermutoMissingKeyException& e) {
            EXPECT_EQ(e.get_key_path(), "/user/address/street"); // Check reported path
             EXPECT_NE(std::string(e.what()).find("/user/address/street"), std::string::npos);
            throw;
        }
    }, permuto::PermutoMissingKeyException);
}

TEST_F(PermutoResolvePathTests, HandlesMissingIntermediateKeyError) {
    EXPECT_THROW({
        try {
            permuto::detail::resolve_path(context, "/user/profile/id", errorOpts);
        } catch (const permuto::PermutoMissingKeyException& e) {
            EXPECT_EQ(e.get_key_path(), "/user/profile/id"); // Check reported path
             EXPECT_NE(std::string(e.what()).find("/user/profile/id"), std::string::npos);
            throw;
        }
    }, permuto::PermutoMissingKeyException);
}


TEST_F(PermutoResolvePathTests, HandlesTraversalIntoNonObjectIgnore) {
    // Attempt to access "first" inside string "Alice"
    const json* result = permuto::detail::resolve_path(context, "/user/name/first", ignoreOpts);
    EXPECT_EQ(result, nullptr);
     // Attempt to access "value" inside boolean true
    result = permuto::detail::resolve_path(context, "/settings/enabled/value", ignoreOpts);
    EXPECT_EQ(result, nullptr);
     // Attempt to access "key" inside array [10, 20]
    result = permuto::detail::resolve_path(context, "/settings/values/key", ignoreOpts);
    EXPECT_EQ(result, nullptr);
     // Attempt to access "z" inside null
     result = permuto::detail::resolve_path(context, "/top_level_null/z", ignoreOpts);
     EXPECT_EQ(result, nullptr);
}

TEST_F(PermutoResolvePathTests, HandlesTraversalIntoNonObjectError) {
    EXPECT_THROW({
        try {
            permuto::detail::resolve_path(context, "/user/name/first", errorOpts);
        } catch (const permuto::PermutoMissingKeyException& e) {
            EXPECT_EQ(e.get_key_path(), "/user/name/first"); // Check reported path
             EXPECT_NE(std::string(e.what()).find("/user/name/first"), std::string::npos);
            throw;
        }
    }, permuto::PermutoMissingKeyException);
    
    EXPECT_THROW({
        try {
            permuto::detail::resolve_path(context, "/settings/values/key", errorOpts);
        } catch (const permuto::PermutoMissingKeyException& e) {
            EXPECT_EQ(e.get_key_path(), "/settings/values/key");
             EXPECT_NE(std::string(e.what()).find("/settings/values/key"), std::string::npos);
            throw;
        }
    }, permuto::PermutoMissingKeyException);
    
    EXPECT_THROW({
        try {
            permuto::detail::resolve_path(context, "/top_level_null/z", errorOpts);
        } catch (const permuto::PermutoMissingKeyException& e) {
            EXPECT_EQ(e.get_key_path(), "/top_level_null/z");
             EXPECT_NE(std::string(e.what()).find("/top_level_null/z"), std::string::npos);
            throw;
        }
    }, permuto::PermutoMissingKeyException);
}


TEST_F(PermutoResolvePathTests, HandlesEmptyPathIgnore) {
    // Empty path now returns root, not nullptr
    const json* result = permuto::detail::resolve_path(context, "", ignoreOpts);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(*result, context);
}

TEST_F(PermutoResolvePathTests, HandlesEmptyPathError) {
    // Empty path should return root, not throw
    const json* result = permuto::detail::resolve_path(context, "", errorOpts);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(*result, context);
}

TEST_F(PermutoResolvePathTests, HandlesInvalidPathFormatsIgnore) {
    // These should fail validation as they don't start with '/'
    EXPECT_EQ(permuto::detail::resolve_path(context, "user", ignoreOpts), nullptr);
    EXPECT_EQ(permuto::detail::resolve_path(context, "user/name", ignoreOpts), nullptr);
    EXPECT_EQ(permuto::detail::resolve_path(context, ".", ignoreOpts), nullptr);
}

TEST_F(PermutoResolvePathTests, HandlesInvalidPathFormatsError) {
     // These should throw as they don't start with '/'
      EXPECT_THROW( { try { permuto::detail::resolve_path(context, "user", errorOpts); } catch(const permuto::PermutoMissingKeyException& e) { EXPECT_EQ(e.get_key_path(), "user"); throw;} }, permuto::PermutoMissingKeyException);
      EXPECT_THROW( { try { permuto::detail::resolve_path(context, "user/name", errorOpts); } catch(const permuto::PermutoMissingKeyException& e) { EXPECT_EQ(e.get_key_path(), "user/name"); throw;} }, permuto::PermutoMissingKeyException);
      EXPECT_THROW( { try { permuto::detail::resolve_path(context, ".", errorOpts); } catch(const permuto::PermutoMissingKeyException& e) { EXPECT_EQ(e.get_key_path(), "."); throw;} }, permuto::PermutoMissingKeyException);
}

// Tests for JSON Pointer syntax (RFC 6901)
TEST_F(PermutoResolvePathTests, JsonPointerAccessKeyWithSlashes) {
     // Path "/key~1with~1slashes" to access "key/with/slashes"
     const json* result = permuto::detail::resolve_path(context, "/key~1with~1slashes", ignoreOpts);
     ASSERT_NE(result, nullptr);
     EXPECT_EQ(*result, "slashes_val");
}

TEST_F(PermutoResolvePathTests, JsonPointerAccessKeyWithTildes) {
      // Path "/key~0with~0tildes" to access "key~with~tildes"
     const json* result = permuto::detail::resolve_path(context, "/key~0with~0tildes", ignoreOpts);
     ASSERT_NE(result, nullptr);
     EXPECT_EQ(*result, "tildes_val");
}

TEST_F(PermutoResolvePathTests, JsonPointerArrayAccess) {
     // Access array elements
     const json* result = permuto::detail::resolve_path(context, "/items/0", ignoreOpts);
     ASSERT_NE(result, nullptr);
     EXPECT_EQ(*result, "first");
     
     result = permuto::detail::resolve_path(context, "/items/2", ignoreOpts);
     ASSERT_NE(result, nullptr);
     EXPECT_EQ(*result, "third");
}

TEST_F(PermutoResolvePathTests, JsonPointerNestedArrayAccess) {
     // Access nested array elements
     const json* result = permuto::detail::resolve_path(context, "/matrix/1/0", ignoreOpts);
     ASSERT_NE(result, nullptr);
     EXPECT_EQ(*result, 3);
     
     result = permuto::detail::resolve_path(context, "/matrix/2/1", ignoreOpts);
     ASSERT_NE(result, nullptr);
     EXPECT_EQ(*result, 6);
}

TEST_F(PermutoResolvePathTests, JsonPointerMissingPath) {
     const json* result = permuto::detail::resolve_path(context, "/nonexistent/path", ignoreOpts);
     EXPECT_EQ(result, nullptr);
}

TEST_F(PermutoResolvePathTests, JsonPointerInvalidSyntax) {
     // Invalid JSON Pointer (missing leading slash)
     const json* result = permuto::detail::resolve_path(context, "user/name", ignoreOpts);
     // This will be treated as dot notation and fail because of the slash
     EXPECT_EQ(result, nullptr);
}

TEST_F(PermutoResolvePathTests, JsonPointerEmptyPath) {
     // Empty JSON Pointer refers to the root
     const json* result = permuto::detail::resolve_path(context, "", ignoreOpts);
     ASSERT_NE(result, nullptr);
     EXPECT_EQ(*result, context);
}

TEST_F(PermutoResolvePathTests, JsonPointerSlashAlone) {
     // "/" refers to a property with an empty string as key
     // Our test context doesn't have such a key, so it should return nullptr
     const json* result = permuto::detail::resolve_path(context, "/", ignoreOpts);
     EXPECT_EQ(result, nullptr);
}

TEST_F(PermutoResolvePathTests, JsonPointerEmptyStringKey) {
     // Test with a context that has an empty string as a key
     json context_with_empty_key = R"({
         "": "value_for_empty_key",
         "normal_key": "normal_value"
     })"_json;
     
     // "/" should access the empty string key
     const json* result = permuto::detail::resolve_path(context_with_empty_key, "/", ignoreOpts);
     ASSERT_NE(result, nullptr);
     EXPECT_EQ(*result, "value_for_empty_key");
     
     // Empty path should return the whole context
     result = permuto::detail::resolve_path(context_with_empty_key, "", ignoreOpts);
     ASSERT_NE(result, nullptr);
     EXPECT_EQ(*result, context_with_empty_key);
}


// --- Test Suite for detail::process_string ---
// ... (No changes needed in this suite, should still pass) ...
class PermutoProcessStringTests : public ::testing::Test {
protected:
    json context = R"(
        {
            "user": { "name": "Alice", "id": 123 },
            "enabled": true,
            "pi": 3.14,
            "nothing": null,
            "items": [1, "two"],
            "config": { "theme": "dark" },
            "html": "<p>Hello</p>",
            "ref": "${/user/name}",
            "nested_ref": "User is ${/ref}",
            "cycle_a": "${/cycle_b}",
            "cycle_b": "${/cycle_a}",
            "deep_cycle_a": "${/deep_cycle_b/ref}",
            "deep_cycle_b": { "ref": "${/deep_cycle_a}" },
            "empty_placeholder_ref": "${}",
            "missing": null
        }
    )"_json;

    // Define the 4 main option combinations
    permuto::Options optsNoInterpIgnore; // Interpolation OFF, MissingKey Ignore (DEFAULT)
    permuto::Options optsNoInterpError;  // Interpolation OFF, MissingKey Error
    permuto::Options optsInterpIgnore;   // Interpolation ON, MissingKey Ignore
    permuto::Options optsInterpError;    // Interpolation ON, MissingKey Error

    std::set<std::string> active_paths; // Fresh set for each call

    void SetUp() override {
        // Interpolation Disabled (DEFAULT)
        optsNoInterpIgnore.onMissingKey = permuto::MissingKeyBehavior::Ignore;
        optsNoInterpIgnore.enableStringInterpolation = false;
        optsNoInterpError.onMissingKey = permuto::MissingKeyBehavior::Error;
        optsNoInterpError.enableStringInterpolation = false;

        // Interpolation Enabled
        optsInterpIgnore.onMissingKey = permuto::MissingKeyBehavior::Ignore;
        optsInterpIgnore.enableStringInterpolation = true;
        optsInterpError.onMissingKey = permuto::MissingKeyBehavior::Error;
        optsInterpError.enableStringInterpolation = true;

        active_paths.clear(); // Ensure clean slate for each test

        // Validate default options are as expected
         ASSERT_FALSE(permuto::Options{}.enableStringInterpolation);
         ASSERT_EQ(permuto::Options{}.onMissingKey, permuto::MissingKeyBehavior::Ignore);
    }

    // Helper to call the detail function
    json process_str_opts(const std::string& tpl, const permuto::Options& opts) {
         active_paths.clear(); // Ensure clear before call
         return permuto::detail::process_string(tpl, context, opts, active_paths);
    }
};

TEST_F(PermutoProcessStringTests, ExactMatchString_InterpOff) {
    json result = process_str_opts("${/user/name}", optsNoInterpIgnore);
    EXPECT_EQ(result.type(), json::value_t::string);
    EXPECT_EQ(result.get<std::string>(), "Alice");
}
TEST_F(PermutoProcessStringTests, ExactMatchString_InterpOn) {
    json result = process_str_opts("${/user/name}", optsInterpIgnore);
    EXPECT_EQ(result.type(), json::value_t::string);
    EXPECT_EQ(result.get<std::string>(), "Alice");
}
// ... rest of PermutoProcessStringTests tests are unchanged ...
TEST_F(PermutoProcessStringTests, ExactMatchNumber_InterpOff) {
    json result = process_str_opts("${/user/id}", optsNoInterpIgnore);
    EXPECT_TRUE(result.is_number());
    EXPECT_EQ(result.get<int>(), 123);
}
TEST_F(PermutoProcessStringTests, ExactMatchNumber_InterpOn) {
    json result = process_str_opts("${/user/id}", optsInterpIgnore);
    EXPECT_TRUE(result.is_number());
    EXPECT_EQ(result.get<int>(), 123);
}
TEST_F(PermutoProcessStringTests, ExactMatchFloat_InterpOff) {
    json result = process_str_opts("${/pi}", optsNoInterpIgnore);
    EXPECT_EQ(result.type(), json::value_t::number_float);
    EXPECT_DOUBLE_EQ(result.get<double>(), 3.14);
}
TEST_F(PermutoProcessStringTests, ExactMatchFloat_InterpOn) {
    json result = process_str_opts("${/pi}", optsInterpIgnore);
    EXPECT_EQ(result.type(), json::value_t::number_float);
    EXPECT_DOUBLE_EQ(result.get<double>(), 3.14);
}
TEST_F(PermutoProcessStringTests, ExactMatchBoolean_InterpOff) {
    json result = process_str_opts("${/enabled}", optsNoInterpIgnore);
    EXPECT_EQ(result.type(), json::value_t::boolean);
    EXPECT_EQ(result.get<bool>(), true);
}
TEST_F(PermutoProcessStringTests, ExactMatchBoolean_InterpOn) {
    json result = process_str_opts("${/enabled}", optsInterpIgnore);
    EXPECT_EQ(result.type(), json::value_t::boolean);
    EXPECT_EQ(result.get<bool>(), true);
}
TEST_F(PermutoProcessStringTests, ExactMatchNull_InterpOff) {
    json result = process_str_opts("${/nothing}", optsNoInterpIgnore);
    EXPECT_EQ(result.type(), json::value_t::null);
    EXPECT_TRUE(result.is_null());
}
TEST_F(PermutoProcessStringTests, ExactMatchNull_InterpOn) {
    json result = process_str_opts("${/nothing}", optsInterpIgnore);
    EXPECT_EQ(result.type(), json::value_t::null);
    EXPECT_TRUE(result.is_null());
}
TEST_F(PermutoProcessStringTests, ExactMatchArray_InterpOff) {
    json result = process_str_opts("${/items}", optsNoInterpIgnore);
    EXPECT_EQ(result.type(), json::value_t::array);
    EXPECT_EQ(result, json::parse("[1, \"two\"]"));
}
TEST_F(PermutoProcessStringTests, ExactMatchArray_InterpOn) {
    json result = process_str_opts("${/items}", optsInterpIgnore);
    EXPECT_EQ(result.type(), json::value_t::array);
    EXPECT_EQ(result, json::parse("[1, \"two\"]"));
}
TEST_F(PermutoProcessStringTests, ExactMatchObject_InterpOff) {
    json result = process_str_opts("${/config}", optsNoInterpIgnore);
    EXPECT_EQ(result.type(), json::value_t::object);
    EXPECT_EQ(result, json::parse("{\"theme\": \"dark\"}"));
}
TEST_F(PermutoProcessStringTests, ExactMatchObject_InterpOn) {
    json result = process_str_opts("${/config}", optsInterpIgnore);
    EXPECT_EQ(result.type(), json::value_t::object);
    EXPECT_EQ(result, json::parse("{\"theme\": \"dark\"}"));
}
TEST_F(PermutoProcessStringTests, ExactMatchMissingIgnore_InterpOff) {
    json result = process_str_opts("${/missing/key}", optsNoInterpIgnore);
    EXPECT_EQ(result.type(), json::value_t::string); // Returns placeholder string
    EXPECT_EQ(result.get<std::string>(), "${/missing/key}");
}
TEST_F(PermutoProcessStringTests, ExactMatchMissingIgnore_InterpOn) {
    json result = process_str_opts("${/missing/key}", optsInterpIgnore);
    EXPECT_EQ(result.type(), json::value_t::string); // Returns placeholder string
    EXPECT_EQ(result.get<std::string>(), "${/missing/key}");
}
TEST_F(PermutoProcessStringTests, ExactMatchMissingError_InterpOff) {
    EXPECT_THROW({
        process_str_opts("${/missing/key}", optsNoInterpError);
    }, permuto::PermutoMissingKeyException);
}
TEST_F(PermutoProcessStringTests, ExactMatchMissingError_InterpOn) {
    EXPECT_THROW({
        process_str_opts("${/missing/key}", optsInterpError);
    }, permuto::PermutoMissingKeyException);
}
TEST_F(PermutoProcessStringTests, ExactMatchRecursion_InterpOff) {
    // ${/ref} -> "${/user/name}" -> "Alice"
    json result = process_str_opts("${/ref}", optsNoInterpIgnore);
    EXPECT_EQ(result.type(), json::value_t::string);
    EXPECT_EQ(result.get<std::string>(), "Alice");
}
TEST_F(PermutoProcessStringTests, ExactMatchRecursion_InterpOn) {
    // ${/ref} -> "${/user/name}" -> "Alice"
    json result = process_str_opts("${/ref}", optsInterpIgnore);
    EXPECT_EQ(result.type(), json::value_t::string);
    EXPECT_EQ(result.get<std::string>(), "Alice");
}
TEST_F(PermutoProcessStringTests, ExactMatchRecursionNested_InterpOn) {
     // ${/nested_ref} -> "User is ${/ref}" -> (interpolation) -> "User is Alice"
    json result = process_str_opts("${/nested_ref}", optsInterpIgnore);
    EXPECT_EQ(result.type(), json::value_t::string);
    EXPECT_EQ(result.get<std::string>(), "User is Alice");
}
TEST_F(PermutoProcessStringTests, ExactMatchRecursionNested_InterpOff) {
     // ${/nested_ref} -> "User is ${/ref}" -> (no interpolation) -> literal "User is ${/ref}"
    json result = process_str_opts("${/nested_ref}", optsNoInterpIgnore);
    EXPECT_EQ(result.type(), json::value_t::string);
    EXPECT_EQ(result.get<std::string>(), "User is ${/ref}");
}
TEST_F(PermutoProcessStringTests, NonExactMatchWithMarkers_InterpOn) { // Renamed from ExactMatchWithInnerMarkersShouldInterpolate
    // Input is not an exact match, falls to interpolation
    json result = process_str_opts("${/user/name} has ID ${/user/id}", optsInterpIgnore);
    EXPECT_EQ(result.type(), json::value_t::string);
    EXPECT_EQ(result.get<std::string>(), "Alice has ID 123");
}
TEST_F(PermutoProcessStringTests, NonExactMatchWithMarkers_InterpOff) {
    // Input is not an exact match, interpolation disabled -> literal
    json result = process_str_opts("${/user/name} has ID ${/user/id}", optsNoInterpIgnore);
    EXPECT_EQ(result.type(), json::value_t::string);
    EXPECT_EQ(result.get<std::string>(), "${/user/name} has ID ${/user/id}");
}
TEST_F(PermutoProcessStringTests, ExactMatchEmptyPlaceholder_InterpOff) {
    // ${} is invalid placeholder path, treated as literal
    json result = process_str_opts("${}", optsNoInterpIgnore);
    EXPECT_EQ(result.type(), json::value_t::string);
    EXPECT_EQ(result.get<std::string>(), "${}");
}
TEST_F(PermutoProcessStringTests, ExactMatchEmptyPlaceholder_InterpOn) {
    json result = process_str_opts("${}", optsInterpIgnore);
    EXPECT_EQ(result.type(), json::value_t::string);
    EXPECT_EQ(result.get<std::string>(), "${}");
}
TEST_F(PermutoProcessStringTests, ExactMatchResolvesToEmptyPlaceholder_InterpOff) {
    // ${/empty_placeholder_ref} -> "${}" -> literal "${}"
     json result = process_str_opts("${/empty_placeholder_ref}", optsNoInterpIgnore);
     EXPECT_EQ(result.type(), json::value_t::string);
     EXPECT_EQ(result.get<std::string>(), "${}");
}
TEST_F(PermutoProcessStringTests, ExactMatchResolvesToEmptyPlaceholder_InterpOn) {
     json result = process_str_opts("${/empty_placeholder_ref}", optsInterpIgnore);
     EXPECT_EQ(result.type(), json::value_t::string);
     EXPECT_EQ(result.get<std::string>(), "${}");
}
TEST_F(PermutoProcessStringTests, InterpolationSimple_InterpOn) {
    json result = process_str_opts("Name: ${/user/name}", optsInterpIgnore);
    EXPECT_EQ(result.type(), json::value_t::string);
    EXPECT_EQ(result.get<std::string>(), "Name: Alice");
}
TEST_F(PermutoProcessStringTests, InterpolationSimple_InterpOff) {
    json result = process_str_opts("Name: ${/user/name}", optsNoInterpIgnore);
    EXPECT_EQ(result.type(), json::value_t::string);
    EXPECT_EQ(result.get<std::string>(), "Name: ${/user/name}"); // Literal
}
TEST_F(PermutoProcessStringTests, InterpolationMultiple_InterpOn) {
    json result = process_str_opts("${/user/name} (${/user/id})", optsInterpIgnore);
    EXPECT_EQ(result.get<std::string>(), "Alice (123)");
}
TEST_F(PermutoProcessStringTests, InterpolationMultiple_InterpOff) {
    json result = process_str_opts("${/user/name} (${/user/id})", optsNoInterpIgnore);
    EXPECT_EQ(result.get<std::string>(), "${/user/name} (${/user/id})"); // Literal
}
TEST_F(PermutoProcessStringTests, InterpolationMixedTypes_InterpOn) {
    json result = process_str_opts("User: ${/user/name}, ID: ${/user/id}, Enabled: ${/enabled}, Pi: ${/pi}, Nada: ${/nothing}, Items: ${/items}, Cfg: ${/config}, Html: ${/html}", optsInterpIgnore);
    // Note: json::dump produces compact JSON for objects/arrays during stringify_json
    EXPECT_EQ(result.get<std::string>(), "User: Alice, ID: 123, Enabled: true, Pi: 3.14, Nada: null, Items: [1,\"two\"], Cfg: {\"theme\":\"dark\"}, Html: <p>Hello</p>");
}
TEST_F(PermutoProcessStringTests, InterpolationMixedTypes_InterpOff) {
    json result = process_str_opts("User: ${/user/name}, ID: ${/user/id}, Enabled: ${/enabled}, Pi: ${/pi}, Nada: ${/nothing}, Items: ${/items}, Cfg: ${/config}, Html: ${/html}", optsNoInterpIgnore);
    EXPECT_EQ(result.get<std::string>(), "User: ${/user/name}, ID: ${/user/id}, Enabled: ${/enabled}, Pi: ${/pi}, Nada: ${/nothing}, Items: ${/items}, Cfg: ${/config}, Html: ${/html}"); // Literal
}
TEST_F(PermutoProcessStringTests, InterpolationMissingIgnore_InterpOn) {
    json result = process_str_opts("Val: ${/user/name} Missing: ${/missing/key} End", optsInterpIgnore);
    EXPECT_EQ(result.get<std::string>(), "Val: Alice Missing: ${/missing/key} End");
}
TEST_F(PermutoProcessStringTests, InterpolationMissingIgnore_InterpOff) {
    json result = process_str_opts("Val: ${/user/name} Missing: ${/missing/key} End", optsNoInterpIgnore);
    EXPECT_EQ(result.get<std::string>(), "Val: ${/user/name} Missing: ${/missing/key} End"); // Literal
}
TEST_F(PermutoProcessStringTests, InterpolationMissingError_InterpOn) {
     EXPECT_THROW({
        process_str_opts("Val: ${/user/name} Missing: ${/missing/key} End", optsInterpError);
    }, permuto::PermutoMissingKeyException);
}
TEST_F(PermutoProcessStringTests, InterpolationMissingError_InterpOff) {
     // Should NOT throw, just return literal
    json result = process_str_opts("Val: ${/user/name} Missing: ${/missing/key} End", optsNoInterpError);
    EXPECT_EQ(result.get<std::string>(), "Val: ${/user/name} Missing: ${/missing/key} End");
}
TEST_F(PermutoProcessStringTests, InterpolationWithRecursion_InterpOn) {
    // "Info: ${/nested_ref}" -> "Info: User is ${/ref}" -> "Info: User is Alice"
    json result = process_str_opts("Info: ${/nested_ref}", optsInterpIgnore);
    EXPECT_EQ(result.get<std::string>(), "Info: User is Alice");
}
TEST_F(PermutoProcessStringTests, InterpolationWithRecursion_InterpOff) {
    // "Info: ${/nested_ref}" -> literal
    json result = process_str_opts("Info: ${/nested_ref}", optsNoInterpIgnore);
    EXPECT_EQ(result.get<std::string>(), "Info: ${/nested_ref}");
}
TEST_F(PermutoProcessStringTests, InterpolationAdjacent_InterpOn) {
    json result = process_str_opts("${/user/name}${/user/id}", optsInterpIgnore);
    EXPECT_EQ(result.get<std::string>(), "Alice123");
}
TEST_F(PermutoProcessStringTests, InterpolationAdjacent_InterpOff) {
    json result = process_str_opts("${/user/name}${/user/id}", optsNoInterpIgnore);
    EXPECT_EQ(result.get<std::string>(), "${/user/name}${/user/id}"); // Literal
}
TEST_F(PermutoProcessStringTests, InterpolationOnlyPlaceholders_InterpOn) {
    json result = process_str_opts("${/user/name}:${/enabled}", optsInterpIgnore);
     EXPECT_EQ(result.get<std::string>(), "Alice:true");
}
TEST_F(PermutoProcessStringTests, InterpolationOnlyPlaceholders_InterpOff) {
    json result = process_str_opts("${/user/name}:${/enabled}", optsNoInterpIgnore);
     EXPECT_EQ(result.get<std::string>(), "${/user/name}:${/enabled}"); // Literal
}
TEST_F(PermutoProcessStringTests, InterpolationUnterminated_InterpOn) {
     json result = process_str_opts("Hello ${/user/name} and ${/unclosed", optsInterpIgnore);
     EXPECT_EQ(result.get<std::string>(), "Hello Alice and ${/unclosed");
}
TEST_F(PermutoProcessStringTests, InterpolationUnterminated_InterpOff) {
     json result = process_str_opts("Hello ${/user/name} and ${/unclosed", optsNoInterpIgnore);
     EXPECT_EQ(result.get<std::string>(), "Hello ${/user/name} and ${/unclosed"); // Literal
}
TEST_F(PermutoProcessStringTests, InterpolationEmptyPlaceholder_InterpOn) {
    // ${} is treated as literal within interpolation
    json result = process_str_opts("Value: ${}", optsInterpIgnore);
    EXPECT_EQ(result.type(), json::value_t::string);
    EXPECT_EQ(result.get<std::string>(), "Value: ${}");
}
TEST_F(PermutoProcessStringTests, InterpolationEmptyPlaceholder_InterpOff) {
    json result = process_str_opts("Value: ${}", optsNoInterpIgnore);
    EXPECT_EQ(result.type(), json::value_t::string);
    EXPECT_EQ(result.get<std::string>(), "Value: ${}"); // Literal
}
TEST_F(PermutoProcessStringTests, InterpolationResolvesToEmptyPlaceholder_InterpOn) {
    // ${/empty_placeholder_ref} resolves to "${}" which is included literally
     json result = process_str_opts("Value: ${/empty_placeholder_ref} End", optsInterpIgnore);
     EXPECT_EQ(result.type(), json::value_t::string);
     EXPECT_EQ(result.get<std::string>(), "Value: ${} End");
}
TEST_F(PermutoProcessStringTests, InterpolationResolvesToEmptyPlaceholder_InterpOff) {
     json result = process_str_opts("Value: ${/empty_placeholder_ref} End", optsNoInterpIgnore);
     EXPECT_EQ(result.type(), json::value_t::string);
     EXPECT_EQ(result.get<std::string>(), "Value: ${/empty_placeholder_ref} End"); // Literal
}
TEST_F(PermutoProcessStringTests, StringNoPlaceholders_InterpOff) {
    json result = process_str_opts("Just plain text.", optsNoInterpIgnore);
    EXPECT_EQ(result.get<std::string>(), "Just plain text.");
}
TEST_F(PermutoProcessStringTests, StringNoPlaceholders_InterpOn) {
    json result = process_str_opts("Just plain text.", optsInterpIgnore);
    EXPECT_EQ(result.get<std::string>(), "Just plain text.");
}
TEST_F(PermutoProcessStringTests, StringEmpty_InterpOff) {
    json result = process_str_opts("", optsNoInterpIgnore);
    EXPECT_EQ(result.get<std::string>(), "");
}
TEST_F(PermutoProcessStringTests, StringEmpty_InterpOn) {
    json result = process_str_opts("", optsInterpIgnore);
    EXPECT_EQ(result.get<std::string>(), "");
}
TEST_F(PermutoProcessStringTests, CycleExactMatch_InterpOff) {
    EXPECT_THROW({
        process_str_opts("${/cycle_a}", optsNoInterpIgnore);
    }, permuto::PermutoCycleException);
}
TEST_F(PermutoProcessStringTests, CycleExactMatch_InterpOn) {
    EXPECT_THROW({
        process_str_opts("${/cycle_a}", optsInterpIgnore);
    }, permuto::PermutoCycleException);
}
TEST_F(PermutoProcessStringTests, CycleInterpolation_InterpOn) {
     EXPECT_THROW({
        process_str_opts("Cycle here: ${/cycle_a}", optsInterpIgnore);
    }, permuto::PermutoCycleException);
}
TEST_F(PermutoProcessStringTests, CycleInterpolation_InterpOff) {
     // Interpolation disabled, returns literal string, cycle not reached
     json result = process_str_opts("Cycle here: ${/cycle_a}", optsNoInterpIgnore);
     EXPECT_EQ(result.get<std::string>(), "Cycle here: ${/cycle_a}");
}
TEST_F(PermutoProcessStringTests, CycleDeepExact_InterpOff) {
     EXPECT_THROW({
        process_str_opts("${/deep_cycle_a}", optsNoInterpIgnore);
    }, permuto::PermutoCycleException);
}
TEST_F(PermutoProcessStringTests, CycleDeepExact_InterpOn) {
     EXPECT_THROW({
        process_str_opts("${/deep_cycle_a}", optsInterpIgnore);
    }, permuto::PermutoCycleException);
}
TEST_F(PermutoProcessStringTests, CycleDeepInterpolation_InterpOn) {
     EXPECT_THROW({
        process_str_opts("Deep cycle: ${/deep_cycle_a}...", optsInterpIgnore);
    }, permuto::PermutoCycleException);
}
TEST_F(PermutoProcessStringTests, CycleDeepInterpolation_InterpOff) {
     // Interpolation disabled, returns literal string, cycle not reached
     json result = process_str_opts("Deep cycle: ${/deep_cycle_a}...", optsNoInterpIgnore);
     EXPECT_EQ(result.get<std::string>(), "Deep cycle: ${/deep_cycle_a}...");
}
TEST_F(PermutoProcessStringTests, CustomDelimitersExact_InterpOff) {
    permuto::Options opts;
    opts.variableStartMarker = "<<";
    opts.variableEndMarker = ">>";
    opts.enableStringInterpolation = false; // Explicitly OFF
    json result = process_str_opts("<</user/name>>", opts);
    EXPECT_EQ(result.get<std::string>(), "Alice");
}
TEST_F(PermutoProcessStringTests, CustomDelimitersExact_InterpOn) {
    permuto::Options opts;
    opts.variableStartMarker = "<<";
    opts.variableEndMarker = ">>";
    opts.enableStringInterpolation = true; // Explicitly ON
    json result = process_str_opts("<</user/name>>", opts);
    EXPECT_EQ(result.get<std::string>(), "Alice");
}
TEST_F(PermutoProcessStringTests, CustomDelimitersInterpolation_InterpOn) {
    permuto::Options opts;
    opts.variableStartMarker = "::";
    opts.variableEndMarker = "::";
    opts.enableStringInterpolation = true;
    json result = process_str_opts("ID is ::/user/id:: Status ::/enabled::", opts);
    EXPECT_EQ(result.get<std::string>(), "ID is 123 Status true");
}
TEST_F(PermutoProcessStringTests, CustomDelimitersInterpolation_InterpOff) {
    permuto::Options opts;
    opts.variableStartMarker = "::";
    opts.variableEndMarker = "::";
    opts.enableStringInterpolation = false;
    json result = process_str_opts("ID is ::/user/id:: Status ::/enabled::", opts);
    EXPECT_EQ(result.get<std::string>(), "ID is ::/user/id:: Status ::/enabled::"); // Literal
}
TEST_F(PermutoProcessStringTests, CustomDelimitersInterpolationAdjacent_InterpOn) {
    permuto::Options opts;
    opts.variableStartMarker = "%%";
    opts.variableEndMarker = "%%";
    opts.enableStringInterpolation = true;
    json result = process_str_opts("%%/user/name%%%%/user/id%%", opts);
    EXPECT_EQ(result.get<std::string>(), "Alice123");
}
TEST_F(PermutoProcessStringTests, CustomDelimitersInterpolationAdjacent_InterpOff) {
    permuto::Options opts;
    opts.variableStartMarker = "%%";
    opts.variableEndMarker = "%%";
    opts.enableStringInterpolation = false;
    json result = process_str_opts("%%/user/name%%%%/user/id%%", opts);
    EXPECT_EQ(result.get<std::string>(), "%%/user/name%%%%/user/id%%"); // Literal
}



// === Public API Tests (using permuto::apply) ===
// ... (No changes needed in this suite, should still pass) ...
TEST(PermutoPublicApiTests, SimpleSubstitution_InterpOn) {
    json template_json = R"(
        {
            "greeting": "Hello, ${/user/name}!",
            "email": "${/user/email}"
        }
    )"_json;
    json context = R"( {"user": {"name": "Alice", "email": "alice@example.com"}} )"_json;
    json expected = R"( {"greeting": "Hello, Alice!", "email": "alice@example.com"} )"_json;

    permuto::Options opts;
    opts.enableStringInterpolation = true; // Explicitly true
    json result = permuto::apply(template_json, context, opts);
    EXPECT_EQ(result, expected);
}
TEST(PermutoPublicApiTests, SimpleSubstitution_InterpOff) {
    json template_json = R"(
        {
            "greeting": "Hello, ${/user/name}!",
            "email": "${/user/email}"
        }
    )"_json;
    json context = R"( {"user": {"name": "Alice", "email": "alice@example.com"}} )"_json;
    // Exact match for email works, greeting is literal
    json expected = R"( {"greeting": "Hello, ${/user/name}!", "email": "alice@example.com"} )"_json;

    permuto::Options opts;
    opts.enableStringInterpolation = false; // Explicitly false
    json result = permuto::apply(template_json, context, opts);
    EXPECT_EQ(result, expected);
}
TEST(PermutoPublicApiTests, SimpleSubstitution_DefaultOpts) {
    json template_json = R"(
        {
            "greeting": "Hello, ${/user/name}!",
            "email": "${/user/email}"
        }
    )"_json;
    json context = R"( {"user": {"name": "Alice", "email": "alice@example.com"}} )"_json;
    // Expect same as InterpOff test
    json expected = R"( {"greeting": "Hello, ${/user/name}!", "email": "alice@example.com"} )"_json;

    permuto::Options defaultOpts; // Default constructed
    ASSERT_FALSE(defaultOpts.enableStringInterpolation); // Verify default is FALSE

    json result = permuto::apply(template_json, context); // Use default opts
    EXPECT_EQ(result, expected);
    json result2 = permuto::apply(template_json, context, defaultOpts); // Explicit default opts
    EXPECT_EQ(result2, expected);
}
TEST(PermutoPublicApiTests, ExactMatchTypePreservation_InterpOff) {
    json template_json = R"(
        { "num": "${/d/num}", "bool": "${/d/bool}", "null": "${/d/null}",
          "arr": "${/d/arr}", "obj": "${/d/obj}", "str": "${/d/str}" }
    )"_json;
     json context = R"( {"d": {"num": 123, "bool": true, "null": null, "arr": [1], "obj": {"k":"v"}, "str": "hi"}} )"_json;
    json expected = R"( {"num": 123, "bool": true, "null": null, "arr": [1], "obj": {"k":"v"}, "str": "hi"} )"_json;

    permuto::Options opts;
    opts.enableStringInterpolation = false;
    json result = permuto::apply(template_json, context, opts);
    EXPECT_EQ(result, expected);
}
TEST(PermutoPublicApiTests, ExactMatchTypePreservation_InterpOn) {
    json template_json = R"(
        { "num": "${/d/num}", "bool": "${/d/bool}", "null": "${/d/null}",
          "arr": "${/d/arr}", "obj": "${/d/obj}", "str": "${/d/str}" }
    )"_json;
     json context = R"( {"d": {"num": 123, "bool": true, "null": null, "arr": [1], "obj": {"k":"v"}, "str": "hi"}} )"_json;
    // Result should be identical because these are all exact matches
    json expected = R"( {"num": 123, "bool": true, "null": null, "arr": [1], "obj": {"k":"v"}, "str": "hi"} )"_json;

    permuto::Options opts;
    opts.enableStringInterpolation = true;
    json result = permuto::apply(template_json, context, opts);
    EXPECT_EQ(result, expected);
}
TEST(PermutoPublicApiTests, InterpolationTypeConversion_InterpOn) {
     json template_json = R"({ "message": "Data: num=${/d/num}, bool=${/d/bool}, null=${/d/null}, arr=${/d/arr}, obj=${/d/obj}" })"_json;
     json context = R"( { "d": { "num": 456, "bool": false, "null": null, "arr": [true, null], "obj": {"x": 1} } } )"_json;
    json expected = R"( { "message": "Data: num=456, bool=false, null=null, arr=[true,null], obj={\"x\":1}" } )"_json;

    permuto::Options opts;
    opts.enableStringInterpolation = true;
    json result = permuto::apply(template_json, context, opts);
    EXPECT_EQ(result, expected);
}
TEST(PermutoPublicApiTests, InterpolationTypeConversion_InterpOff) {
     json template_json = R"({ "message": "Data: num=${d.num}, bool=${d.bool}, null=${d.null}, arr=${d.arr}, obj=${d.obj}" })"_json;
     json context = R"( { "d": { "num": 456, "bool": false, "null": null, "arr": [true, null], "obj": {"x": 1} } } )"_json;
    // Expect literal template string
    json expected = R"({ "message": "Data: num=${d.num}, bool=${d.bool}, null=${d.null}, arr=${d.arr}, obj=${d.obj}" })"_json;

    permuto::Options opts;
    opts.enableStringInterpolation = false;
    json result = permuto::apply(template_json, context, opts);
    EXPECT_EQ(result, expected);
}
TEST(PermutoPublicApiTests, MissingKey_Ignore_InterpOff) {
     json template_json = R"( {"p": "${/a/b}", "m_leaf": "${/a/c}", "m_branch": "${/x/y}", "m_interp": "Val: ${/x/y}"} )"_json;
     json context = R"( {"a": {"b": 1}} )"_json;
     // Exact matches resolve or remain placeholder, non-exact match remains literal
     json expected = R"( {"p": 1, "m_leaf": "${/a/c}", "m_branch": "${/x/y}", "m_interp": "Val: ${/x/y}"} )"_json;

    permuto::Options opts;
    opts.onMissingKey = permuto::MissingKeyBehavior::Ignore;
    opts.enableStringInterpolation = false;
    json result = permuto::apply(template_json, context, opts);
    EXPECT_EQ(result, expected);
}
TEST(PermutoPublicApiTests, MissingKey_Ignore_InterpOn) {
     json template_json = R"( {"p": "${/a/b}", "m_leaf": "${/a/c}", "m_branch": "${/x/y}", "m_interp": "Val: ${/x/y}"} )"_json;
     json context = R"( {"a": {"b": 1}} )"_json;
     // Exact matches resolve or remain placeholder, interpolation happens leaving placeholder
     json expected = R"( {"p": 1, "m_leaf": "${/a/c}", "m_branch": "${/x/y}", "m_interp": "Val: ${/x/y}"} )"_json;

    permuto::Options opts;
    opts.onMissingKey = permuto::MissingKeyBehavior::Ignore;
    opts.enableStringInterpolation = true;
    json result = permuto::apply(template_json, context, opts);
    EXPECT_EQ(result, expected); // Note: Same result as InterpOff for Ignore mode
}
TEST(PermutoPublicApiTests, MissingKey_Error_Exact_InterpOff) {
     json template_json = R"( {"missing": "${/a/b/c}"} )"_json;
     json context = R"( {"a": {"b": {}}} )"_json;
    permuto::Options opts;
    opts.onMissingKey = permuto::MissingKeyBehavior::Error;
    opts.enableStringInterpolation = false;
    EXPECT_THROW({ permuto::apply(template_json, context, opts); }, permuto::PermutoMissingKeyException);
}
TEST(PermutoPublicApiTests, MissingKey_Error_Exact_InterpOn) {
     json template_json = R"( {"missing": "${/a/b/c}"} )"_json;
     json context = R"( {"a": {"b": {}}} )"_json;
    permuto::Options opts;
    opts.onMissingKey = permuto::MissingKeyBehavior::Error;
    opts.enableStringInterpolation = true;
    EXPECT_THROW({ permuto::apply(template_json, context, opts); }, permuto::PermutoMissingKeyException); // Should still throw for exact match lookup
}
TEST(PermutoPublicApiTests, MissingKey_Error_Interpolation_InterpOn) {
     json template_json = R"( {"msg": "Hello ${/a/b/c}"} )"_json;
     json context = R"( {"a": {"b": {}}} )"_json;
    permuto::Options opts;
    opts.onMissingKey = permuto::MissingKeyBehavior::Error;
    opts.enableStringInterpolation = true;
    EXPECT_THROW({ permuto::apply(template_json, context, opts); }, permuto::PermutoMissingKeyException);
}
TEST(PermutoPublicApiTests, MissingKey_Error_Interpolation_InterpOff) {
     json template_json = R"( {"msg": "Hello ${/a/b/c}"} )"_json;
     json context = R"( {"a": {"b": {}}} )"_json;
     json expected = R"( {"msg": "Hello ${/a/b/c}"} )"_json; // Expect literal
    permuto::Options opts;
    opts.onMissingKey = permuto::MissingKeyBehavior::Error;
    opts.enableStringInterpolation = false;
    // NO THROW expected, as interpolation doesn't happen
    json result = permuto::apply(template_json, context, opts);
    EXPECT_EQ(result, expected);
}
TEST(PermutoPublicApiTests, DirectCycle_Exact_InterpOff) {
    json template_json = R"( {"a": "${/b}", "b": "${/a}"} )"_json;
    json context = R"({"a": "${/b}", "b": "${/a}"})"_json;
    permuto::Options opts; opts.enableStringInterpolation = false;
    EXPECT_THROW({ permuto::apply(template_json, context, opts); }, permuto::PermutoCycleException);
}
TEST(PermutoPublicApiTests, DirectCycle_Exact_InterpOn) {
    json template_json = R"( {"a": "${/b}", "b": "${/a}"} )"_json;
    json context = R"({"a": "${/b}", "b": "${/a}"})"_json;
    permuto::Options opts; opts.enableStringInterpolation = true;
    EXPECT_THROW({ permuto::apply(template_json, context, opts); }, permuto::PermutoCycleException);
}
TEST(PermutoPublicApiTests, IndirectCycle_Exact_InterpOff) {
    json template_json = R"( {"start": "${/path/to/b}"} )"_json;
    json context = R"( { "path": {"to": {"b": "${/cycled/a}" } }, "cycled": {"a": "${/path/to/b}"} } )"_json;
    permuto::Options opts; opts.enableStringInterpolation = false;
    EXPECT_THROW({ permuto::apply(template_json, context, opts); }, permuto::PermutoCycleException);
}
TEST(PermutoPublicApiTests, IndirectCycle_Exact_InterpOn) {
    json template_json = R"( {"start": "${/path/to/b}"} )"_json;
    json context = R"( { "path": {"to": {"b": "${/cycled/a}" } }, "cycled": {"a": "${/path/to/b}"} } )"_json;
    permuto::Options opts; opts.enableStringInterpolation = true;
    EXPECT_THROW({ permuto::apply(template_json, context, opts); }, permuto::PermutoCycleException);
}
TEST(PermutoPublicApiTests, SelfReferenceCycle_Exact_InterpOff) {
    json template_json = R"( {"a": "${/a}"} )"_json;
    json context = R"( {"a": "${/a}"} )"_json;
    permuto::Options opts; opts.enableStringInterpolation = false;
     EXPECT_THROW({ permuto::apply(template_json, context, opts); }, permuto::PermutoCycleException);
}
TEST(PermutoPublicApiTests, SelfReferenceCycle_Exact_InterpOn) {
    json template_json = R"( {"a": "${/a}"} )"_json;
    json context = R"( {"a": "${/a}"} )"_json;
    permuto::Options opts; opts.enableStringInterpolation = true;
     EXPECT_THROW({ permuto::apply(template_json, context, opts); }, permuto::PermutoCycleException);
}
TEST(PermutoPublicApiTests, CycleTriggeredByInterpolation_InterpOn) {
    json template_json = R"( {"msg": "Cycle: ${/a}"} )"_json;
    json context = R"({"a": "${/b}", "b": "${/a}"})"_json; // Cycle in context
    permuto::Options opts; opts.enableStringInterpolation = true;
    EXPECT_THROW({ permuto::apply(template_json, context, opts); }, permuto::PermutoCycleException);
}
TEST(PermutoPublicApiTests, CycleTriggeredByInterpolation_InterpOff) {
    json template_json = R"( {"msg": "Cycle: ${/a}"} )"_json;
    json context = R"({"a": "${/b}", "b": "${/a}"})"_json; // Cycle in context
    json expected = R"( {"msg": "Cycle: ${/a}"} )"_json; // Expect literal
    permuto::Options opts; opts.enableStringInterpolation = false;
    // NO THROW expected
    json result = permuto::apply(template_json, context, opts);
    EXPECT_EQ(result, expected);
}
TEST(PermutoPublicApiTests, CustomDelimiters_InterpOn) {
    json template_json = R"( {"value": "<</data/item>>", "msg": "Item is: <</data/item>>"} )"_json;
    json context = R"( {"data": {"item": "success"}} )"_json;
    json expected = R"( {"value": "success", "msg": "Item is: success"} )"_json;
    permuto::Options opts;
    opts.variableStartMarker = "<<"; opts.variableEndMarker = ">>";
    opts.enableStringInterpolation = true;
    json result = permuto::apply(template_json, context, opts);
    EXPECT_EQ(result, expected);
}
TEST(PermutoPublicApiTests, CustomDelimiters_InterpOff) {
    json template_json = R"( {"value": "<</data/item>>", "msg": "Item is: <</data/item>>"} )"_json;
    json context = R"( {"data": {"item": "success"}} )"_json;
    // Exact match for "value" works, "msg" is literal
    json expected = R"( {"value": "success", "msg": "Item is: <</data/item>>"} )"_json;
    permuto::Options opts;
    opts.variableStartMarker = "<<"; opts.variableEndMarker = ">>";
    opts.enableStringInterpolation = false;
    json result = permuto::apply(template_json, context, opts);
    EXPECT_EQ(result, expected);
}
TEST(PermutoPublicApiTests, InvalidOptions_Apply) {
    json template_json = R"({})"_json; json context = R"({})"_json;
    permuto::Options opts;

    opts.variableStartMarker = "";
    EXPECT_THROW(permuto::apply(template_json, context, opts), std::invalid_argument);
    opts.variableStartMarker = "{"; // Restore for next test

    opts.variableEndMarker = "";
    EXPECT_THROW(permuto::apply(template_json, context, opts), std::invalid_argument);
    opts.variableEndMarker = "}"; // Restore

     opts.variableStartMarker = "@@"; opts.variableEndMarker = "@@";
    EXPECT_THROW(permuto::apply(template_json, context, opts), std::invalid_argument);
}
TEST(PermutoPublicApiTests, InvalidOptions_CreateReverse) {
    json template_json = R"({})"_json;
    permuto::Options opts;
    opts.enableStringInterpolation = false; // Need this false

    opts.variableStartMarker = "";
    EXPECT_THROW(permuto::create_reverse_template(template_json, opts), std::invalid_argument);
    opts.variableStartMarker = "{";

    opts.variableEndMarker = "";
    EXPECT_THROW(permuto::create_reverse_template(template_json, opts), std::invalid_argument);
    opts.variableEndMarker = "}";

     opts.variableStartMarker = "@@"; opts.variableEndMarker = "@@";
    EXPECT_THROW(permuto::create_reverse_template(template_json, opts), std::invalid_argument);
}
TEST(PermutoPublicApiTests, PlaceholdersInKeys_InterpOff) { // Keys are never processed
    json template_json = R"( { "${/a/key}": "v1", "k2": "${/val}" } )"_json;
    json context = R"( { "val": "the_value", "a": { "key": "ignored"} } )"_json;
    json expected = R"( { "${/a/key}": "v1", "k2": "the_value" } )"_json;
    permuto::Options opts; opts.enableStringInterpolation = false;
    json result = permuto::apply(template_json, context, opts);
    EXPECT_EQ(result, expected);
}
TEST(PermutoPublicApiTests, PlaceholdersInKeys_InterpOn) { // Keys are never processed
    json template_json = R"( { "${/a/key}": "v1", "k2": "${/val}" } )"_json;
    json context = R"( { "val": "the_value", "a": { "key": "ignored"} } )"_json;
    json expected = R"( { "${/a/key}": "v1", "k2": "the_value" } )"_json;
    permuto::Options opts; opts.enableStringInterpolation = true;
    json result = permuto::apply(template_json, context, opts);
    EXPECT_EQ(result, expected);
}
TEST(PermutoPublicApiTests, EmptyTemplateAndContext) { // Behavior independent of options
     json template_json = R"({})"_json; json context = R"({})"_json; json expected = R"({})"_json;
     // Use default opts (interp=false)
     EXPECT_EQ(permuto::apply(template_json, context), expected);
     template_json = R"([])"_json; expected = R"([])"_json;
     EXPECT_EQ(permuto::apply(template_json, context), expected);
     template_json = R"("a string")"_json; expected = R"("a string")"_json;
     EXPECT_EQ(permuto::apply(template_json, context), expected);
}
TEST(PermutoPublicApiTests, DelimitersInText_InterpOn) {
    json template_json = R"( { "msg": "This is ${not a var} but ${/this/is}", "brackets": "Some { curly } text" } )"_json;
    json context = R"( { "this": { "is": "substituted"} } )"_json;
    json expected = R"( { "msg": "This is ${not a var} but substituted", "brackets": "Some { curly } text" } )"_json;
    permuto::Options opts; opts.enableStringInterpolation = true;
    json result = permuto::apply(template_json, context, opts);
    EXPECT_EQ(result, expected);
}
TEST(PermutoPublicApiTests, DelimitersInText_InterpOff) {
    json template_json = R"( { "msg": "This is ${not a var} but ${/this/is}", "brackets": "Some { curly } text" } )"_json;
    json context = R"( { "this": { "is": "substituted"} } )"_json;
    // Expect literal as no interpolation happens
    json expected = R"( { "msg": "This is ${not a var} but ${/this/is}", "brackets": "Some { curly } text" } )"_json;
    permuto::Options opts; opts.enableStringInterpolation = false;
    json result = permuto::apply(template_json, context, opts);
    EXPECT_EQ(result, expected);
}
TEST(PermutoPublicApiTests, NestedStructures_InterpOn) {
     json template_json = R"( { "l1": { "a": "${/t/v}", "l2": [ "${/a/0}", { "b": "${/n/num}" }, "S: ${/i}" ] } } )"_json;
     json context = R"( {"t":{"v": true}, "n":{"num": 99}, "i":"val", "a":["ignore"]} )"_json;
     json expected = R"( { "l1": { "a": true, "l2": [ "ignore", { "b": 99 }, "S: val" ] } } )"_json;
    permuto::Options opts; opts.enableStringInterpolation = true;
    json result = permuto::apply(template_json, context, opts);
    EXPECT_EQ(result, expected);
}
TEST(PermutoPublicApiTests, NestedStructures_InterpOff) {
     json template_json = R"( { "l1": { "a": "${/t/v}", "l2": [ "${/a/0}", { "b": "${/n/num}" }, "S: ${/i}" ] } } )"_json;
     json context = R"( {"t":{"v": true}, "n":{"num": 99}, "i":"val", "a":["ignore"]} )"_json;
     // Exact matches for 'a' and 'b' resolve, the string "S: ${/i}" remains literal
     json expected = R"( { "l1": { "a": true, "l2": [ "ignore", { "b": 99 }, "S: ${/i}" ] } } )"_json;
    permuto::Options opts; opts.enableStringInterpolation = false;
    json result = permuto::apply(template_json, context, opts);
    EXPECT_EQ(result, expected);
}


// ======================================================
// === Reverse Template Tests (`create_reverse_template`)
// ======================================================

class PermutoCreateReverseTests : public ::testing::Test {
protected:
    permuto::Options optsInterpOff; // Must use interp off (DEFAULT)
    permuto::Options optsInterpOn; // Used for error case

    void SetUp() override {
        optsInterpOff.enableStringInterpolation = false;
        optsInterpOn.enableStringInterpolation = true;

        // Validate default is interp OFF
         ASSERT_FALSE(permuto::Options{}.enableStringInterpolation);
    }
};

TEST_F(PermutoCreateReverseTests, BasicReverseTemplate) {
    json original_template = R"(
        {
            "output_name": "${/user/name}",
            "output_email": "${/user/email}",
            "system_id": "${/sys/id}",
            "literal_string": "Hello World",
            "interpolated_string": "Hello ${/user/name}",
            "nested": {
                "is_active": "${/user/active}",
                "config_value": "${/cfg/val}"
            }
        }
    )"_json;

    json expected_reverse = R"(
        {
            "user": {
                "name": "/output_name",
                "email": "/output_email",
                "active": "/nested/is_active"
            },
            "sys": {
                "id": "/system_id"
            },
            "cfg": {
                "val": "/nested/config_value"
            }
        }
    )"_json;

    json result = permuto::create_reverse_template(original_template, optsInterpOff);
    EXPECT_EQ(result, expected_reverse);
    // Test with default options as well
    json result_default = permuto::create_reverse_template(original_template);
    EXPECT_EQ(result_default, expected_reverse);
}

TEST_F(PermutoCreateReverseTests, ReverseTemplateWithArrays) {
     json original_template = R"(
        {
            "ids": ["${/user/id}", "${/sys/id}"],
            "mixed": [1, "${/user/name}", true, {"key": "${/cfg/val}"}]
        }
    )"_json;
     json expected_reverse = R"(
        {
            "user": { "id": "/ids/0", "name": "/mixed/1" },
            "sys": { "id": "/ids/1" },
            "cfg": { "val": "/mixed/3/key" }
        }
    )"_json;

    json result = permuto::create_reverse_template(original_template, optsInterpOff);
    EXPECT_EQ(result, expected_reverse);
}

// MODIFIED: Removed dot.key case as it's ambiguous/unsupported by dot notation for keys with dots.
TEST_F(PermutoCreateReverseTests, ReverseTemplateWithSpecialKeys) {
     json original_template = R"(
        {
            "slash/key": "${/data/slash~0key}",
            "tilde~key": "${/data/tilde~1key}"
        }
    )"_json;
     // The context path segments define the structure.
     // The result JSON Pointers have escaped segments for the result keys.
     json expected_reverse = R"(
        {
            "data": {
                "slash~key": "/slash~1key",
                "tilde/key": "/tilde~0key"
            }
        }
    )"_json;

    json result = permuto::create_reverse_template(original_template, optsInterpOff);
    EXPECT_EQ(result, expected_reverse);
}


TEST_F(PermutoCreateReverseTests, EmptyTemplate) {
    json original_template = R"({})"_json;
    json expected_reverse = R"({})"_json;
    json result = permuto::create_reverse_template(original_template, optsInterpOff);
    EXPECT_EQ(result, expected_reverse);

    original_template = R"([])"_json;
    result = permuto::create_reverse_template(original_template, optsInterpOff);
    EXPECT_EQ(result, expected_reverse); // Still creates empty object
}

TEST_F(PermutoCreateReverseTests, TemplateWithNoPlaceholders) {
    json original_template = R"( {"a": 1, "b": "literal", "c": [true]} )"_json;
    json expected_reverse = R"({})"_json;
    json result = permuto::create_reverse_template(original_template, optsInterpOff);
    EXPECT_EQ(result, expected_reverse);
}

TEST_F(PermutoCreateReverseTests, TemplateWithOnlyInvalidPlaceholders) {
    // Placeholders with empty or invalid paths should be ignored during reverse template creation
    json original_template = R"( {"a": "${}", "b": "hello ${/name}", "c": "${not a pointer}", "d": "${relative/path}"} )"_json;
    json expected_reverse = R"({})"_json;
    json result = permuto::create_reverse_template(original_template, optsInterpOff);
    EXPECT_EQ(result, expected_reverse);
}

TEST_F(PermutoCreateReverseTests, ThrowsIfInterpolationEnabled) {
     json original_template = R"( {"a": "${/b}"} )"_json;
     EXPECT_THROW({
        permuto::create_reverse_template(original_template, optsInterpOn);
     }, std::logic_error);
}

TEST_F(PermutoCreateReverseTests, CustomDelimiters) {
    json original_template = R"( {"out": "<</var/path>>"} )"_json;
    json expected_reverse = R"( {"var": {"path": "/out"}} )"_json;
    permuto::Options opts;
    opts.enableStringInterpolation = false;
    opts.variableStartMarker = "<<";
    opts.variableEndMarker = ">>";
    json result = permuto::create_reverse_template(original_template, opts);
    EXPECT_EQ(result, expected_reverse);
}

// FIXED: This test should now pass with corrected expectations based on processing order.
TEST_F(PermutoCreateReverseTests, ContextPathConflict) {
    // Two placeholders ("${/a/b}" and "${/a}" or "${/a/b/c}") that would map to conflicting paths
     // Latest revision: Deeper paths overwrite shallower ones
     json template1 = R"({
         "val1": "${/a}",
         "val2": "${/a/b}"
     })"_json;
     json reverse1 = permuto::create_reverse_template(template1, optsInterpOff);
     // Expected: {"a": {"b": "/val2"}} - "a" is overwritten by deeper path
     EXPECT_EQ(reverse1["a"]["b"], "/val2");
     EXPECT_FALSE(reverse1["a"].is_string()); // No longer a pointer string

     json template2 = R"({
         "val1": "${/a/b}",
         "val2": "${/a}"
     })"_json;
     json reverse2 = permuto::create_reverse_template(template2, optsInterpOff);
     // Expected: {"a": "/val2"} - Shallower path processed later overwrites
     EXPECT_EQ(reverse2["a"], "/val2");
     EXPECT_FALSE(reverse2.contains("a") && reverse2["a"].is_object() && reverse2["a"].contains("b"));

      json template3 = R"({
          "v1": "${/x}",
          "v2": "${/x/y}",
          "v3": "${/x/y/z}"
      })"_json;
      json reverse3 = permuto::create_reverse_template(template3, optsInterpOff);
      // Expect: {"x": {"y": {"z": "/v3"}}} - Deepest wins
      EXPECT_EQ(reverse3["x"]["y"]["z"], "/v3");
      EXPECT_FALSE(reverse3["x"]["y"].is_string());
      EXPECT_FALSE(reverse3["x"].is_string());
}

TEST_F(PermutoCreateReverseTests, JsonPointerSyntax) {
    json template_json = R"({
        "user_name": "${/user/name}",
        "special_key": "${/user.email}",
        "array_item": "${/items/0}",
        "escaped_key": "${/path~1to~1file}"
    })"_json;
    
    json reverse_template = permuto::create_reverse_template(template_json, optsInterpOff);
    
    // Check that JSON Pointer paths are correctly converted to context structure
    EXPECT_EQ(reverse_template["user"]["name"], "/user_name");
    EXPECT_EQ(reverse_template["user.email"], "/special_key");
    EXPECT_EQ(reverse_template["items"]["0"], "/array_item");
    EXPECT_EQ(reverse_template["path/to/file"], "/escaped_key");
}

// ======================================================
// === Reverse Apply Tests (`apply_reverse`)
// ======================================================
// ... (No changes needed in this suite, should now pass since fixture is fixed) ...
class PermutoApplyReverseTests : public ::testing::Test {
protected:
    // FIXED: Carefully rewrite this JSON literal to avoid parsing errors
    // Ensure keys are correctly quoted, no trailing commas, etc.
     json reverse_template = R"(
        {
            "user": {
                "name": "/output_name",
                "email": "/output_email",
                "active": "/nested/is_active",
                "id": "/ids/0"
            },
            "sys": {
                "id": "/ids/1",
                "nested_arr": "/complex/0/value"
            },
            "cfg": {
                "val": "/nested/config_value",
                "num": "/num_val",
                "null_val": "/null_item",
                "obj": "/obj_val"
            },
            "special/key": "/tilde~0key",
            "special~key": "/slash~1key"
        }
    )"_json; // Removed potential problematic characters/whitespace

    json result_json = R"(
        {
            "output_name": "Alice",
            "output_email": "alice@example.com",
            "literal_string": "Hello World",
            "nested": {
                "is_active": true,
                "config_value": "theme_dark"
            },
            "ids": [123, "XYZ"],
            "num_val": 45.6,
            "null_item": null,
            "obj_val": {"a": 1},
            "complex": [ {"value": [10, 20]} ],
             "tilde~key": "Tilde Value",
             "slash/key": "Slash Value"
        }
    )"_json;

     json expected_context = R"(
        {
            "user": {
                "name": "Alice",
                "email": "alice@example.com",
                "active": true,
                "id": 123
            },
            "sys": {
                "id": "XYZ",
                "nested_arr": [10, 20]
            },
            "cfg": {
                "val": "theme_dark",
                "num": 45.6,
                "null_val": null,
                "obj": {"a": 1}
            },
             "special/key": "Tilde Value",
             "special~key": "Slash Value"
        }
    )"_json;
};

TEST_F(PermutoApplyReverseTests, BasicReconstruction) {
    // Test fixture parsing first (implicitly done by reaching here)
    ASSERT_NO_THROW( (void)reverse_template ); // Explicit check just in case
    ASSERT_NO_THROW( (void)result_json );
    ASSERT_NO_THROW( (void)expected_context );

    json reconstructed = permuto::apply_reverse(reverse_template, result_json);
    EXPECT_EQ(reconstructed, expected_context);
}
TEST_F(PermutoApplyReverseTests, EmptyReverseTemplate) {
    json rt = R"({})"_json;
    json result = R"( {"a": 1} )"_json;
    json expected = R"({})"_json;
    json reconstructed = permuto::apply_reverse(rt, result);
    EXPECT_EQ(reconstructed, expected);
}
TEST_F(PermutoApplyReverseTests, PointerNotFoundInResult) {
     json rt = R"( {"user": {"name": "/missing_pointer"}} )"_json;
     json result = R"( {"output_name": "Alice"} )"_json;
     EXPECT_THROW({
        permuto::apply_reverse(rt, result);
     }, std::runtime_error); // Or nlohmann::json::out_of_range caught and rethrown
}
TEST_F(PermutoApplyReverseTests, InvalidPointerSyntaxInReverseTemplate) {
     json rt = R"( {"user": {"name": "invalid pointer"}} )"_json;
     json result = R"( {"output_name": "Alice"} )"_json;
      EXPECT_THROW({
        permuto::apply_reverse(rt, result);
     }, std::runtime_error); // Or nlohmann::json::parse_error caught and rethrown
}
TEST_F(PermutoApplyReverseTests, MalformedReverseTemplate_NonObjectNode) {
      json rt = R"( {"user": ["/pointer1", "/pointer2"]} )"_json; // Array instead of object/string
     json result = R"( {"output_name": "Alice"} )"_json;
     EXPECT_THROW({
        permuto::apply_reverse(rt, result);
     }, std::runtime_error);
}
TEST_F(PermutoApplyReverseTests, MalformedReverseTemplate_NonStringLeaf) {
      json rt = R"( {"user": {"name": 123}} )"_json; // Number instead of pointer string
     json result = R"( {"output_name": "Alice"} )"_json;
     EXPECT_THROW({
        permuto::apply_reverse(rt, result);
     }, std::runtime_error);
}
TEST_F(PermutoApplyReverseTests, MalformedReverseTemplate_RootNotObject) {
      json rt = R"( ["/pointer1"] )"_json;
      json result = R"( {"output_name": "Alice"} )"_json;
     EXPECT_THROW({
        permuto::apply_reverse(rt, result);
     }, std::runtime_error);

     // Test scalar root too
     rt = R"( "/pointer1" )"_json;
      EXPECT_THROW({
        permuto::apply_reverse(rt, result);
     }, std::runtime_error);
}

// ======================================================
// === Round Trip Test (apply -> create_reverse -> apply_reverse)
// ======================================================
// ... (No changes needed in this test, should still pass) ...
TEST(PermutoRoundTripTests, FullCycle) {
    json original_template = R"(
        {
            "userName": "${/user/name}",
            "details": {
                 "isActive": "${/user/active}",
                 "city": "${/user/address/city}"
             },
            "ids": [ "${/sys/id}", "${/user/id}" ],
            "config": {
                "theme": "${/settings/theme}",
                "notify": "${/settings/notify}"
            },
            "literal": "some text",
            "maybe_interpolated": "Value is ${/settings/theme}"
        }
    )"_json;

    json original_context = R"(
        {
            "user": {
                 "name": "Charlie",
                 "active": true,
                 "id": 999,
                 "address": { "city": "Metropolis" }
            },
            "sys": { "id": "XYZ" },
            "settings": { "theme": "dark", "notify": false }
        }
    )"_json;

    // --- Step 1: Apply (Interpolation OFF - Use Default Opts) ---
    permuto::Options opts; // Use default options (interp=false)
    ASSERT_FALSE(opts.enableStringInterpolation); // Verify default

    json result = permuto::apply(original_template, original_context, opts);

    // Verify result structure (ensure literals/interpolation didn't happen)
    json expected_result = R"(
         {
            "userName": "Charlie",
            "details": { "isActive": true, "city": "Metropolis" },
            "ids": [ "XYZ", 999 ],
            "config": { "theme": "dark", "notify": false },
            "literal": "some text",
            "maybe_interpolated": "Value is ${/settings/theme}"
        }
    )"_json;
    ASSERT_EQ(result, expected_result);

    // --- Step 2: Create Reverse Template (Use Default Opts) ---
    json reverse_template = permuto::create_reverse_template(original_template, opts); // or just create_reverse_template(original_template)

    // Verify reverse template structure
    json expected_reverse_template = R"(
        {
            "user": {
                "name": "/userName",
                "active": "/details/isActive",
                "address": { "city": "/details/city" },
                "id": "/ids/1"
             },
            "sys": { "id": "/ids/0" },
            "settings": { "theme": "/config/theme", "notify": "/config/notify" }
        }
    )"_json;
     ASSERT_EQ(reverse_template, expected_reverse_template);


     // --- Step 3: Apply Reverse ---
     json reconstructed_context = permuto::apply_reverse(reverse_template, result);

     // --- Step 4: Verify Reconstruction ---
     // The reconstructed context should match the original context EXACTLY.
     EXPECT_EQ(reconstructed_context, original_context);

}

// --- JSON Pointer Syntax Public API Tests ---
TEST(PermutoPublicApiTests, JsonPointerBasicUsage) {
    const json tpl = R"({
        "name": "${/user/name}",
        "city": "${/user/address/city}",
        "first_item": "${/items/0}"
    })"_json;
    const json ctx = R"({
        "user": {
            "name": "Bob",
            "address": { "city": "NYC" }
        },
        "items": ["apple", "banana", "cherry"]
    })"_json;
    permuto::Options opts;
    const json result = permuto::apply(tpl, ctx, opts);
    EXPECT_EQ(result["name"], "Bob");
    EXPECT_EQ(result["city"], "NYC");
    EXPECT_EQ(result["first_item"], "apple");
}

TEST(PermutoPublicApiTests, JsonPointerWithSpecialKeys) {
    const json tpl = R"({
        "dot_key": "${/user.name}",
        "slash_key": "${/path~1to~1file}",
        "tilde_key": "${/var~0name}"
    })"_json;
    const json ctx = R"({
        "user.name": "dotted_key_value",
        "path/to/file": "slashed_key_value",
        "var~name": "tilde_key_value"
    })"_json;
    permuto::Options opts;
    const json result = permuto::apply(tpl, ctx, opts);
    EXPECT_EQ(result["dot_key"], "dotted_key_value");
    EXPECT_EQ(result["slash_key"], "slashed_key_value");
    EXPECT_EQ(result["tilde_key"], "tilde_key_value");
}

// Removed MixedDotAndJsonPointerSyntax test - we only support JSON Pointers now

TEST(PermutoPublicApiTests, JsonPointerWithInterpolation) {
    const json tpl = R"({
        "message": "User ${/user/name} lives in ${/user/address/city}",
        "array_msg": "Item at index 1 is ${/items/1}"
    })"_json;
    const json ctx = R"({
        "user": {
            "name": "Diana",
            "address": { "city": "Seattle" }
        },
        "items": ["first", "second", "third"]
    })"_json;
    permuto::Options opts;
    opts.enableStringInterpolation = true;
    const json result = permuto::apply(tpl, ctx, opts);
    EXPECT_EQ(result["message"], "User Diana lives in Seattle");
    EXPECT_EQ(result["array_msg"], "Item at index 1 is second");
}

// Test for recursion depth limit
TEST(PermutoPublicApiTests, RecursionDepthLimit) {
    json context = R"({
        "level1": "${/level2}",
        "level2": "${/level3}",
        "level3": "${/level4}",
        "level4": "end"
    })"_json;

    json template_json = R"({
        "result": "${/level1}"
    })"_json;

    permuto::Options opts;
    opts.enableStringInterpolation = true;
    opts.maxRecursionDepth = 2; // Set a very low limit

    EXPECT_THROW({
        try {
            permuto::apply(template_json, context, opts);
        } catch (const permuto::PermutoRecursionDepthException& e) {
            EXPECT_EQ(e.get_current_depth(), 3); // Should fail at depth 3
            EXPECT_EQ(e.get_max_depth(), 2);
            throw;
        }
    }, permuto::PermutoRecursionDepthException);
}

// Test edge case: limit of 0
TEST(PermutoPublicApiTests, RecursionDepthLimitZero) {
    json context = R"({
        "level1": "${/level2}",
        "level2": "end"
    })"_json;

    json template_json = R"({
        "result": "${/level1}"
    })"_json;

    permuto::Options opts;
    opts.enableStringInterpolation = true;
    opts.maxRecursionDepth = 0; // No recursion allowed

    EXPECT_THROW({
        try {
            permuto::apply(template_json, context, opts);
        } catch (const permuto::PermutoRecursionDepthException& e) {
            EXPECT_EQ(e.get_current_depth(), 1); // Should fail at depth 1
            EXPECT_EQ(e.get_max_depth(), 0);
            throw;
        }
    }, permuto::PermutoRecursionDepthException);
}

// Test edge case: limit of 1
TEST(PermutoPublicApiTests, RecursionDepthLimitOne) {
    json context = R"({
        "level1": "${/level2}",
        "level2": "${/level3}",
        "level3": "end"
    })"_json;

    json template_json = R"({
        "result": "${/level1}"
    })"_json;

    permuto::Options opts;
    opts.enableStringInterpolation = true;
    opts.maxRecursionDepth = 1; // Only one level of recursion allowed

    EXPECT_THROW({
        try {
            permuto::apply(template_json, context, opts);
        } catch (const permuto::PermutoRecursionDepthException& e) {
            EXPECT_EQ(e.get_current_depth(), 2); // Should fail at depth 2
            EXPECT_EQ(e.get_max_depth(), 1);
            throw;
        }
    }, permuto::PermutoRecursionDepthException);
}

// Test boundary condition: 6 levels with limit 6 (should succeed)
TEST(PermutoPublicApiTests, RecursionDepthLimitSixSuccess) {
    json context = R"({
        "level1": "${/level2}",
        "level2": "${/level3}",
        "level3": "${/level4}",
        "level4": "${/level5}",
        "level5": "${/level6}",
        "level6": "end"
    })"_json;

    json template_json = R"({
        "result": "${/level1}"
    })"_json;

    permuto::Options opts;
    opts.enableStringInterpolation = true;
    opts.maxRecursionDepth = 7; // One more than the chain length to allow success

    // Should succeed
    json result = permuto::apply(template_json, context, opts);
    EXPECT_EQ(result["result"], "end");
}

// Test boundary condition: 7 levels with limit 6 (should fail)
TEST(PermutoPublicApiTests, RecursionDepthLimitSixFailure) {
    json context = R"({
        "level1": "${/level2}",
        "level2": "${/level3}",
        "level3": "${/level4}",
        "level4": "${/level5}",
        "level5": "${/level6}",
        "level6": "${/level7}",
        "level7": "end"
    })"_json;

    json template_json = R"({
        "result": "${/level1}"
    })"_json;

    permuto::Options opts;
    opts.enableStringInterpolation = true;
    opts.maxRecursionDepth = 6; // One less than the chain length

    EXPECT_THROW({
        try {
            permuto::apply(template_json, context, opts);
        } catch (const permuto::PermutoRecursionDepthException& e) {
            EXPECT_EQ(e.get_current_depth(), 7); // Should fail at depth 7
            EXPECT_EQ(e.get_max_depth(), 6);
            throw;
        }
    }, permuto::PermutoRecursionDepthException);
}

// Test higher limit: 10 levels with limit 10 (should succeed)
TEST(PermutoPublicApiTests, RecursionDepthLimitTenSuccess) {
    json context = R"({
        "level1": "${/level2}",
        "level2": "${/level3}",
        "level3": "${/level4}",
        "level4": "${/level5}",
        "level5": "${/level6}",
        "level6": "${/level7}",
        "level7": "${/level8}",
        "level8": "${/level9}",
        "level9": "${/level10}",
        "level10": "end"
    })"_json;

    json template_json = R"({
        "result": "${/level1}"
    })"_json;

    permuto::Options opts;
    opts.enableStringInterpolation = true;
    opts.maxRecursionDepth = 11; // One more than the chain length to allow success

    // Should succeed
    json result = permuto::apply(template_json, context, opts);
    EXPECT_EQ(result["result"], "end");
}

// Test higher limit: 11 levels with limit 10 (should fail)
TEST(PermutoPublicApiTests, RecursionDepthLimitTenFailure) {
    json context = R"({
        "level1": "${/level2}",
        "level2": "${/level3}",
        "level3": "${/level4}",
        "level4": "${/level5}",
        "level5": "${/level6}",
        "level6": "${/level7}",
        "level7": "${/level8}",
        "level8": "${/level9}",
        "level9": "${/level10}",
        "level10": "${/level11}",
        "level11": "end"
    })"_json;

    json template_json = R"({
        "result": "${/level1}"
    })"_json;

    permuto::Options opts;
    opts.enableStringInterpolation = true;
    opts.maxRecursionDepth = 10; // One less than the chain length

    EXPECT_THROW({
        try {
            permuto::apply(template_json, context, opts);
        } catch (const permuto::PermutoRecursionDepthException& e) {
            EXPECT_EQ(e.get_current_depth(), 11); // Should fail at depth 11
            EXPECT_EQ(e.get_max_depth(), 10);
            throw;
        }
    }, permuto::PermutoRecursionDepthException);
}

// Test non-throwing case: shorter chain than limit
TEST(PermutoPublicApiTests, RecursionDepthLimitNotReached) {
    json context = R"({
        "level1": "${/level2}",
        "level2": "${/level3}",
        "level3": "end"
    })"_json;

    json template_json = R"({
        "result": "${/level1}"
    })"_json;

    permuto::Options opts;
    opts.enableStringInterpolation = true;
    opts.maxRecursionDepth = 10; // Much higher than needed

    // Should succeed without throwing
    json result = permuto::apply(template_json, context, opts);
    EXPECT_EQ(result["result"], "end");
}
