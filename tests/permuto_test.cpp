#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include "permuto/permuto.hpp"
#include "permuto/exceptions.hpp"

using json = nlohmann::json;

// --- Test Suite for Public API ---

TEST(PermutoPublicApiTests, SimpleSubstitution_InterpOn) {
    json template_json = R"({"name": "${user.name}", "age": "${user.age}"})"_json;
    json context = R"({"user": {"name": "Alice", "age": 30}})"_json;
    
    permuto::Options opts;
    opts.enableStringInterpolation = true;
    
    auto result = permuto::apply(template_json, context, opts);
    
    EXPECT_EQ(result["name"], "Alice");
    EXPECT_EQ(result["age"], "30"); // Stringified due to interpolation
}

TEST(PermutoPublicApiTests, SimpleSubstitution_InterpOff) {
    json template_json = R"({"name": "${user.name}", "age": "${user.age}"})"_json;
    json context = R"({"user": {"name": "Alice", "age": 30}})"_json;
    
    permuto::Options opts;
    opts.enableStringInterpolation = false;
    
    auto result = permuto::apply(template_json, context, opts);
    
    EXPECT_EQ(result["name"], "Alice");
    EXPECT_EQ(result["age"], 30); // Preserved as number
}

TEST(PermutoPublicApiTests, SimpleSubstitution_DefaultOpts) {
    json template_json = R"({"name": "${user.name}", "age": "${user.age}"})"_json;
    json context = R"({"user": {"name": "Alice", "age": 30}})"_json;
    
    // Use default options (interpolation OFF, ignore missing)
    auto result = permuto::apply(template_json, context);
    
    EXPECT_EQ(result["name"], "Alice");
    EXPECT_EQ(result["age"], 30); // Preserved as number
}

TEST(PermutoPublicApiTests, ExactMatchTypePreservation_InterpOff) {
    json template_json = R"({
        "string_val": "${user.name}",
        "number_val": "${user.age}",
        "bool_val": "${user.active}",
        "null_val": "${user.nickname}",
        "array_val": "${user.hobbies}",
        "object_val": "${user.address}"
    })"_json;
    
    json context = R"({
        "user": {
            "name": "Alice",
            "age": 30,
            "active": true,
            "nickname": null,
            "hobbies": ["reading", "swimming"],
            "address": {"city": "Wonderland", "country": "Fantasy"}
        }
    })"_json;
    
    permuto::Options opts;
    opts.enableStringInterpolation = false;
    
    auto result = permuto::apply(template_json, context, opts);
    
    EXPECT_EQ(result["string_val"], "Alice");
    EXPECT_EQ(result["number_val"], 30);
    EXPECT_EQ(result["bool_val"], true);
    EXPECT_TRUE(result["null_val"].is_null());
    EXPECT_EQ(result["array_val"], context["user"]["hobbies"]);
    EXPECT_EQ(result["object_val"], context["user"]["address"]);
}

TEST(PermutoPublicApiTests, ExactMatchTypePreservation_InterpOn) {
    json template_json = R"({
        "string_val": "${user.name}",
        "number_val": "${user.age}",
        "bool_val": "${user.active}",
        "null_val": "${user.nickname}",
        "array_val": "${user.hobbies}",
        "object_val": "${user.address}"
    })"_json;
    
    json context = R"({
        "user": {
            "name": "Alice",
            "age": 30,
            "active": true,
            "nickname": null,
            "hobbies": ["reading", "swimming"],
            "address": {"city": "Wonderland", "country": "Fantasy"}
        }
    })"_json;
    
    permuto::Options opts;
    opts.enableStringInterpolation = true;
    
    auto result = permuto::apply(template_json, context, opts);
    
    EXPECT_EQ(result["string_val"], "Alice");
    EXPECT_EQ(result["number_val"], "30"); // Stringified
    EXPECT_EQ(result["bool_val"], "true"); // Stringified
    EXPECT_EQ(result["null_val"], "null"); // Stringified
    EXPECT_EQ(result["array_val"], "[\"reading\",\"swimming\"]"); // Stringified
    EXPECT_EQ(result["object_val"], "{\"city\":\"Wonderland\",\"country\":\"Fantasy\"}"); // Stringified
}

