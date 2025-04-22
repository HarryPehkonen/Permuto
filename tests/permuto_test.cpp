// permuto/tests/permuto_test.cpp
#include <gtest/gtest.h>
#include <permuto/permuto.hpp>
#include <permuto/exceptions.hpp>
#include <nlohmann/json.hpp>

// Include the internal header FOR TESTING detail functions DIRECTLY
#include "../src/permuto_internal.hpp" // Adjust path if necessary

#include <set>
#include <string>

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
            "key~with~tildes": "tildes_val"
        }
    )"_json;

    permuto::Options ignoreOpts; // Default is ignore
    permuto::Options ignoreInterpolateOpts;
    permuto::Options errorOpts;

    void SetUp() override {
        ignoreOpts.onMissingKey = permuto::MissingKeyBehavior::Ignore;
        ignoreInterpolateOpts.onMissingKey = permuto::MissingKeyBehavior::Ignore;
        ignoreInterpolateOpts.enableStringInterpolation = true;
        errorOpts.onMissingKey = permuto::MissingKeyBehavior::Error;
    }
};

TEST_F(PermutoResolvePathTests, ResolvesTopLevelKey) {
    const json* result = permuto::detail::resolve_path(context, "settings", ignoreOpts, "${settings}");
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(*result, context["settings"]);
}

TEST_F(PermutoResolvePathTests, ResolvesNestedKey) {
    const json* result = permuto::detail::resolve_path(context, "user.address.city", ignoreOpts, "${user.address.city}");
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(*result, "Wonderland");
}

TEST_F(PermutoResolvePathTests, ResolvesToNullValue) {
    const json* result = permuto::detail::resolve_path(context, "top_level_null", ignoreOpts, "${top_level_null}");
    ASSERT_NE(result, nullptr);
    ASSERT_TRUE(result->is_null());
}

// Tests for keys needing JSON Pointer escaping
TEST_F(PermutoResolvePathTests, ResolvesKeyWithSlashes) {
     const json* result = permuto::detail::resolve_path(context, "key/with/slashes", ignoreOpts, "${key/with/slashes}");
     ASSERT_NE(result, nullptr);
     EXPECT_EQ(*result, "slashes_val");
}

TEST_F(PermutoResolvePathTests, ResolvesKeyWithTildes) {
     const json* result = permuto::detail::resolve_path(context, "key~with~tildes", ignoreOpts, "${key~with~tildes}");
     ASSERT_NE(result, nullptr);
     EXPECT_EQ(*result, "tildes_val");
}


TEST_F(PermutoResolvePathTests, HandlesMissingTopLevelKeyIgnore) {
    const json* result = permuto::detail::resolve_path(context, "nonexistent", ignoreOpts, "${nonexistent}");
    EXPECT_EQ(result, nullptr);
}

TEST_F(PermutoResolvePathTests, HandlesMissingNestedKeyIgnore) {
    const json* result = permuto::detail::resolve_path(context, "user.address.street", ignoreOpts, "${user.address.street}");
    EXPECT_EQ(result, nullptr);
}

TEST_F(PermutoResolvePathTests, HandlesMissingIntermediateKeyIgnore) {
    const json* result = permuto::detail::resolve_path(context, "user.profile.id", ignoreOpts, "${user.profile.id}");
    EXPECT_EQ(result, nullptr); // "profile" does not exist
}

TEST_F(PermutoResolvePathTests, HandlesMissingTopLevelKeyError) {
    EXPECT_THROW({
        try {
            permuto::detail::resolve_path(context, "nonexistent", errorOpts, "${nonexistent}");
        } catch (const permuto::PermutoMissingKeyException& e) {
            EXPECT_EQ(e.get_key_path(), "nonexistent");
            // Optionally check e.what() content too
            EXPECT_NE(std::string(e.what()).find("nonexistent"), std::string::npos);
            throw; // Re-throw
        }
    }, permuto::PermutoMissingKeyException);
}

TEST_F(PermutoResolvePathTests, HandlesMissingNestedKeyError) {
     EXPECT_THROW({
        try {
            permuto::detail::resolve_path(context, "user.address.street", errorOpts, "${user.address.street}");
        } catch (const permuto::PermutoMissingKeyException& e) {
            EXPECT_EQ(e.get_key_path(), "user.address.street");
             EXPECT_NE(std::string(e.what()).find("user.address.street"), std::string::npos);
            throw; // Re-throw
        }
    }, permuto::PermutoMissingKeyException);
}

