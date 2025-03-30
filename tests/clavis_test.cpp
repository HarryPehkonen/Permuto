// clavis/tests/clavis_test.cpp
#include <gtest/gtest.h>
#include <clavis/clavis.hpp>
#include <clavis/exceptions.hpp>
#include <nlohmann/json.hpp>

// Include the internal header FOR TESTING detail functions DIRECTLY
#include "../src/clavis_internal.hpp" // Adjust path if necessary

#include <set>
#include <string>

using json = nlohmann::json;


// --- Test Suite for detail::resolve_path ---
class ClavisResolvePathTests : public ::testing::Test {
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

    clavis::Options ignoreOpts; // Default is ignore
    clavis::Options errorOpts;

    void SetUp() override {
        ignoreOpts.onMissingKey = clavis::MissingKeyBehavior::Ignore;
        errorOpts.onMissingKey = clavis::MissingKeyBehavior::Error;
    }
};

TEST_F(ClavisResolvePathTests, ResolvesTopLevelKey) {
    const json* result = clavis::detail::resolve_path(context, "settings", ignoreOpts, "${settings}");
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(*result, context["settings"]);
}

TEST_F(ClavisResolvePathTests, ResolvesNestedKey) {
    const json* result = clavis::detail::resolve_path(context, "user.address.city", ignoreOpts, "${user.address.city}");
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(*result, "Wonderland");
}

TEST_F(ClavisResolvePathTests, ResolvesToNullValue) {
    const json* result = clavis::detail::resolve_path(context, "top_level_null", ignoreOpts, "${top_level_null}");
    ASSERT_NE(result, nullptr);
    ASSERT_TRUE(result->is_null());
}

// Tests for keys needing JSON Pointer escaping
TEST_F(ClavisResolvePathTests, ResolvesKeyWithSlashes) {
     const json* result = clavis::detail::resolve_path(context, "key/with/slashes", ignoreOpts, "${key/with/slashes}");
     ASSERT_NE(result, nullptr);
     EXPECT_EQ(*result, "slashes_val");
}

TEST_F(ClavisResolvePathTests, ResolvesKeyWithTildes) {
     const json* result = clavis::detail::resolve_path(context, "key~with~tildes", ignoreOpts, "${key~with~tildes}");
     ASSERT_NE(result, nullptr);
     EXPECT_EQ(*result, "tildes_val");
}


TEST_F(ClavisResolvePathTests, HandlesMissingTopLevelKeyIgnore) {
    const json* result = clavis::detail::resolve_path(context, "nonexistent", ignoreOpts, "${nonexistent}");
    EXPECT_EQ(result, nullptr);
}

TEST_F(ClavisResolvePathTests, HandlesMissingNestedKeyIgnore) {
    const json* result = clavis::detail::resolve_path(context, "user.address.street", ignoreOpts, "${user.address.street}");
    EXPECT_EQ(result, nullptr);
}

TEST_F(ClavisResolvePathTests, HandlesMissingIntermediateKeyIgnore) {
    const json* result = clavis::detail::resolve_path(context, "user.profile.id", ignoreOpts, "${user.profile.id}");
    EXPECT_EQ(result, nullptr); // "profile" does not exist
}

TEST_F(ClavisResolvePathTests, HandlesMissingTopLevelKeyError) {
    EXPECT_THROW({
        try {
            clavis::detail::resolve_path(context, "nonexistent", errorOpts, "${nonexistent}");
        } catch (const clavis::ClavisMissingKeyException& e) {
            EXPECT_EQ(e.get_key_path(), "nonexistent");
            // Optionally check e.what() content too
            EXPECT_NE(std::string(e.what()).find("nonexistent"), std::string::npos);
            throw; // Re-throw
        }
    }, clavis::ClavisMissingKeyException);
}

TEST_F(ClavisResolvePathTests, HandlesMissingNestedKeyError) {
     EXPECT_THROW({
        try {
            clavis::detail::resolve_path(context, "user.address.street", errorOpts, "${user.address.street}");
        } catch (const clavis::ClavisMissingKeyException& e) {
            EXPECT_EQ(e.get_key_path(), "user.address.street");
             EXPECT_NE(std::string(e.what()).find("user.address.street"), std::string::npos);
            throw; // Re-throw
        }
    }, clavis::ClavisMissingKeyException);
}