TEST(PermutoPublicApiTests, InterpolationTypeConversion_InterpOn) {
    json template_json = R"({"message": "Hello ${user.name}, you are ${user.age} years old"})"_json;
    json context = R"({"user": {"name": "Alice", "age": 30}})"_json;
    
    permuto::Options opts;
    opts.enableStringInterpolation = true;
    
    auto result = permuto::apply(template_json, context, opts);
    
    EXPECT_EQ(result["message"], "Hello Alice, you are 30 years old");
}

TEST(PermutoPublicApiTests, InterpolationTypeConversion_InterpOff) {
    json template_json = R"({"message": "Hello ${user.name}, you are ${user.age} years old"})"_json;
    json context = R"({"user": {"name": "Alice", "age": 30}})"_json;
    
    permuto::Options opts;
    opts.enableStringInterpolation = false;
    
    auto result = permuto::apply(template_json, context, opts);
    
    // Should remain unchanged since it's not an exact match
    EXPECT_EQ(result["message"], "Hello ${user.name}, you are ${user.age} years old");
}

TEST(PermutoPublicApiTests, MissingKey_Ignore_InterpOff) {
    json template_json = R"({"name": "${user.name}", "missing": "${user.missing}"})"_json;
    json context = R"({"user": {"name": "Alice"}})"_json;
    
    permuto::Options opts;
    opts.enableStringInterpolation = false;
    opts.onMissingKey = permuto::MissingKeyBehavior::Ignore;
    
    auto result = permuto::apply(template_json, context, opts);
    
    EXPECT_EQ(result["name"], "Alice");
    EXPECT_EQ(result["missing"], "${user.missing}"); // Left as-is
}

TEST(PermutoPublicApiTests, MissingKey_Ignore_InterpOn) {
    json template_json = R"({"name": "${user.name}", "missing": "${user.missing}"})"_json;
    json context = R"({"user": {"name": "Alice"}})"_json;
    
    permuto::Options opts;
    opts.enableStringInterpolation = true;
    opts.onMissingKey = permuto::MissingKeyBehavior::Ignore;
    
    auto result = permuto::apply(template_json, context, opts);
    
    EXPECT_EQ(result["name"], "Alice");
    EXPECT_EQ(result["missing"], "${user.missing}"); // Left as-is
}

TEST(PermutoPublicApiTests, MissingKey_Error_Exact_InterpOff) {
    json template_json = R"({"name": "${user.missing}"})"_json;
    json context = R"({"user": {"name": "Alice"}})"_json;
    
    permuto::Options opts;
    opts.enableStringInterpolation = false;
    opts.onMissingKey = permuto::MissingKeyBehavior::Error;
    
    EXPECT_THROW(permuto::apply(template_json, context, opts), permuto::PermutoMissingKeyException);
}

TEST(PermutoPublicApiTests, MissingKey_Error_Exact_InterpOn) {
    json template_json = R"({"name": "${user.missing}"})"_json;
    json context = R"({"user": {"name": "Alice"}})"_json;
    
    permuto::Options opts;
    opts.enableStringInterpolation = true;
    opts.onMissingKey = permuto::MissingKeyBehavior::Error;
    
    EXPECT_THROW(permuto::apply(template_json, context, opts), permuto::PermutoMissingKeyException);
}

TEST(PermutoPublicApiTests, MissingKey_Error_Interpolation_InterpOn) {
    json template_json = R"({"message": "Hello ${user.missing}"})"_json;
    json context = R"({"user": {"name": "Alice"}})"_json;
    
    permuto::Options opts;
    opts.enableStringInterpolation = true;
    opts.onMissingKey = permuto::MissingKeyBehavior::Error;
    
    EXPECT_THROW(permuto::apply(template_json, context, opts), permuto::PermutoMissingKeyException);
}

TEST(PermutoPublicApiTests, MissingKey_Error_Interpolation_InterpOff) {
    json template_json = R"({"message": "Hello ${user.missing}"})"_json;
    json context = R"({"user": {"name": "Alice"}})"_json;
    
    permuto::Options opts;
    opts.enableStringInterpolation = false;
    opts.onMissingKey = permuto::MissingKeyBehavior::Error;
    
    // Should not throw since interpolation is off and it's not an exact match
    auto result = permuto::apply(template_json, context, opts);
    EXPECT_EQ(result["message"], "Hello ${user.missing}");
}