TEST_F(PermutoResolvePathTests, HandlesMissingIntermediateKeyError) {
    EXPECT_THROW({
        try {
            permuto::detail::resolve_path(context, "user.profile.id", errorOpts, "${user.profile.id}");
        } catch (const permuto::PermutoMissingKeyException& e) {
            EXPECT_EQ(e.get_key_path(), "user.profile.id");
             EXPECT_NE(std::string(e.what()).find("user.profile.id"), std::string::npos);
            throw; // Re-throw
        }
    }, permuto::PermutoMissingKeyException);
}


TEST_F(PermutoResolvePathTests, HandlesTraversalIntoNonObjectIgnore) {
    // JSON pointer handles this by simply not finding the path
    const json* result = permuto::detail::resolve_path(context, "user.name.first", ignoreOpts, "${user.name.first}");
    EXPECT_EQ(result, nullptr);
    result = permuto::detail::resolve_path(context, "settings.enabled.value", ignoreOpts, "${settings.enabled.value}");
    EXPECT_EQ(result, nullptr);
    result = permuto::detail::resolve_path(context, "settings.values.key", ignoreOpts, "${settings.values.key}");
    EXPECT_EQ(result, nullptr); // Note: JSON pointer path for array index would be /settings/values/0 etc.
}

TEST_F(PermutoResolvePathTests, HandlesTraversalIntoNonObjectError) {
    // JSON pointer throws out_of_range which we map to PermutoMissingKeyException
    EXPECT_THROW({
        try {
            permuto::detail::resolve_path(context, "user.name.first", errorOpts, "${user.name.first}");
        } catch (const permuto::PermutoMissingKeyException& e) {
            EXPECT_EQ(e.get_key_path(), "user.name.first");
            throw; // Re-throw
        }
    }, permuto::PermutoMissingKeyException);
}

TEST_F(PermutoResolvePathTests, HandlesEmptyPathIgnore) {
     const json* result = permuto::detail::resolve_path(context, "", ignoreOpts, "${}");
     EXPECT_EQ(result, nullptr);
}

TEST_F(PermutoResolvePathTests, HandlesEmptyPathError) {
    EXPECT_THROW({
       permuto::detail::resolve_path(context, "", errorOpts, "${}");
    }, permuto::PermutoMissingKeyException);
}


TEST_F(PermutoResolvePathTests, HandlesInvalidPathFormatsIgnore) {
    EXPECT_EQ(permuto::detail::resolve_path(context, ".user", ignoreOpts, "${.user}"), nullptr);
    EXPECT_EQ(permuto::detail::resolve_path(context, "user.", ignoreOpts, "${user.}"), nullptr);
    EXPECT_EQ(permuto::detail::resolve_path(context, "user..name", ignoreOpts, "${user..name}"), nullptr);
}

TEST_F(PermutoResolvePathTests, HandlesInvalidPathFormatsError) {
     EXPECT_THROW( permuto::detail::resolve_path(context, ".user", errorOpts, "${.user}"), permuto::PermutoMissingKeyException);
     EXPECT_THROW( permuto::detail::resolve_path(context, "user.", errorOpts, "${user.}"), permuto::PermutoMissingKeyException);
     EXPECT_THROW( permuto::detail::resolve_path(context, "user..name", errorOpts, "${user..name}"), permuto::PermutoMissingKeyException);
}