TEST_F(ClavisResolvePathTests, HandlesMissingIntermediateKeyError) {
    EXPECT_THROW({
        try {
            clavis::detail::resolve_path(context, "user.profile.id", errorOpts, "${user.profile.id}");
        } catch (const clavis::ClavisMissingKeyException& e) {
            EXPECT_EQ(e.get_key_path(), "user.profile.id");
             EXPECT_NE(std::string(e.what()).find("user.profile.id"), std::string::npos);
            throw; // Re-throw
        }
    }, clavis::ClavisMissingKeyException);
}


TEST_F(ClavisResolvePathTests, HandlesTraversalIntoNonObjectIgnore) {
    // JSON pointer handles this by simply not finding the path
    const json* result = clavis::detail::resolve_path(context, "user.name.first", ignoreOpts, "${user.name.first}");
    EXPECT_EQ(result, nullptr);
    result = clavis::detail::resolve_path(context, "settings.enabled.value", ignoreOpts, "${settings.enabled.value}");
    EXPECT_EQ(result, nullptr);
    result = clavis::detail::resolve_path(context, "settings.values.key", ignoreOpts, "${settings.values.key}");
    EXPECT_EQ(result, nullptr); // Note: JSON pointer path for array index would be /settings/values/0 etc.
}

TEST_F(ClavisResolvePathTests, HandlesTraversalIntoNonObjectError) {
    // JSON pointer throws out_of_range which we map to ClavisMissingKeyException
    EXPECT_THROW({
        try {
            clavis::detail::resolve_path(context, "user.name.first", errorOpts, "${user.name.first}");
        } catch (const clavis::ClavisMissingKeyException& e) {
            EXPECT_EQ(e.get_key_path(), "user.name.first");
            throw; // Re-throw
        }
    }, clavis::ClavisMissingKeyException);
}

TEST_F(ClavisResolvePathTests, HandlesEmptyPathIgnore) {
     const json* result = clavis::detail::resolve_path(context, "", ignoreOpts, "${}");
     EXPECT_EQ(result, nullptr);
}

TEST_F(ClavisResolvePathTests, HandlesEmptyPathError) {
    EXPECT_THROW({
       clavis::detail::resolve_path(context, "", errorOpts, "${}");
    }, clavis::ClavisMissingKeyException);
}


TEST_F(ClavisResolvePathTests, HandlesInvalidPathFormatsIgnore) {
    EXPECT_EQ(clavis::detail::resolve_path(context, ".user", ignoreOpts, "${.user}"), nullptr);
    EXPECT_EQ(clavis::detail::resolve_path(context, "user.", ignoreOpts, "${user.}"), nullptr);
    EXPECT_EQ(clavis::detail::resolve_path(context, "user..name", ignoreOpts, "${user..name}"), nullptr);
}

TEST_F(ClavisResolvePathTests, HandlesInvalidPathFormatsError) {
     EXPECT_THROW( clavis::detail::resolve_path(context, ".user", errorOpts, "${.user}"), clavis::ClavisMissingKeyException);
     EXPECT_THROW( clavis::detail::resolve_path(context, "user.", errorOpts, "${user.}"), clavis::ClavisMissingKeyException);
     EXPECT_THROW( clavis::detail::resolve_path(context, "user..name", errorOpts, "${user..name}"), clavis::ClavisMissingKeyException);
}


// --- Test Suite for detail::process_string ---
class ClavisProcessStringTests : public ::testing::Test {
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

    clavis::Options ignoreOpts; // Default is ignore
    clavis::Options errorOpts;
    std::set<std::string> active_paths; // Fresh set for each call

    void SetUp() override {
        ignoreOpts.onMissingKey = clavis::MissingKeyBehavior::Ignore;
        errorOpts.onMissingKey = clavis::MissingKeyBehavior::Error;
        active_paths.clear(); // Ensure clean slate for each test
    }