TEST(PermutoPublicApiTests, DirectCycle_Exact_InterpOff) {
    json template_json = R"({"value": "${cycle.value}"})"_json;
    json context = R"({"cycle": {"value": "${cycle.value}"}})"_json;
    
    permuto::Options opts;
    opts.enableStringInterpolation = false;
    
    EXPECT_THROW(permuto::apply(template_json, context, opts), permuto::PermutoCycleException);
}

TEST(PermutoPublicApiTests, DirectCycle_Exact_InterpOn) {
    json template_json = R"({"value": "${cycle.value}"})"_json;
    json context = R"({"cycle": {"value": "${cycle.value}"}})"_json;
    
    permuto::Options opts;
    opts.enableStringInterpolation = true;
    
    EXPECT_THROW(permuto::apply(template_json, context, opts), permuto::PermutoCycleException);
}

TEST(PermutoPublicApiTests, IndirectCycle_Exact_InterpOff) {
    json template_json = R"({"value": "${a.value}"})"_json;
    json context = R"({
        "a": {"value": "${b.value}"},
        "b": {"value": "${a.value}"}
    })"_json;
    
    permuto::Options opts;
    opts.enableStringInterpolation = false;
    
    EXPECT_THROW(permuto::apply(template_json, context, opts), permuto::PermutoCycleException);
}

TEST(PermutoPublicApiTests, IndirectCycle_Exact_InterpOn) {
    json template_json = R"({"value": "${a.value}"})"_json;
    json context = R"({
        "a": {"value": "${b.value}"},
        "b": {"value": "${a.value}"}
    })"_json;
    
    permuto::Options opts;
    opts.enableStringInterpolation = true;
    
    EXPECT_THROW(permuto::apply(template_json, context, opts), permuto::PermutoCycleException);
}

TEST(PermutoPublicApiTests, SelfReferenceCycle_Exact_InterpOff) {
    json template_json = R"({"value": "${value}"})"_json;
    json context = R"({"value": "${value}"})"_json;
    
    permuto::Options opts;
    opts.enableStringInterpolation = false;
    
    EXPECT_THROW(permuto::apply(template_json, context, opts), permuto::PermutoCycleException);
}

TEST(PermutoPublicApiTests, SelfReferenceCycle_Exact_InterpOn) {
    json template_json = R"({"value": "${value}"})"_json;
    json context = R"({"value": "${value}"})"_json;
    
    permuto::Options opts;
    opts.enableStringInterpolation = true;
    
    EXPECT_THROW(permuto::apply(template_json, context, opts), permuto::PermutoCycleException);
}

TEST(PermutoPublicApiTests, CycleTriggeredByInterpolation_InterpOn) {
    json template_json = R"({"message": "Value: ${cycle.value}"})"_json;
    json context = R"({"cycle": {"value": "${cycle.value}"}})"_json;
    
    permuto::Options opts;
    opts.enableStringInterpolation = true;
    
    EXPECT_THROW(permuto::apply(template_json, context, opts), permuto::PermutoCycleException);
}

TEST(PermutoPublicApiTests, CycleTriggeredByInterpolation_InterpOff) {
    json template_json = R"({"message": "Value: ${cycle.value}"})"_json;
    json context = R"({"cycle": {"value": "${cycle.value}"}})"_json;
    
    permuto::Options opts;
    opts.enableStringInterpolation = false;
    
    // Should not throw since interpolation is off
    auto result = permuto::apply(template_json, context, opts);
    EXPECT_EQ(result["message"], "Value: ${cycle.value}");
}

TEST(PermutoPublicApiTests, CustomDelimiters_InterpOn) {
    json template_json = R"({"name": "[[user.name]]", "age": "[[user.age]]"})"_json;
    json context = R"({"user": {"name": "Alice", "age": 30}})"_json;
    
    permuto::Options opts;
    opts.variableStartMarker = "[[";
    opts.variableEndMarker = "]]";
    opts.enableStringInterpolation = true;
    
    auto result = permuto::apply(template_json, context, opts);
    
    EXPECT_EQ(result["name"], "Alice");
    EXPECT_EQ(result["age"], "30");
}

