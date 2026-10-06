#include "../src/reverse_processor.hpp"
#include "../src/template_processor.hpp"
#include <gtest/gtest.h>
#include <stdexcept>

using namespace permuto;

class ReverseProcessorTest : public ::testing::Test {
protected:
    Options default_options; // interpolation disabled by default

    nlohmann::json template_json = R"({
        "user_id": "${/user/id}",
        "name": "${/user/name}",
        "email": "${/user/email}",
        "settings": "${/preferences}"
    })"_json;

    nlohmann::json context = R"({
        "user": {
            "id": 123,
            "name": "Alice",
            "email": "alice@example.com"
        },
        "preferences": {
            "theme": "dark",
            "notifications": true
        }
    })"_json;

    nlohmann::json result = R"({
        "user_id": 123,
        "name": "Alice",
        "email": "alice@example.com",
        "settings": {
            "theme": "dark",
            "notifications": true
        }
    })"_json;
};

TEST_F(ReverseProcessorTest, CreateReverseTemplate) {
    ReverseProcessor processor(default_options);

    auto reverse_template = processor.create_reverse_template(template_json);

    EXPECT_EQ(reverse_template["/user_id"], "/user/id");
    EXPECT_EQ(reverse_template["/name"], "/user/name");
    EXPECT_EQ(reverse_template["/email"], "/user/email");
    EXPECT_EQ(reverse_template["/settings"], "/preferences");
}

TEST_F(ReverseProcessorTest, ApplyReverse) {
    ReverseProcessor processor(default_options);

    auto reverse_template = processor.create_reverse_template(template_json);
    auto reconstructed = processor.apply_reverse(reverse_template, result);

    EXPECT_EQ(reconstructed, context);
}

TEST_F(ReverseProcessorTest, RoundTripIntegrity) {
    ReverseProcessor processor(default_options);

    // Forward: template + context -> result
    TemplateProcessor forward_processor(default_options);
    auto forward_result = forward_processor.process(template_json, context);

    // Reverse: template + result -> context
    auto reverse_template = processor.create_reverse_template(template_json);
    auto reconstructed = processor.apply_reverse(reverse_template, forward_result);

    EXPECT_EQ(reconstructed, context);
}

TEST_F(ReverseProcessorTest, NestedTemplate) {
    nlohmann::json nested_template = R"({
        "user": {
            "profile": {
                "name": "${/user/name}",
                "id": "${/user/id}"
            }
        },
        "config": "${/preferences}"
    })"_json;

    ReverseProcessor processor(default_options);
    auto reverse_template = processor.create_reverse_template(nested_template);

    EXPECT_EQ(reverse_template["/user/profile/name"], "/user/name");
    EXPECT_EQ(reverse_template["/user/profile/id"], "/user/id");
    EXPECT_EQ(reverse_template["/config"], "/preferences");
}

TEST_F(ReverseProcessorTest, ArrayTemplate) {
    nlohmann::json array_template = R"([
        "${/user/name}",
        "${/user/id}",
        {"email": "${/user/email}"}
    ])"_json;

    ReverseProcessor processor(default_options);
    auto reverse_template = processor.create_reverse_template(array_template);

    EXPECT_EQ(reverse_template["/0"], "/user/name");
    EXPECT_EQ(reverse_template["/1"], "/user/id");
    EXPECT_EQ(reverse_template["/2/email"], "/user/email");
}

TEST_F(ReverseProcessorTest, InterpolationDisallowed) {
    Options interpolation_opts;
    interpolation_opts.enable_interpolation = true;

    try {
        ReverseProcessor processor(interpolation_opts);
        FAIL() << "Expected std::invalid_argument but no exception was thrown";
    } catch (const std::invalid_argument& e) {
        SUCCEED();
    } catch (const std::exception& e) {
        FAIL() << "Expected std::invalid_argument but got: " << e.what();
    }
}

TEST_F(ReverseProcessorTest, NonPlaceholderElements) {
    nlohmann::json mixed_template = R"({
        "placeholder": "${/user/name}",
        "literal": "constant_value",
        "number": 42,
        "boolean": true,
        "null_value": null
    })"_json;

    ReverseProcessor processor(default_options);
    auto reverse_template = processor.create_reverse_template(mixed_template);

    // Only placeholder should be in reverse template
    EXPECT_EQ(reverse_template.size(), 1);
    EXPECT_EQ(reverse_template["/placeholder"], "/user/name");
}

TEST_F(ReverseProcessorTest, EmptyTemplate) {
    nlohmann::json empty_template = nlohmann::json::object();

    ReverseProcessor processor(default_options);
    auto reverse_template = processor.create_reverse_template(empty_template);

    EXPECT_TRUE(reverse_template.empty());
}

TEST_F(ReverseProcessorTest, MissingDataInResult) {
    nlohmann::json incomplete_result = R"({
        "user_id": 123,
        "name": "Alice"
    })"_json;

    ReverseProcessor processor(default_options);
    auto reverse_template = processor.create_reverse_template(template_json);
    auto reconstructed = processor.apply_reverse(reverse_template, incomplete_result);

    // Should only reconstruct available data
    EXPECT_EQ(reconstructed["user"]["id"], 123);
    EXPECT_EQ(reconstructed["user"]["name"], "Alice");
    EXPECT_FALSE(reconstructed["user"].contains("email"));
    EXPECT_FALSE(reconstructed.contains("preferences"));
}