    // Helper to call the detail function
    json process_str_opts(const std::string& tpl, const clavis::Options& opts) {
         active_paths.clear(); // Ensure clear before call
         return clavis::detail::process_string(tpl, context, opts, active_paths);
    }
     // Default to ignoreOpts if not specified
     json process_str(const std::string& tpl) {
        return process_str_opts(tpl, ignoreOpts);
     }

};

// Exact Match Tests
TEST_F(ClavisProcessStringTests, ExactMatchString) {
    json result = process_str("${user.name}");
    EXPECT_EQ(result.type(), json::value_t::string);
    EXPECT_EQ(result.get<std::string>(), "Alice");
}

TEST_F(ClavisProcessStringTests, ExactMatchNumber) {
    json result = process_str("${user.id}");
    EXPECT_TRUE(result.is_number());
    EXPECT_EQ(result.get<int>(), 123);
}

TEST_F(ClavisProcessStringTests, ExactMatchFloat) {
    json result = process_str("${pi}");
    EXPECT_EQ(result.type(), json::value_t::number_float);
    EXPECT_DOUBLE_EQ(result.get<double>(), 3.14);
}

TEST_F(ClavisProcessStringTests, ExactMatchBoolean) {
    json result = process_str("${enabled}");
    EXPECT_EQ(result.type(), json::value_t::boolean);
    EXPECT_EQ(result.get<bool>(), true);
}

TEST_F(ClavisProcessStringTests, ExactMatchNull) {
    json result = process_str("${nothing}");
    EXPECT_EQ(result.type(), json::value_t::null);
    EXPECT_TRUE(result.is_null());
}

TEST_F(ClavisProcessStringTests, ExactMatchArray) {
    json result = process_str("${items}");
    EXPECT_EQ(result.type(), json::value_t::array);
    EXPECT_EQ(result, json::parse("[1, \"two\"]"));
}

TEST_F(ClavisProcessStringTests, ExactMatchObject) {
    json result = process_str("${config}");
    EXPECT_EQ(result.type(), json::value_t::object);
    EXPECT_EQ(result, json::parse("{\"theme\": \"dark\"}"));
}

TEST_F(ClavisProcessStringTests, ExactMatchMissingIgnore) {
    json result = process_str_opts("${missing.key}", ignoreOpts);
    EXPECT_EQ(result.type(), json::value_t::string); // Returns placeholder string
    EXPECT_EQ(result.get<std::string>(), "${missing.key}");
}

TEST_F(ClavisProcessStringTests, ExactMatchMissingError) {
    EXPECT_THROW({
        process_str_opts("${missing.key}", errorOpts);
    }, clavis::ClavisMissingKeyException);
}

TEST_F(ClavisProcessStringTests, ExactMatchRecursion) {
    // ${ref} resolves to "${user.name}", which then resolves to "Alice"
    json result = process_str("${ref}");
    EXPECT_EQ(result.type(), json::value_t::string);
    EXPECT_EQ(result.get<std::string>(), "Alice");
}

TEST_F(ClavisProcessStringTests, ExactMatchRecursionNested) {
     // ${nested_ref} resolves to "User is ${ref}", which interpolates to "User is Alice"
    json result = process_str("${nested_ref}");
    EXPECT_EQ(result.type(), json::value_t::string);
    EXPECT_EQ(result.get<std::string>(), "User is Alice");
}

TEST_F(ClavisProcessStringTests, ExactMatchWithInnerMarkersShouldInterpolate) {
    // This looks like an exact match but contains markers, so treat as interpolation
    json result = process_str("${user.name} has ID ${user.id}"); // Not wrapped in outer literal quotes
    EXPECT_EQ(result.type(), json::value_t::string);
    EXPECT_EQ(result.get<std::string>(), "Alice has ID 123");
}

TEST_F(ClavisProcessStringTests, ExactMatchEmptyPlaceholder) {
    // ${} should be treated as literal string "${}" in exact match context
    json result = process_str("${}");
    EXPECT_EQ(result.type(), json::value_t::string);
    EXPECT_EQ(result.get<std::string>(), "${}");
}