TEST(PermutoPublicApiTests, CustomDelimiters_InterpOff) {
    json template_json = R"({"name": "[[user.name]]", "age": "[[user.age]]"})"_json;
    json context = R"({"user": {"name": "Alice", "age": 30}})"_json;
    
    permuto::Options opts;
    opts.variableStartMarker = "[[";
    opts.variableEndMarker = "]]";
    opts.enableStringInterpolation = false;
    
    auto result = permuto::apply(template_json, context, opts);
    
    EXPECT_EQ(result["name"], "Alice");
    EXPECT_EQ(result["age"], 30);
}

TEST(PermutoPublicApiTests, InvalidOptions_Apply) {
    json template_json = R"({"name": "${user.name}"})"_json;
    json context = R"({"user": {"name": "Alice"}})"_json;
    
    permuto::Options opts;
    opts.variableStartMarker = ""; // Invalid: empty start marker
    
    EXPECT_THROW(permuto::apply(template_json, context, opts), std::invalid_argument);
}

TEST(PermutoPublicApiTests, InvalidOptions_CreateReverse) {
    json template_json = R"({"name": "${user.name}"})"_json;
    
    permuto::Options opts;
    opts.variableStartMarker = ""; // Invalid: empty start marker
    
    EXPECT_THROW(permuto::create_reverse_template(template_json, opts), std::invalid_argument);
}

TEST(PermutoPublicApiTests, PlaceholdersInKeys_InterpOff) { // Keys are never processed
    json template_json = R"({"${user.name}": "value"})"_json;
    json context = R"({"user": {"name": "Alice"}})"_json;
    
    permuto::Options opts;
    opts.enableStringInterpolation = false;
    
    auto result = permuto::apply(template_json, context, opts);
    EXPECT_EQ(result["${user.name}"], "value"); // Key unchanged
}

TEST(PermutoPublicApiTests, PlaceholdersInKeys_InterpOn) { // Keys are never processed
    json template_json = R"({"${user.name}": "value"})"_json;
    json context = R"({"user": {"name": "Alice"}})"_json;
    
    permuto::Options opts;
    opts.enableStringInterpolation = true;
    
    auto result = permuto::apply(template_json, context, opts);
    EXPECT_EQ(result["${user.name}"], "value"); // Key unchanged
}

TEST(PermutoPublicApiTests, EmptyTemplateAndContext) { // Behavior independent of options
    json template_json = R"({})"_json;
    json context = R"({})"_json;
    
    auto result = permuto::apply(template_json, context);
    EXPECT_EQ(result, template_json);
}

TEST(PermutoPublicApiTests, DelimitersInText_InterpOn) {
    json template_json = R"({"text": "This contains ${ and } characters"})"_json;
    json context = R"({"user": {"name": "Alice"}})"_json;
    
    permuto::Options opts;
    opts.enableStringInterpolation = true;
    
    auto result = permuto::apply(template_json, context, opts);
    EXPECT_EQ(result["text"], "This contains ${ and } characters");
}

TEST(PermutoPublicApiTests, DelimitersInText_InterpOff) {
    json template_json = R"({"text": "This contains ${ and } characters"})"_json;
    json context = R"({"user": {"name": "Alice"}})"_json;
    
    permuto::Options opts;
    opts.enableStringInterpolation = false;
    
    auto result = permuto::apply(template_json, context, opts);
    EXPECT_EQ(result["text"], "This contains ${ and } characters");
}

TEST(PermutoPublicApiTests, NestedStructures_InterpOn) {
    json template_json = R"({
        "user": {
            "name": "${user.name}",
            "profile": {
                "age": "${user.age}",
                "active": "${user.active}"
            }
        },
        "settings": ["${setting1}", "${setting2}"]
    })"_json;
    
    json context = R"({
        "user": {"name": "Alice", "age": 30, "active": true},
        "setting1": "enabled",
        "setting2": "disabled"
    })"_json;
    
    permuto::Options opts;
    opts.enableStringInterpolation = true;
    
    auto result = permuto::apply(template_json, context, opts);
    
    EXPECT_EQ(result["user"]["name"], "Alice");
    EXPECT_EQ(result["user"]["profile"]["age"], "30");
    EXPECT_EQ(result["user"]["profile"]["active"], "true");
    EXPECT_EQ(result["settings"][0], "enabled");
    EXPECT_EQ(result["settings"][1], "disabled");
}