// --- Test Suite for detail::process_string ---
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
            "ref": "${user.name}",
            "nested_ref": "User is ${ref}",
            "cycle_a": "${cycle_b}",
            "cycle_b": "${cycle_a}",
            "deep_cycle_a": "${deep_cycle_b.ref}",
            "deep_cycle_b": { "ref": "${deep_cycle_a}" },
            "empty_placeholder_ref": "${}"
        }
    )"_json;

    permuto::Options ignoreOpts; // Default is ignore
    permuto::Options ignoreInterpolateOpts;
    permuto::Options errorOpts;
    permuto::Options errorInterpolateOpts;
    std::set<std::string> active_paths; // Fresh set for each call

    void SetUp() override {
        ignoreOpts.onMissingKey = permuto::MissingKeyBehavior::Ignore;
        ignoreInterpolateOpts.onMissingKey = permuto::MissingKeyBehavior::Ignore;
        ignoreInterpolateOpts.enableStringInterpolation = true;
        errorOpts.onMissingKey = permuto::MissingKeyBehavior::Error;
        errorInterpolateOpts.onMissingKey = permuto::MissingKeyBehavior::Error;
        errorInterpolateOpts.enableStringInterpolation = true;
        active_paths.clear(); // Ensure clean slate for each test
    }

    // Helper to call the detail function
    json process_str_opts(const std::string& tpl, const permuto::Options& opts) {
         active_paths.clear(); // Ensure clear before call
         return permuto::detail::process_string(tpl, context, opts, active_paths);
    }
     // Default to ignoreOpts if not specified
     json process_str(const std::string& tpl) {
        return process_str_opts(tpl, ignoreOpts);
     }

     json process_interp_str(const std::string& tpl) {
        return process_str_opts(tpl, ignoreInterpolateOpts);
     }

};

// Exact Match Tests
TEST_F(PermutoProcessStringTests, ExactMatchString) {
    json result = process_str("${user.name}");
    EXPECT_EQ(result.type(), json::value_t::string);
    EXPECT_EQ(result.get<std::string>(), "Alice");
}

TEST_F(PermutoProcessStringTests, ExactMatchNumber) {
    json result = process_str("${user.id}");
    EXPECT_TRUE(result.is_number());
    EXPECT_EQ(result.get<int>(), 123);
}

TEST_F(PermutoProcessStringTests, ExactMatchFloat) {
    json result = process_str("${pi}");
    EXPECT_EQ(result.type(), json::value_t::number_float);
    EXPECT_DOUBLE_EQ(result.get<double>(), 3.14);
}

TEST_F(PermutoProcessStringTests, ExactMatchBoolean) {
    json result = process_str("${enabled}");
    EXPECT_EQ(result.type(), json::value_t::boolean);
    EXPECT_EQ(result.get<bool>(), true);
}

TEST_F(PermutoProcessStringTests, ExactMatchNull) {
    json result = process_str("${nothing}");
    EXPECT_EQ(result.type(), json::value_t::null);
    EXPECT_TRUE(result.is_null());
}

TEST_F(PermutoProcessStringTests, ExactMatchArray) {
    json result = process_str("${items}");
    EXPECT_EQ(result.type(), json::value_t::array);
    EXPECT_EQ(result, json::parse("[1, \"two\"]"));
}

TEST_F(PermutoProcessStringTests, ExactMatchObject) {
    json result = process_str("${config}");
    EXPECT_EQ(result.type(), json::value_t::object);
    EXPECT_EQ(result, json::parse("{\"theme\": \"dark\"}"));
}

TEST_F(PermutoProcessStringTests, ExactMatchMissingIgnore) {
    json result = process_str_opts("${missing.key}", ignoreOpts);
    EXPECT_EQ(result.type(), json::value_t::string); // Returns placeholder string
    EXPECT_EQ(result.get<std::string>(), "${missing.key}");
}

TEST_F(PermutoProcessStringTests, ExactMatchMissingError) {
    EXPECT_THROW({
        process_str_opts("${missing.key}", errorOpts);
    }, permuto::PermutoMissingKeyException);
}

TEST_F(PermutoProcessStringTests, ExactMatchRecursion) {
    // ${ref} resolves to "${user.name}", which then resolves to "Alice"
    json result = process_str("${ref}");
    EXPECT_EQ(result.type(), json::value_t::string);
    EXPECT_EQ(result.get<std::string>(), "Alice");
}

TEST_F(PermutoProcessStringTests, ExactMatchRecursionNested) {
     // ${nested_ref} resolves to "User is ${ref}", which interpolates to "User is Alice"
    json result = process_interp_str("${nested_ref}");
    EXPECT_EQ(result.type(), json::value_t::string);
    EXPECT_EQ(result.get<std::string>(), "User is Alice");
}

TEST_F(PermutoProcessStringTests, ExactMatchWithInnerMarkersShouldInterpolate) {
    // This looks like an exact match but contains markers, so treat as interpolation
    json result = process_interp_str("${user.name} has ID ${user.id}"); // Not wrapped in outer literal quotes
    EXPECT_EQ(result.type(), json::value_t::string);
    EXPECT_EQ(result.get<std::string>(), "Alice has ID 123");
}

