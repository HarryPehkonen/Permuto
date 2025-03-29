// clavis/tests/clavis_test.cpp
#include <gtest/gtest.h>
#include <clavis/clavis.hpp>
#include <clavis/exceptions.hpp>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// --- Basic Substitution Test ---
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

    // TODO: Uncomment and implement when clavis::process is ready
    // json result = clavis::process(template_json, context);
    // EXPECT_EQ(result, expected);

    // Placeholder assertion until implemented
    EXPECT_TRUE(true);
}

// --- Type Handling Test ---
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

    // TODO: Uncomment and implement
    // json result = clavis::process(template_json, context);
    // EXPECT_EQ(result, expected);

    EXPECT_TRUE(true); // Placeholder
}

// --- String Interpolation Test ---
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

    // TODO: Uncomment and implement
    // json result = clavis::process(template_json, context);
    // EXPECT_EQ(result, expected);

    EXPECT_TRUE(true); // Placeholder
}


// --- Missing Key Tests ---
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
    options.onMissingKey = clavis::MissingKeyBehavior::Ignore;

    // TODO: Uncomment and implement
    // json result = clavis::process(template_json, context, options);
    // EXPECT_EQ(result, expected);

    EXPECT_TRUE(true); // Placeholder
}

TEST(ClavisMissingKeyTests, ErrorMode) {
     json template_json = R"( {"missing": "${a.b.c}"} )"_json;
     json context = R"( {"a": {"b": {}}} )"_json; // 'c' is missing

    clavis::Options options;
    options.onMissingKey = clavis::MissingKeyBehavior::Error;

    // TODO: Uncomment and implement
    // EXPECT_THROW({
    //     try {
    //         clavis::process(template_json, context, options);
    //     } catch (const clavis::ClavisMissingKeyException& e) {
    //         EXPECT_NE(std::string(e.what()).find("a.b.c"), std::string::npos); // Check if path is mentioned
    //         EXPECT_EQ(e.get_key_path(), "a.b.c"); // Check specific path retrieval
    //         throw; // Re-throw to satisfy EXPECT_THROW
    //     }
    // }, clavis::ClavisMissingKeyException);

    EXPECT_TRUE(true); // Placeholder
}


// --- Cycle Detection Test ---
TEST(ClavisCycleTests, DirectCycleError) {
    json template_json = R"( {"a": "${b}", "b": "${a}"} )"_json;
    json context = R"( {"a": "${b}", "b": "${a}"} )"_json; // Define in context to trigger lookup

    // TODO: Uncomment and implement
    // EXPECT_THROW({
    //     try {
    //         clavis::process(template_json, context);
    //     } catch (const clavis::ClavisCycleException& e) {
    //         // Check that the path indicates the cycle reasonably (e.g., "a -> b -> a")
    //         std::string cycle_path = e.get_cycle_path();
    //         EXPECT_TRUE(cycle_path.find("a") != std::string::npos);
    //         EXPECT_TRUE(cycle_path.find("b") != std::string::npos);
    //         EXPECT_TRUE(cycle_path.length() > 3); // Basic sanity check
    //         throw;
    //     }
    // }, clavis::ClavisCycleException);
    EXPECT_TRUE(true); // Placeholder
}

TEST(ClavisCycleTests, IndirectCycleError) {
    json template_json = R"( {"start": "${path.to.b}"} )"_json;
    json context = R"(
        {
            "path": {"to": {"b": "${cycled.a}" } },
            "cycled": {"a": "${path.to.b}"}
        }
    )"_json;

    // TODO: Uncomment and implement
    // EXPECT_THROW({
    //      clavis::process(template_json, context);
    // }, clavis::ClavisCycleException);
    EXPECT_TRUE(true); // Placeholder
}


// --- Custom Delimiter Test ---
TEST(ClavisOptionTests, CustomDelimiters) {
    json template_json = R"( {"value": "<<data.item>>"} )"_json;
    json context = R"( {"data": {"item": "success"}} )"_json;
    json expected = R"( {"value": "success"} )"_json;

    clavis::Options options;
    options.variableStartMarker = "<<";
    options.variableEndMarker = ">>";

    // TODO: Uncomment and implement
    // json result = clavis::process(template_json, context, options);
    // EXPECT_EQ(result, expected);
    EXPECT_TRUE(true); // Placeholder
}

// TODO: Add more tests:
// - Nested structures in template
// - Empty context / empty template
// - Placeholders in keys (should not be processed)
// - Placeholders resolving to other placeholders (recursive substitution)
// - Edge cases with delimiters (e.g., adjacent placeholders, delimiters in text)
// - More complex dot notation paths
// - Invalid options validation (e.g., empty markers)