TEST(PermutoPublicApiTests, NestedStructures_InterpOff) {
    json template_json = R"({
        "user": {
            "name": "${user.name}",
            "profile": {
                "age": "${user.age}",
                "active": "${user.active}"
            }
        },
        "settings": ["${setting1}", "${setting2}"]
    })"_json;
    
    json context = R"({
        "user": {"name": "Alice", "age": 30, "active": true},
        "setting1": "enabled",
        "setting2": "disabled"
    })"_json;
    
    permuto::Options opts;
    opts.enableStringInterpolation = false;
    
    auto result = permuto::apply(template_json, context, opts);
    
    EXPECT_EQ(result["user"]["name"], "Alice");
    EXPECT_EQ(result["user"]["profile"]["age"], 30);
    EXPECT_EQ(result["user"]["profile"]["active"], true);
    EXPECT_EQ(result["settings"][0], "enabled");
    EXPECT_EQ(result["settings"][1], "disabled");
}

// --- Test Suite for Reverse Template Operations ---

class PermutoCreateReverseTests : public ::testing::Test {
protected:
    permuto::Options optsInterpOff; // Must use interp off (DEFAULT)
    permuto::Options optsInterpOn; // Used for error case
    
    void SetUp() override {
        optsInterpOff.enableStringInterpolation = false;
        optsInterpOff.onMissingKey = permuto::MissingKeyBehavior::Ignore;
        
        optsInterpOn.enableStringInterpolation = true;
        optsInterpOn.onMissingKey = permuto::MissingKeyBehavior::Ignore;
    }
};

TEST_F(PermutoCreateReverseTests, BasicReverseTemplate) {
    json template_json = R"({
        "userName": "${user.name}",
        "userAge": "${user.age}"
    })"_json;
    
    auto reverse_template = permuto::create_reverse_template(template_json, optsInterpOff);
    
    json expected = R"({
        "user": {
            "name": "/userName",
            "age": "/userAge"
        }
    })"_json;
    
    EXPECT_EQ(reverse_template, expected);
}

TEST_F(PermutoCreateReverseTests, ReverseTemplateWithArrays) {
    json template_json = R"({
        "items": ["${item1}", "${item2}"],
        "matrix": [
            ["${cell00}", "${cell01}"],
            ["${cell10}", "${cell11}"]
        ]
    })"_json;
    
    auto reverse_template = permuto::create_reverse_template(template_json, optsInterpOff);
    
    json expected = R"({
        "item1": "/items/0",
        "item2": "/items/1",
        "cell00": "/matrix/0/0",
        "cell01": "/matrix/0/1",
        "cell10": "/matrix/1/0",
        "cell11": "/matrix/1/1"
    })"_json;
    
    EXPECT_EQ(reverse_template, expected);
}

TEST_F(PermutoCreateReverseTests, ReverseTemplateWithSpecialKeys) {
    json template_json = R"({
        "key~1with~1slashes": "${key/with/slashes}",
        "key~0with~0tildes": "${key~with~tildes}",
        "user.name": "${user.name}"
    })"_json;
    
    auto reverse_template = permuto::create_reverse_template(template_json, optsInterpOff);
    
    json expected = R"({
        "key/with/slashes": "/key~1with~1slashes",
        "key~with~tildes": "/key~0with~0tildes",
        "user": {
            "name": "/user.name"
        }
    })"_json;
    
    EXPECT_EQ(reverse_template, expected);
}

TEST_F(PermutoCreateReverseTests, EmptyTemplate) {
    json template_json = R"({})"_json;
    
    auto reverse_template = permuto::create_reverse_template(template_json, optsInterpOff);
    
    EXPECT_EQ(reverse_template, json::object());
}

TEST_F(PermutoCreateReverseTests, TemplateWithNoPlaceholders) {
    json template_json = R"({"name": "Alice", "age": 30})"_json;
    
    auto reverse_template = permuto::create_reverse_template(template_json, optsInterpOff);
    
    EXPECT_EQ(reverse_template, json::object());
}