TEST_F(PermutoProcessStringTests, ExactMatchEmptyPlaceholder) {
    // ${} should be treated as literal string "${}" in exact match context
    json result = process_str("${}");
    EXPECT_EQ(result.type(), json::value_t::string);
    EXPECT_EQ(result.get<std::string>(), "${}");
}

TEST_F(PermutoProcessStringTests, ExactMatchResolvesToEmptyPlaceholder) {
    // ${empty_placeholder_ref} resolves to "${}" which should be returned literally
     json result = process_str("${empty_placeholder_ref}");
     EXPECT_EQ(result.type(), json::value_t::string);
     EXPECT_EQ(result.get<std::string>(), "${}");
}


// Interpolation Tests
TEST_F(PermutoProcessStringTests, InterpolationSimple) {
    json result = process_interp_str("Name: ${user.name}");
    EXPECT_EQ(result.type(), json::value_t::string);
    EXPECT_EQ(result.get<std::string>(), "Name: Alice");
}

TEST_F(PermutoProcessStringTests, InterpolationMultiple) {
    json result = process_interp_str("${user.name} (${user.id})");
    EXPECT_EQ(result.get<std::string>(), "Alice (123)");
}

TEST_F(PermutoProcessStringTests, InterpolationMixedTypes) {
    json result = process_interp_str("User: ${user.name}, ID: ${user.id}, Enabled: ${enabled}, Pi: ${pi}, Nada: ${nothing}, Items: ${items}, Cfg: ${config}, Html: ${html}");
    // Note the stringification of array/object via dump()
    EXPECT_EQ(result.get<std::string>(), "User: Alice, ID: 123, Enabled: true, Pi: 3.14, Nada: null, Items: [1,\"two\"], Cfg: {\"theme\":\"dark\"}, Html: <p>Hello</p>");
}

TEST_F(PermutoProcessStringTests, InterpolationMissingIgnore) {
    json result = process_str_opts("Val: ${user.name} Missing: ${missing.key} End", ignoreInterpolateOpts);
    EXPECT_EQ(result.get<std::string>(), "Val: Alice Missing: ${missing.key} End");
}

TEST_F(PermutoProcessStringTests, InterpolationMissingError) {
     EXPECT_THROW({
        process_str_opts("Val: ${user.name} Missing: ${missing.key} End", errorInterpolateOpts);
    }, permuto::PermutoMissingKeyException);
}

TEST_F(PermutoProcessStringTests, InterpolationWithRecursion) {
    // "User is ${ref}" -> "User is ${user.name}" -> "User is Alice"
    json result = process_interp_str("Info: ${nested_ref}");
    EXPECT_EQ(result.get<std::string>(), "Info: User is Alice");
}

TEST_F(PermutoProcessStringTests, InterpolationAdjacent) {
    json result = process_interp_str("${user.name}${user.id}");
    EXPECT_EQ(result.get<std::string>(), "Alice123");
}

TEST_F(PermutoProcessStringTests, InterpolationOnlyPlaceholders) {
    json result = process_interp_str("${user.name}:${enabled}");
     EXPECT_EQ(result.get<std::string>(), "Alice:true");
}

TEST_F(PermutoProcessStringTests, InterpolationUnterminated) {
     json result = process_interp_str("Hello ${user.name} and ${unclosed");
     EXPECT_EQ(result.get<std::string>(), "Hello Alice and ${unclosed");
}

TEST_F(PermutoProcessStringTests, InterpolationEmptyPlaceholder) {
    // ${} should be treated as literal string "${}" in interpolation context
    json result = process_interp_str("Value: ${}");
    EXPECT_EQ(result.type(), json::value_t::string);
    EXPECT_EQ(result.get<std::string>(), "Value: ${}");
}

TEST_F(PermutoProcessStringTests, InterpolationResolvesToEmptyPlaceholder) {
    // ${empty_placeholder_ref} resolves to "${}" which should be included literally
     json result = process_interp_str("Value: ${empty_placeholder_ref} End");
     EXPECT_EQ(result.type(), json::value_t::string);
     EXPECT_EQ(result.get<std::string>(), "Value: ${} End");
}