TEST_F(ClavisProcessStringTests, ExactMatchResolvesToEmptyPlaceholder) {
    // ${empty_placeholder_ref} resolves to "${}" which should be returned literally
     json result = process_str("${empty_placeholder_ref}");
     EXPECT_EQ(result.type(), json::value_t::string);
     EXPECT_EQ(result.get<std::string>(), "${}");
}


// Interpolation Tests
TEST_F(ClavisProcessStringTests, InterpolationSimple) {
    json result = process_str("Name: ${user.name}");
    EXPECT_EQ(result.type(), json::value_t::string);
    EXPECT_EQ(result.get<std::string>(), "Name: Alice");
}

TEST_F(ClavisProcessStringTests, InterpolationMultiple) {
    json result = process_str("${user.name} (${user.id})");
    EXPECT_EQ(result.get<std::string>(), "Alice (123)");
}

TEST_F(ClavisProcessStringTests, InterpolationMixedTypes) {
    json result = process_str("User: ${user.name}, ID: ${user.id}, Enabled: ${enabled}, Pi: ${pi}, Nada: ${nothing}, Items: ${items}, Cfg: ${config}, Html: ${html}");
    // Note the stringification of array/object via dump()
    EXPECT_EQ(result.get<std::string>(), "User: Alice, ID: 123, Enabled: true, Pi: 3.14, Nada: null, Items: [1,\"two\"], Cfg: {\"theme\":\"dark\"}, Html: <p>Hello</p>");
}

TEST_F(ClavisProcessStringTests, InterpolationMissingIgnore) {
    json result = process_str_opts("Val: ${user.name} Missing: ${missing.key} End", ignoreOpts);
    EXPECT_EQ(result.get<std::string>(), "Val: Alice Missing: ${missing.key} End");
}

TEST_F(ClavisProcessStringTests, InterpolationMissingError) {
     EXPECT_THROW({
        process_str_opts("Val: ${user.name} Missing: ${missing.key} End", errorOpts);
    }, clavis::ClavisMissingKeyException);
}

TEST_F(ClavisProcessStringTests, InterpolationWithRecursion) {
    // "User is ${ref}" -> "User is ${user.name}" -> "User is Alice"
    json result = process_str("Info: ${nested_ref}");
    EXPECT_EQ(result.get<std::string>(), "Info: User is Alice");
}

TEST_F(ClavisProcessStringTests, InterpolationAdjacent) {
    json result = process_str("${user.name}${user.id}");
    EXPECT_EQ(result.get<std::string>(), "Alice123");
}

TEST_F(ClavisProcessStringTests, InterpolationOnlyPlaceholders) {
    json result = process_str("${user.name}:${enabled}");
     EXPECT_EQ(result.get<std::string>(), "Alice:true");
}

TEST_F(ClavisProcessStringTests, InterpolationUnterminated) {
     json result = process_str("Hello ${user.name} and ${unclosed");
     EXPECT_EQ(result.get<std::string>(), "Hello Alice and ${unclosed");
}

TEST_F(ClavisProcessStringTests, InterpolationEmptyPlaceholder) {
    // ${} should be treated as literal string "${}" in interpolation context
    json result = process_str("Value: ${}");
    EXPECT_EQ(result.type(), json::value_t::string);
    EXPECT_EQ(result.get<std::string>(), "Value: ${}");
}

TEST_F(ClavisProcessStringTests, InterpolationResolvesToEmptyPlaceholder) {
    // ${empty_placeholder_ref} resolves to "${}" which should be included literally
     json result = process_str("Value: ${empty_placeholder_ref} End");
     EXPECT_EQ(result.type(), json::value_t::string);
     EXPECT_EQ(result.get<std::string>(), "Value: ${} End");
}


TEST_F(ClavisProcessStringTests, StringNoPlaceholders) {
    json result = process_str("Just plain text.");
    EXPECT_EQ(result.get<std::string>(), "Just plain text.");
}

TEST_F(ClavisProcessStringTests, StringEmpty) {
    json result = process_str("");
    EXPECT_EQ(result.get<std::string>(), "");
}


// Cycle Tests
TEST_F(ClavisProcessStringTests, CycleExactMatch) {
    EXPECT_THROW({
        process_str("${cycle_a}");
    }, clavis::ClavisCycleException);
}