TEST_F(PermutoCreateReverseTests, TemplateWithOnlyInvalidPlaceholders) {
    json template_json = R"({"name": "${}", "age": "${invalid"})"_json;
    
    auto reverse_template = permuto::create_reverse_template(template_json, optsInterpOff);
    
    EXPECT_EQ(reverse_template, json::object());
}

TEST_F(PermutoCreateReverseTests, ThrowsIfInterpolationEnabled) {
    json template_json = R"({"name": "${user.name}"})"_json;
    
    EXPECT_THROW(permuto::create_reverse_template(template_json, optsInterpOn), std::logic_error);
}

TEST_F(PermutoCreateReverseTests, CustomDelimiters) {
    json template_json = R"({
        "userName": "[[user.name]]",
        "userAge": "[[user.age]]"
    })"_json;
    
    permuto::Options opts;
    opts.variableStartMarker = "[[";
    opts.variableEndMarker = "]]";
    opts.enableStringInterpolation = false;
    
    auto reverse_template = permuto::create_reverse_template(template_json, opts);
    
    json expected = R"({
        "user": {
            "name": "/userName",
            "age": "/userAge"
        }
    })"_json;
    
    EXPECT_EQ(reverse_template, expected);
}

TEST_F(PermutoCreateReverseTests, ContextPathConflict) {
    json template_json = R"({
        "userName": "${user.name}",
        "userAge": "${user.age}",
        "user": "${user}" // This conflicts with the nested structure above
    })"_json;
    
    EXPECT_THROW(permuto::create_reverse_template(template_json, optsInterpOff), std::runtime_error);
}

TEST_F(PermutoCreateReverseTests, JsonPointerSyntax) {
    json template_json = R"({
        "key~1with~1slashes": "${key/with/slashes}",
        "key~0with~0tildes": "${key~with~tildes}",
        "array~1element": "${array/element}"
    })"_json;
    
    auto reverse_template = permuto::create_reverse_template(template_json, optsInterpOff);
    
    json expected = R"({
        "key/with/slashes": "/key~1with~1slashes",
        "key~with~tildes": "/key~0with~0tildes",
        "array/element": "/array~1element"
    })"_json;
    
    EXPECT_EQ(reverse_template, expected);
}

// --- Test Suite for Apply Reverse Operations ---

class PermutoApplyReverseTests : public ::testing::Test {
protected:
    json reverse_template = R"({
        "user": {
            "name": "/userName",
            "age": "/userAge"
        }
    })"_json;
    
    json result_json = R"({
        "userName": "Alice",
        "userAge": 30
    })"_json;
    
    json expected_context = R"({
        "user": {
            "name": "Alice",
            "age": 30
        }
    })"_json;
};

TEST_F(PermutoApplyReverseTests, BasicReconstruction) {
    auto reconstructed_context = permuto::apply_reverse(reverse_template, result_json);
    
    EXPECT_EQ(reconstructed_context, expected_context);
}

TEST_F(PermutoApplyReverseTests, EmptyReverseTemplate) {
    json empty_template = R"({})"_json;
    json result = R"({"any": "data"})"_json;
    
    auto reconstructed_context = permuto::apply_reverse(empty_template, result);
    
    EXPECT_EQ(reconstructed_context, json::object());
}

TEST_F(PermutoApplyReverseTests, PointerNotFoundInResult) {
    json bad_template = R"({"user": {"name": "/nonexistent"}})"_json;
    
    EXPECT_THROW(permuto::apply_reverse(bad_template, result_json), std::runtime_error);
}

TEST_F(PermutoApplyReverseTests, InvalidPointerSyntaxInReverseTemplate) {
    json bad_template = R"({"user": {"name": "invalid/pointer"}})"_json;
    
    EXPECT_THROW(permuto::apply_reverse(bad_template, result_json), std::runtime_error);
}

TEST_F(PermutoApplyReverseTests, MalformedReverseTemplate_NonObjectNode) {
    json bad_template = R"({"user": "not_an_object"})"_json;
    
    EXPECT_THROW(permuto::apply_reverse(bad_template, result_json), std::runtime_error);
}

TEST_F(PermutoApplyReverseTests, MalformedReverseTemplate_NonStringLeaf) {
    json bad_template = R"({"user": {"name": 123}})"_json;
    
    EXPECT_THROW(permuto::apply_reverse(bad_template, result_json), std::runtime_error);
}