TEST_F(PermutoProcessStringTests, StringNoPlaceholders) {
    json result = process_str("Just plain text.");
    EXPECT_EQ(result.get<std::string>(), "Just plain text.");
}

TEST_F(PermutoProcessStringTests, StringEmpty) {
    json result = process_str("");
    EXPECT_EQ(result.get<std::string>(), "");
}


// Cycle Tests
TEST_F(PermutoProcessStringTests, CycleExactMatch) {
    EXPECT_THROW({
        process_str("${cycle_a}");
    }, permuto::PermutoCycleException);
}

TEST_F(PermutoProcessStringTests, CycleInterpolation) {
     EXPECT_THROW({
        process_interp_str("Cycle here: ${cycle_a}");
    }, permuto::PermutoCycleException);
}

TEST_F(PermutoProcessStringTests, CycleDeepExact) {
     EXPECT_THROW({
        process_str("${deep_cycle_a}");
    }, permuto::PermutoCycleException);
}

TEST_F(PermutoProcessStringTests, CycleDeepInterpolation) {
     EXPECT_THROW({
        process_interp_str("Deep cycle: ${deep_cycle_a}...");
    }, permuto::PermutoCycleException);
}

// Custom Delimiter Tests (using process_str_opts)
TEST_F(PermutoProcessStringTests, CustomDelimitersExact) {
    permuto::Options opts;
    opts.variableStartMarker = "<<";
    opts.variableEndMarker = ">>";
    json result = process_str_opts("<<user.name>>", opts);
    EXPECT_EQ(result.get<std::string>(), "Alice");
}

TEST_F(PermutoProcessStringTests, CustomDelimitersInterpolation) {
    permuto::Options opts;
    opts.variableStartMarker = "::";
    opts.variableEndMarker = "::";
    opts.enableStringInterpolation = true;
    json result = process_str_opts("ID is ::user.id:: Status ::enabled::", opts);
    EXPECT_EQ(result.get<std::string>(), "ID is 123 Status true");
}

TEST_F(PermutoProcessStringTests, CustomDelimitersInterpolationAdjacent) {
    permuto::Options opts;
    opts.variableStartMarker = "%%";
    opts.variableEndMarker = "%%";
    opts.enableStringInterpolation = true;
    json result = process_str_opts("%%user.name%%%%user.id%%", opts);
    EXPECT_EQ(result.get<std::string>(), "Alice123");
}

// --- End of detail::process_string tests ---


// --- Basic Substitution Test (Using permuto::process) ---
TEST(PermutoBasicTests, SimpleSubstitution) {
    json template_json = R"(
        {
            "greeting": "Hello, ${user.name}!",
            "email": "${user.email}"
        }
    )"_json;

    json context = R"(
        {
            "user": {
                "name": "Alice",
                "email": "alice@example.com"
            }
        }
    )"_json;

    json expected = R"(
        {
            "greeting": "Hello, Alice!",
            "email": "alice@example.com"
        }
    )"_json;

    permuto::Options opts;
    opts.enableStringInterpolation = true;
    json result = permuto::process(template_json, context, opts);
    EXPECT_EQ(result, expected);
}

// --- Type Handling Test (Using permuto::process) ---
TEST(PermutoBasicTests, ExactMatchTypePreservation) {
    json template_json = R"(
        {
            "num_val": "${data.num}",
            "bool_val": "${data.bool}",
            "null_val": "${data.null}",
            "arr_val": "${data.arr}",
            "obj_val": "${data.obj}",
            "str_val": "${data.str}"
        }
    )"_json;

     json context = R"(
        {
            "data": {
                "num": 123,
                "bool": true,
                "null": null,
                "arr": [1, "a"],
                "obj": {"k": "v"},
                "str": "hello"
            }
        }
    )"_json;

    json expected = R"(
        {
            "num_val": 123,
            "bool_val": true,
            "null_val": null,
            "arr_val": [1, "a"],
            "obj_val": {"k": "v"},
            "str_val": "hello"
        }
    )"_json;

    json result = permuto::process(template_json, context);
    EXPECT_EQ(result, expected);
}