TEST_F(ClavisProcessStringTests, CycleInterpolation) {
     EXPECT_THROW({
        process_str("Cycle here: ${cycle_a}");
    }, clavis::ClavisCycleException);
}

TEST_F(ClavisProcessStringTests, CycleDeepExact) {
     EXPECT_THROW({
        process_str("${deep_cycle_a}");
    }, clavis::ClavisCycleException);
}

TEST_F(ClavisProcessStringTests, CycleDeepInterpolation) {
     EXPECT_THROW({
        process_str("Deep cycle: ${deep_cycle_a}...");
    }, clavis::ClavisCycleException);
}

// Custom Delimiter Tests (using process_str_opts)
TEST_F(ClavisProcessStringTests, CustomDelimitersExact) {
    clavis::Options opts;
    opts.variableStartMarker = "<<";
    opts.variableEndMarker = ">>";
    json result = process_str_opts("<<user.name>>", opts);
    EXPECT_EQ(result.get<std::string>(), "Alice");
}

TEST_F(ClavisProcessStringTests, CustomDelimitersInterpolation) {
    clavis::Options opts;
    opts.variableStartMarker = "::";
    opts.variableEndMarker = "::";
    json result = process_str_opts("ID is ::user.id:: Status ::enabled::", opts);
    EXPECT_EQ(result.get<std::string>(), "ID is 123 Status true");
}

TEST_F(ClavisProcessStringTests, CustomDelimitersInterpolationAdjacent) {
    clavis::Options opts;
    opts.variableStartMarker = "%%";
    opts.variableEndMarker = "%%";
    json result = process_str_opts("%%user.name%%%%user.id%%", opts);
    EXPECT_EQ(result.get<std::string>(), "Alice123");
}

// --- End of detail::process_string tests ---


// --- Basic Substitution Test (Using clavis::process) ---
TEST(ClavisBasicTests, SimpleSubstitution) {
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

    json result = clavis::process(template_json, context);
    EXPECT_EQ(result, expected);
}

// --- Type Handling Test (Using clavis::process) ---
TEST(ClavisBasicTests, ExactMatchTypePreservation) {
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

    json result = clavis::process(template_json, context);
    EXPECT_EQ(result, expected);
}

// --- String Interpolation Test (Using clavis::process) ---
TEST(ClavisBasicTests, InterpolationTypeConversion) {
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

    json result = clavis::process(template_json, context);
    EXPECT_EQ(result, expected);
}


// --- Missing Key Tests (Using clavis::process) ---
TEST(ClavisMissingKeyTests, IgnoreMode) {
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

    clavis::Options options;
    options.onMissingKey = clavis::MissingKeyBehavior::Ignore; // Explicitly set default for clarity

    json result = clavis::process(template_json, context, options);
    EXPECT_EQ(result, expected);
}

TEST(ClavisMissingKeyTests, ErrorMode) {
     json template_json = R"( {"missing": "${a.b.c}"} )"_json;
     json context = R"( {"a": {"b": {}}} )"_json; // 'c' is missing

    clavis::Options options;
    options.onMissingKey = clavis::MissingKeyBehavior::Error;

    EXPECT_THROW({
        try {
            clavis::process(template_json, context, options);
        } catch (const clavis::ClavisMissingKeyException& e) {
            EXPECT_EQ(e.get_key_path(), "a.b.c"); // Check specific path retrieval
            EXPECT_NE(std::string(e.what()).find("a.b.c"), std::string::npos); // Check if path is mentioned
            throw; // Re-throw to satisfy EXPECT_THROW
        }
    }, clavis::ClavisMissingKeyException);
}

TEST(ClavisMissingKeyTests, ErrorModeInterpolation) {
     json template_json = R"( {"msg": "Hello ${a.b.c}"} )"_json; // Missing key in interpolation
     json context = R"( {"a": {"b": {}}} )"_json;

    clavis::Options options;
    options.onMissingKey = clavis::MissingKeyBehavior::Error;

    EXPECT_THROW({
        clavis::process(template_json, context, options);
    }, clavis::ClavisMissingKeyException);
}