TEST_F(PermutoApplyReverseTests, MalformedReverseTemplate_RootNotObject) {
    json bad_template = R"("not_an_object")"_json;
    
    EXPECT_THROW(permuto::apply_reverse(bad_template, result_json), std::runtime_error);
}

// --- Test Suite for Round Trip Operations ---

TEST(PermutoRoundTripTests, FullCycle) {
    // Original template
    json template_json = R"({
        "userName": "${user.name}",
        "userAge": "${user.age}"
    })"_json;
    
    // Original context
    json original_context = R"({
        "user": {
            "name": "Alice",
            "age": 30
        }
    })"_json;
    
    permuto::Options opts;
    opts.enableStringInterpolation = false;
    opts.onMissingKey = permuto::MissingKeyBehavior::Ignore;
    
    // Step 1: Apply template to get result
    auto result_json = permuto::apply(template_json, original_context, opts);
    
    // Step 2: Create reverse template
    auto reverse_template = permuto::create_reverse_template(template_json, opts);
    
    // Step 3: Apply reverse to reconstruct context
    auto reconstructed_context = permuto::apply_reverse(reverse_template, result_json);
    
    // Step 4: Verify round trip
    EXPECT_EQ(reconstructed_context, original_context);
}

// --- Test Suite for JsonPointer Utilities ---

TEST(PermutoPublicApiTests, JsonPointerBasicUsage) {
    EXPECT_TRUE(permuto::JsonPointerUtils::is_valid_pointer(""));
    EXPECT_TRUE(permuto::JsonPointerUtils::is_valid_pointer("/"));
    EXPECT_TRUE(permuto::JsonPointerUtils::is_valid_pointer("/user"));
    EXPECT_TRUE(permuto::JsonPointerUtils::is_valid_pointer("/user/name"));
    EXPECT_TRUE(permuto::JsonPointerUtils::is_valid_pointer("/items/0"));
    EXPECT_TRUE(permuto::JsonPointerUtils::is_valid_pointer("/items/123"));
    
    EXPECT_FALSE(permuto::JsonPointerUtils::is_valid_pointer("user"));
    EXPECT_FALSE(permuto::JsonPointerUtils::is_valid_pointer("user/name"));
    EXPECT_FALSE(permuto::JsonPointerUtils::is_valid_pointer("invalid"));
}

TEST(PermutoPublicApiTests, JsonPointerWithSpecialKeys) {
    EXPECT_TRUE(permuto::JsonPointerUtils::is_valid_pointer("/key~1with~1slashes"));
    EXPECT_TRUE(permuto::JsonPointerUtils::is_valid_pointer("/key~0with~0tildes"));
    
    auto segments1 = permuto::JsonPointerUtils::parse_segments("/key~1with~1slashes");
    EXPECT_EQ(segments1, std::vector<std::string>{"key/with/slashes"});
    
    auto segments2 = permuto::JsonPointerUtils::parse_segments("/key~0with~0tildes");
    EXPECT_EQ(segments2, std::vector<std::string>{"key~with~tildes"});
}

TEST(PermutoPublicApiTests, JsonPointerWithInterpolation) {
    json template_json = R"({"message": "Hello ${/user/name}"})"_json;
    json context = R"({"user": {"name": "Alice"}})"_json;
    
    permuto::Options opts;
    opts.enableStringInterpolation = true;
    
    auto result = permuto::apply(template_json, context, opts);
    EXPECT_EQ(result["message"], "Hello Alice");
}

// --- Test Suite for Recursion Depth Limits ---

TEST(PermutoPublicApiTests, RecursionDepthLimit) {
    json template_json = R"({"value": "${recursive.value}"})"_json;
    
    // Create a context with deep recursion
    json context = json::object();
    json current = context;
    for (int i = 0; i < 100; ++i) {
        current["recursive"] = json::object();
        current = current["recursive"];
    }
    current["value"] = "final_value";
    
    permuto::Options opts;
    opts.maxRecursionDepth = 10;
    opts.enableStringInterpolation = false;
    
    EXPECT_THROW(permuto::apply(template_json, context, opts), permuto::PermutoRecursionDepthException);
}