// --- String Interpolation Test (Using permuto::process) ---
TEST(PermutoBasicTests, InterpolationTypeConversion) {
     json template_json = R"(
        {
            "message": "Data: num=${d.num}, bool=${d.bool}, null=${d.null}, arr=${d.arr}, obj=${d.obj}"
        }
    )"_json;
     json context = R"(
        {
            "d": {
                "num": 456,
                "bool": false,
                "null": null,
                "arr": [true, null],
                "obj": {"x": 1}
            }
        }
    )"_json;
    // Note: Exact string representation of array/object might vary slightly
    // but should be consistent JSON. nlohmann::json::dump() compact is predictable.
    json expected = R"(
        {
            "message": "Data: num=456, bool=false, null=null, arr=[true,null], obj={\"x\":1}"
        }
    )"_json;

    permuto::Options opts;
    opts.enableStringInterpolation = true;
    json result = permuto::process(template_json, context, opts);
    EXPECT_EQ(result, expected);
}


// --- Missing Key Tests (Using permuto::process) ---
TEST(PermutoMissingKeyTests, IgnoreMode) {
     json template_json = R"(
        {
            "present": "${a.b}",
            "missing_leaf": "${a.c}",
            "missing_branch": "${x.y}"
        }
    )"_json;
     json context = R"( {"a": {"b": 1}} )"_json;
     json expected = R"(
        {
            "present": 1,
            "missing_leaf": "${a.c}",
            "missing_branch": "${x.y}"
        }
    )"_json;

    permuto::Options options;
    options.onMissingKey = permuto::MissingKeyBehavior::Ignore; // Explicitly set default for clarity

    json result = permuto::process(template_json, context, options);
    EXPECT_EQ(result, expected);
}

TEST(PermutoMissingKeyTests, ErrorMode) {
     json template_json = R"( {"missing": "${a.b.c}"} )"_json;
     json context = R"( {"a": {"b": {}}} )"_json; // 'c' is missing

    permuto::Options options;
    options.onMissingKey = permuto::MissingKeyBehavior::Error;

    EXPECT_THROW({
        try {
            permuto::process(template_json, context, options);
        } catch (const permuto::PermutoMissingKeyException& e) {
            EXPECT_EQ(e.get_key_path(), "a.b.c"); // Check specific path retrieval
            EXPECT_NE(std::string(e.what()).find("a.b.c"), std::string::npos); // Check if path is mentioned
            throw; // Re-throw to satisfy EXPECT_THROW
        }
    }, permuto::PermutoMissingKeyException);
}

TEST(PermutoMissingKeyTests, ErrorModeInterpolation) {
     json template_json = R"( {"msg": "Hello ${a.b.c}"} )"_json; // Missing key in interpolation
     json context = R"( {"a": {"b": {}}} )"_json;

    permuto::Options options;
    options.onMissingKey = permuto::MissingKeyBehavior::Error;
    options.enableStringInterpolation = true;

    EXPECT_THROW({
        permuto::process(template_json, context, options);
    }, permuto::PermutoMissingKeyException);
}


// --- Cycle Detection Test (Using permuto::process) ---
TEST(PermutoCycleTests, DirectCycleError) {
    json template_json = R"( {"a": "${b}", "b": "${a}"} )"_json;
    // Need context that allows the cycle to be traversed
    json context = R"( {"a": "placeholder_a", "b": "${a}"} )"_json; // b refers to a

    // Processing template's "a":"${b}" will trigger lookup of b -> "${a}" -> lookup of a -> active check detects 'a'
    EXPECT_THROW({
        try {
            // Note: Template defines the structure, context provides values.
            // Processing template["a"] resolves to context["b"] = "${a}"
            // Then process_string("${a}") looks up context["a"] which might be itself... let's refine context
             json context_cycle = R"({"a": "${b}", "b": "${a}"})"_json; // More direct cycle in context values
            permuto::process(template_json, context_cycle); // Process the template using cycling context
        } catch (const permuto::PermutoCycleException& e) {
            std::string cycle_path = e.get_cycle_path();
             // Cycle detection path might not be perfectly predictable ("a -> b" or "b -> a")
            EXPECT_NE(cycle_path.find("a"), std::string::npos);
            EXPECT_NE(cycle_path.find("b"), std::string::npos);
            throw;
        }
    }, permuto::PermutoCycleException);
}