// --- Cycle Detection Test (Using clavis::process) ---
TEST(ClavisCycleTests, DirectCycleError) {
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
            clavis::process(template_json, context_cycle); // Process the template using cycling context
        } catch (const clavis::ClavisCycleException& e) {
            std::string cycle_path = e.get_cycle_path();
             // Cycle detection path might not be perfectly predictable ("a -> b" or "b -> a")
            EXPECT_NE(cycle_path.find("a"), std::string::npos);
            EXPECT_NE(cycle_path.find("b"), std::string::npos);
            throw;
        }
    }, clavis::ClavisCycleException);
}

TEST(ClavisCycleTests, IndirectCycleError) {
    json template_json = R"( {"start": "${path.to.b}"} )"_json;
    json context = R"(
        {
            "path": {"to": {"b": "${cycled.a}" } },
            "cycled": {"a": "${path.to.b}"}
        }
    )"_json;

    EXPECT_THROW({
         clavis::process(template_json, context);
    }, clavis::ClavisCycleException);
}

TEST(ClavisCycleTests, SelfReferenceCycleError) {
    json template_json = R"( {"a": "${a}"} )"_json;
    json context = R"( {"a": "${a}"} )"_json; // Value points to itself

     EXPECT_THROW({
         clavis::process(template_json, context);
     }, clavis::ClavisCycleException);
}


// --- Custom Delimiter Test (Using clavis::process) ---
TEST(ClavisOptionTests, CustomDelimiters) {
    json template_json = R"( {"value": "<<data.item>>", "msg": "Item is: <<data.item>>"} )"_json;
    json context = R"( {"data": {"item": "success"}} )"_json;
    json expected = R"( {"value": "success", "msg": "Item is: success"} )"_json;

    clavis::Options options;
    options.variableStartMarker = "<<";
    options.variableEndMarker = ">>";

    json result = clavis::process(template_json, context, options);
    EXPECT_EQ(result, expected);
}

TEST(ClavisOptionTests, InvalidOptions) {
    json template_json = R"({})"_json;
    json context = R"({})"_json;
    clavis::Options options;

    options.variableStartMarker = "";
    EXPECT_THROW(clavis::process(template_json, context, options), std::invalid_argument);

    options.variableStartMarker = "{"; options.variableEndMarker = "";
    EXPECT_THROW(clavis::process(template_json, context, options), std::invalid_argument);

     options.variableStartMarker = "@@"; options.variableEndMarker = "@@";
    EXPECT_THROW(clavis::process(template_json, context, options), std::invalid_argument);
}

// --- Edge Case Tests ---
TEST(ClavisEdgeCases, PlaceholdersInKeys) {
    // Keys should not be processed
    json template_json = R"( { "${a.key}": "value", "static_key": "${val}" } )"_json;
    json context = R"( { "val": "the_value", "a": { "key": "ignored"} } )"_json;
    json expected = R"( { "${a.key}": "value", "static_key": "the_value" } )"_json;

    json result = clavis::process(template_json, context);
    EXPECT_EQ(result, expected);
}

TEST(ClavisEdgeCases, EmptyTemplateAndContext) {
     json template_json = R"({})"_json;
     json context = R"({})"_json;
     json expected = R"({})"_json;
     json result = clavis::process(template_json, context);
     EXPECT_EQ(result, expected);

     template_json = R"([])"_json;
     expected = R"([])"_json;
     result = clavis::process(template_json, context);
     EXPECT_EQ(result, expected);

     template_json = R"("a string")"_json;
     expected = R"("a string")"_json;
     result = clavis::process(template_json, context);
     EXPECT_EQ(result, expected);
}

TEST(ClavisEdgeCases, DelimitersInText) {
    json template_json = R"( { "msg": "This is ${not a var} but ${this.is}", "brackets": "Some { curly } text" } )"_json;
    json context = R"( { "this": { "is": "substituted"} } )"_json;
    json expected = R"( { "msg": "This is ${not a var} but substituted", "brackets": "Some { curly } text" } )"_json;

    json result = clavis::process(template_json, context);
    EXPECT_EQ(result, expected);
}

TEST(ClavisEdgeCases, NestedStructures) {
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
    json result = clavis::process(template_json, context);
    EXPECT_EQ(result, expected);
}