TEST(PermutoPublicApiTests, RecursionDepthLimitZero) {
    json template_json = R"({"value": "${value}"})"_json;
    json context = R"({"value": "${value}"})"_json;
    
    permuto::Options opts;
    opts.maxRecursionDepth = 0;
    opts.enableStringInterpolation = false;
    
    EXPECT_THROW(permuto::apply(template_json, context, opts), permuto::PermutoRecursionDepthException);
}

TEST(PermutoPublicApiTests, RecursionDepthLimitOne) {
    json template_json = R"({"value": "${value}"})"_json;
    json context = R"({"value": "${value}"})"_json;
    
    permuto::Options opts;
    opts.maxRecursionDepth = 1;
    opts.enableStringInterpolation = false;
    
    EXPECT_THROW(permuto::apply(template_json, context, opts), permuto::PermutoRecursionDepthException);
}

TEST(PermutoPublicApiTests, RecursionDepthLimitSixSuccess) {
    json template_json = R"({"value": "${level1.value}"})"_json;
    json context = R"({
        "level1": {"value": "${level2.value}"},
        "level2": {"value": "${level3.value}"},
        "level3": {"value": "${level4.value}"},
        "level4": {"value": "${level5.value}"},
        "level5": {"value": "final_value"}
    })"_json;
    
    permuto::Options opts;
    opts.maxRecursionDepth = 6;
    opts.enableStringInterpolation = false;
    
    auto result = permuto::apply(template_json, context, opts);
    EXPECT_EQ(result["value"], "final_value");
}

TEST(PermutoPublicApiTests, RecursionDepthLimitSixFailure) {
    json template_json = R"({"value": "${level1.value}"})"_json;
    json context = R"({
        "level1": {"value": "${level2.value}"},
        "level2": {"value": "${level3.value}"},
        "level3": {"value": "${level4.value}"},
        "level4": {"value": "${level5.value}"},
        "level5": {"value": "${level6.value}"},
        "level6": {"value": "final_value"}
    })"_json;
    
    permuto::Options opts;
    opts.maxRecursionDepth = 6;
    opts.enableStringInterpolation = false;
    
    EXPECT_THROW(permuto::apply(template_json, context, opts), permuto::PermutoRecursionDepthException);
}

TEST(PermutoPublicApiTests, RecursionDepthLimitTenSuccess) {
    json template_json = R"({"value": "${level1.value}"})"_json;
    json context = R"({
        "level1": {"value": "${level2.value}"},
        "level2": {"value": "${level3.value}"},
        "level3": {"value": "${level4.value}"},
        "level4": {"value": "${level5.value}"},
        "level5": {"value": "${level6.value}"},
        "level6": {"value": "${level7.value}"},
        "level7": {"value": "${level8.value}"},
        "level8": {"value": "${level9.value}"},
        "level9": {"value": "final_value"}
    })"_json;
    
    permuto::Options opts;
    opts.maxRecursionDepth = 10;
    opts.enableStringInterpolation = false;
    
    auto result = permuto::apply(template_json, context, opts);
    EXPECT_EQ(result["value"], "final_value");
}

TEST(PermutoPublicApiTests, RecursionDepthLimitTenFailure) {
    json template_json = R"({"value": "${level1.value}"})"_json;
    json context = R"({
        "level1": {"value": "${level2.value}"},
        "level2": {"value": "${level3.value}"},
        "level3": {"value": "${level4.value}"},
        "level4": {"value": "${level5.value}"},
        "level5": {"value": "${level6.value}"},
        "level6": {"value": "${level7.value}"},
        "level7": {"value": "${level8.value}"},
        "level8": {"value": "${level9.value}"},
        "level9": {"value": "${level10.value}"},
        "level10": {"value": "final_value"}
    })"_json;
    
    permuto::Options opts;
    opts.maxRecursionDepth = 10;
    opts.enableStringInterpolation = false;
    
    EXPECT_THROW(permuto::apply(template_json, context, opts), permuto::PermutoRecursionDepthException);
}

TEST(PermutoPublicApiTests, RecursionDepthLimitNotReached) {
    json template_json = R"({"value": "${user.name}"})"_json;
    json context = R"({"user": {"name": "Alice"}})"_json;
    
    permuto::Options opts;
    opts.maxRecursionDepth = 5;
    opts.enableStringInterpolation = false;
    
    auto result = permuto::apply(template_json, context, opts);
    EXPECT_EQ(result["value"], "Alice");
} 