// A reverse template names each value by its path in the RESULT document. That path is
// a JSON Pointer, and RFC 6901 (which REQUIREMENTS.md FR-3.1 makes the only supported
// path syntax — "/user~1role" is the documented form "for keys with slashes") escapes
// '~' as "~0" and '/' as "~1". A template KEY becomes a token in that pointer, so a key
// holding either character must be escaped when the pointer is composed; composed raw,
// "/a/b" addresses nested members a -> b, finds nothing, and the value is silently
// dropped from the reconstruction.
//
// Found by the fuzz harness (fuzz/fuzz_permuto.cpp) within ten seconds of its first
// campaign, 2026-09-20 — the round-trip oracle fired on an input mutated from this
// repo's own README example. See INCIDENTS.md.
TEST_F(ReverseProcessorTest, ResultPathEscapesSlashInKey) {
    nlohmann::json templ = R"({"a/b": "${/x}"})"_json;
    nlohmann::json ctx = R"({"x": 1})"_json;

    ReverseProcessor processor(default_options);
    TemplateProcessor forward_processor(default_options);
    auto forward_result = forward_processor.process(templ, ctx);

    auto reverse_template = processor.create_reverse_template(templ);
    EXPECT_EQ(reverse_template["/a~1b"], "/x");

    auto reconstructed = processor.apply_reverse(reverse_template, forward_result);
    EXPECT_EQ(reconstructed, ctx);
}

TEST_F(ReverseProcessorTest, ResultPathEscapesTildeInKey) {
    nlohmann::json templ = R"({"a~b": "${/x}"})"_json;
    nlohmann::json ctx = R"({"x": 1})"_json;

    ReverseProcessor processor(default_options);
    TemplateProcessor forward_processor(default_options);
    auto forward_result = forward_processor.process(templ, ctx);

    auto reverse_template = processor.create_reverse_template(templ);
    EXPECT_EQ(reverse_template["/a~0b"], "/x");

    auto reconstructed = processor.apply_reverse(reverse_template, forward_result);
    EXPECT_EQ(reconstructed, ctx);
}

// Both escapes, nested, so the fix cannot be "escape only the first character".
TEST_F(ReverseProcessorTest, ResultPathEscapesNestedKeyHoldingBoth) {
    nlohmann::json templ = R"({"outer": {"k/1~t": "${/y}"}})"_json;
    nlohmann::json ctx = R"({"y": 7})"_json;

    ReverseProcessor processor(default_options);
    TemplateProcessor forward_processor(default_options);
    auto forward_result = forward_processor.process(templ, ctx);

    auto reverse_template = processor.create_reverse_template(templ);
    EXPECT_EQ(reverse_template["/outer/k~11~0t"], "/y");

    auto reconstructed = processor.apply_reverse(reverse_template, forward_result);
    EXPECT_EQ(reconstructed, ctx);
}

// The reverse path of a template member whose KEY IS EMPTY is "/", and RFC 6901 makes
// that the empty-key member, not the root. ReverseProcessor kept its own copy of
// JsonPointer's getline tokenizer, so both directions were wrong: get_at_path() read the
// whole result document, and set_at_path() wrote nothing at all — the value simply
// vanished from the reconstruction. Second finding of the same harness run; see
// INCIDENTS.md.
TEST_F(ReverseProcessorTest, RoundTripEmptyTemplateKey) {
    nlohmann::json templ = R"({"": "${/x}"})"_json;
    nlohmann::json ctx = R"({"x": 1})"_json;

    ReverseProcessor processor(default_options);
    TemplateProcessor forward_processor(default_options);
    auto forward_result = forward_processor.process(templ, ctx);
    EXPECT_EQ(forward_result, R"({"": 1})"_json);

    auto reverse_template = processor.create_reverse_template(templ);
    EXPECT_EQ(reverse_template["/"], "/x");

    auto reconstructed = processor.apply_reverse(reverse_template, forward_result);
    EXPECT_EQ(reconstructed, ctx);
}

// The context side of the same rule: "${/}" names the member whose key is empty, and the
// reverse template has to write the value back there instead of dropping it.
TEST_F(ReverseProcessorTest, RoundTripEmptyContextKey) {
    nlohmann::json templ = R"({"out": "${/}"})"_json;
    nlohmann::json ctx = R"({"": 5})"_json;

    ReverseProcessor processor(default_options);
    TemplateProcessor forward_processor(default_options);
    auto forward_result = forward_processor.process(templ, ctx);
    EXPECT_EQ(forward_result, R"({"out": 5})"_json);

    auto reverse_template = processor.create_reverse_template(templ);
    auto reconstructed = processor.apply_reverse(reverse_template, forward_result);
    EXPECT_EQ(reconstructed, ctx);
}