TEST(PermutoCycleTests, IndirectCycleError) {
    json template_json = R"( {"start": "${path.to.b}"} )"_json;
    json context = R"(
        {
            "path": {"to": {"b": "${cycled.a}" } },
            "cycled": {"a": "${path.to.b}"}
        }
    )"_json;

    EXPECT_THROW({
         permuto::process(template_json, context);
    }, permuto::PermutoCycleException);
}

TEST(PermutoCycleTests, SelfReferenceCycleError) {
    json template_json = R"( {"a": "${a}"} )"_json;
    json context = R"( {"a": "${a}"} )"_json; // Value points to itself

     EXPECT_THROW({
         permuto::process(template_json, context);
     }, permuto::PermutoCycleException);
}


// --- Custom Delimiter Test (Using permuto::process) ---
TEST(PermutoOptionTests, CustomDelimiters) {
    json template_json = R"( {"value": "<<data.item>>", "msg": "Item is: <<data.item>>"} )"_json;
    json context = R"( {"data": {"item": "success"}} )"_json;
    json expected = R"( {"value": "success", "msg": "Item is: success"} )"_json;

    permuto::Options options;
    options.variableStartMarker = "<<";
    options.variableEndMarker = ">>";
    options.enableStringInterpolation = true;

    json result = permuto::process(template_json, context, options);
    EXPECT_EQ(result, expected);
}

TEST(PermutoOptionTests, InvalidOptions) {
    json template_json = R"({})"_json;
    json context = R"({})"_json;
    permuto::Options options;

    options.variableStartMarker = "";
    EXPECT_THROW(permuto::process(template_json, context, options), std::invalid_argument);

    options.variableStartMarker = "{"; options.variableEndMarker = "";
    EXPECT_THROW(permuto::process(template_json, context, options), std::invalid_argument);

     options.variableStartMarker = "@@"; options.variableEndMarker = "@@";
    EXPECT_THROW(permuto::process(template_json, context, options), std::invalid_argument);
}

// --- Edge Case Tests ---
TEST(PermutoEdgeCases, PlaceholdersInKeys) {
    // Keys should not be processed
    json template_json = R"( { "${a.key}": "value", "static_key": "${val}" } )"_json;
    json context = R"( { "val": "the_value", "a": { "key": "ignored"} } )"_json;
    json expected = R"( { "${a.key}": "value", "static_key": "the_value" } )"_json;

    json result = permuto::process(template_json, context);
    EXPECT_EQ(result, expected);
}

TEST(PermutoEdgeCases, EmptyTemplateAndContext) {
     json template_json = R"({})"_json;
     json context = R"({})"_json;
     json expected = R"({})"_json;
     json result = permuto::process(template_json, context);
     EXPECT_EQ(result, expected);

     template_json = R"([])"_json;
     expected = R"([])"_json;
     result = permuto::process(template_json, context);
     EXPECT_EQ(result, expected);

     template_json = R"("a string")"_json;
     expected = R"("a string")"_json;
     result = permuto::process(template_json, context);
     EXPECT_EQ(result, expected);
}

TEST(PermutoEdgeCases, DelimitersInText) {
    json template_json = R"( { "msg": "This is ${not a var} but ${this.is}", "brackets": "Some { curly } text" } )"_json;
    json context = R"( { "this": { "is": "substituted"} } )"_json;
    json expected = R"( { "msg": "This is ${not a var} but substituted", "brackets": "Some { curly } text" } )"_json;

    permuto::Options options;
    options.enableStringInterpolation = true;
    json result = permuto::process(template_json, context, options);
    EXPECT_EQ(result, expected);
}

TEST(PermutoEdgeCases, NestedStructures) {
     json template_json = R"(
        {
            "level1": {
                "a": "${top.val}",
                "level2": [
                    "${arr[0]}",
                    { "b": "${nested.num}" },
                    "String with ${interpolation}"
                ]
            }
        }
    )"_json;
     json context = R"(
        {
            "top": {"val": true},
            "nested": {"num": 99},
            "interpolation": "value",
            "arr": ["should_not_be_used"]
        }
    )"_json;
     json expected = R"(
        {
            "level1": {
                "a": true,
                "level2": [
                    "${arr[0]}",
                    { "b": 99 },
                    "String with value"
                ]
            }
        }
    )"_json;
    permuto::Options options;
    options.enableStringInterpolation = true;
    json result = permuto::process(template_json, context, options);
    EXPECT_EQ(result, expected);
}
