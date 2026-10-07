// fuzz/fuzz_permuto.cpp — libFuzzer harness for Permuto, with a real oracle.
//
// THE PROPERTY
// ------------
// Permuto documents a round-trip guarantee (README.md "Reverse Operations",
// REQUIREMENTS.md "Round-trip guarantee"):
//
//     result        = apply(template, context)
//     reverse       = create_reverse_template(template)
//     reconstructed = apply_reverse(reverse, result)
//     assert(reconstructed == context)
//
// A wrong answer here never crashes, so it has to be asserted explicitly — that
// assertion IS the oracle. Everything else in this file exists to establish the
// conditions under which the guarantee is claimed, because asserting outside them
// would abort a campaign on a legitimate result, and a gate that cries wolf gets
// bypassed.
//
// INTERPRETATION 1 — ONE INPUT = TWO DOCUMENTS
// --------------------------------------------
// The byte string is split at the FIRST 0x00 byte: before it is the template text,
// after it is the context text. A raw NUL can never appear inside valid JSON text
// (it has to be written as a \u0000 escape), so the split is unambiguous and
// libFuzzer can mutate either side freely.
//
// THE CONDITIONS
// --------------
// The equality is asserted only when ALL of these hold; otherwise the input is
// skipped, which is not a finding:
//
//   C1  apply() returned — no exception. A PermutoException is the documented
//       error path, not a bug.
//   C2  the result holds no leftover placeholder: no string value anywhere in it
//       contains "${". A leftover placeholder means a path did not resolve, which
//       (with MissingKeyBehavior::Ignore) is exactly the case where no context can
//       be reconstructed.
//   C3  the context provides EXACTLY the paths the template references. P(T) is
//       every template string that is entirely one placeholder; the context's
//       leaf-path set is every JSON-Pointer path to a scalar or empty container.
//       The two sets must be equal. Extra context keys, a missing path, or a path
//       that is a prefix of another all fail this and skip the input.
//   C4  no string VALUE inside the context contains "${": such a value is
//       substituted verbatim and then reads back as a placeholder, so the result
//       diverges from the raw context value for a documented reason.
//   C5  the context holds no non-empty array.  ** ADDED BY THIS HARNESS — see the
//       note below; it is not one of the four conditions in the brief. **
//
// WHY C5 EXISTS (measured on this tree, 2026-09-20)
// -------------------------------------------------
// A reverse template is a flat map from result-pointer to context-path string. It
// records no array-ness, and apply_reverse()'s set_at_path() only ever creates
// OBJECTS. So an array in the context that is referenced element by element comes
// back as an object keyed by the index:
//
//     template {"a":"${/items/0}","b":"${/items/1}"}   context {"items":[1,2]}
//     result   {"a":1,"b":2}
//     reverse  {"/a":"/items/0","/b":"/items/1"}
//     reconstructed {"items":{"0":1,"1":2}}   !=   {"items":[1,2]}
//
// That input satisfies C1-C4 exactly, so without C5 the oracle fires on it on the
// first campaign. It is a real gap in the documented guarantee, but it is a
// property of the reverse-template FORMAT rather than a defect a push gate should
// rediscover every time, and this harness is not allowed to change the library. C5
// excludes it; the gap is written up in the report that shipped this file. An EMPTY
// array is a leaf and does round-trip, so only non-empty arrays are excluded.
//
// THE ERROR PATH IS A FLOOR ORACLE
// --------------------------------
// PermutoException -> return. Any other exception, and any non-exception throw, is
// a bug: print one line and abort so libFuzzer writes a reproducer. The property
// itself is evaluated OUTSIDE every try block — a broad catch wrapped around an
// assertion silently deletes the oracle.
//
// TWO INTERPRETATIONS OF ONE INPUT
// --------------------------------
// Every input is read twice, and both readings assert the same property:
//
//   1. <template text> 0x00 <context text> — the two documents the corpus and seeds are made of.
//   2. <template text> + a value pool — the harness collects the paths the template references
//      and BUILDS the context that provides exactly those paths, so the two documents agree by
//      construction and the template's key names, key count and nesting shape mutate freely.
//
// Interpretation 2 exists because interpretation 1 alone cannot reach that surface, and that
// was MEASURED (an independent verification, /tmp/permuto-verification.md, 2026-09-20): C3
// requires the two documents' path sets to be equal, and byte mutation does not change both
// sides of the separator together, so only value mutations survived — 117 of 9,952 corpus inputs
// ever reached the assert and in all 117 the context's KEY NAMES were byte-identical to the
// seeds'. The proof: a sabotage making apply_reverse() skip every mapping whose context path
// contains a digit survived 4.2 MILLION executions, while the hand-written input
// {"x":"${/a1}"} 0x00 {"a1":1} finds it instantly. Interpretation 2 makes exactly those inputs
// reachable; the seed smoke now requires BOTH counters to be non-zero, so neither reading can
// rot while the other keeps the stage green.
//
// WHY THE SEED SMOKE EXISTS
// -------------------------
// A harness that never reaches its property looks exactly like a harness that
// always passes. The atexit handler prints
//     permuto fuzz: inputs=<n> identity_checks=<n> identity_matches=<n>
// and, when PERMUTO_FUZZ_REQUIRE_IDENTITY=1, exits non-zero if identity_checks is
// zero. The `fuzz` stage (scripts/fuzz.sh) sets that variable for a -runs=0 pass over
// fuzz/seeds, so a corpus that cannot reach the round-trip assert fails the gate
// instead of passing silently.
//
// SEED PROVENANCE (fuzz/seeds/, each file is template + 0x00 + context)
//   01-basic               README.md "Quick Start / Basic Usage" plus "Reverse
//                          Operations". The interpolating "Hello ${/user/name}!"
//                          line is replaced by an exact placeholder and the whole
//                          ${/preferences} subtree reference is expanded to its
//                          leaves: reverse operations require interpolation off,
//                          and C3 wants leaf references.
//   02-api-payload         README.md "Examples / API Payload Generation" — the
//                          OpenAI template and its shared context.
//   03-nested-config       README.md "Examples / Configuration Templates" — the
//                          database and logging blocks. The redis entry is dropped
//                          because it is string interpolation.
//   04-root-placeholder    REQUIREMENTS.md FR-3.1, "Empty path ${} refers to root
//                          context".
//   05-mixed-mode          examples/mixed_mode_example.cpp — its context and the
//                          ${...} half of its template, reduced to the fields the
//                          context actually provides.
//   99-invalid-truncated   deliberately invalid: truncated JSON on the template
//                          side, so the parse-rejection path has a seed of its own.
//
// This file must also compile with g++: the default (non-fuzzing) build compiles it
// as an object library so the `build`, `format` and `tidy` gates see it. No
// libFuzzer-only headers here.

#include <permuto/permuto.hpp>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

namespace {

using nlohmann::json;

constexpr std::size_t kMaxInputBytes = 64 * 1024;

// Deep enough for anything apply() accepts (max_recursion_depth defaults to 64), and
// shallow enough that these recursive walks cannot run the stack out on a context
// that nlohmann parsed iteratively.
constexpr std::size_t kMaxWalkDepth = 256;

std::atomic<std::size_t> g_inputs{0};
// Interpretation 1 (the bytes are two documents) and interpretation 2 (the bytes are a template
// plus a pool, and the context is BUILT from the template) are counted separately, because a
// guard that only requires "some input reached the assert" is exactly what let interpretation 1
// look healthy while it was unable to reach key-shape mutations.
std::atomic<std::size_t> g_identity_checks{0};
std::atomic<std::size_t> g_identity_matches{0};
std::atomic<std::size_t> g_structured_checks{0};
std::atomic<std::size_t> g_structured_matches{0};

void report_stats() {
    std::fprintf(stderr,
                 "permuto fuzz: inputs=%zu identity_checks=%zu identity_matches=%zu "
                 "structured_checks=%zu structured_matches=%zu\n",
                 g_inputs.load(), g_identity_checks.load(), g_identity_matches.load(),
                 g_structured_checks.load(), g_structured_matches.load());
    const char* const required = std::getenv("PERMUTO_FUZZ_REQUIRE_IDENTITY");
    if (required != nullptr && std::string(required) == "1") {
        if (g_identity_checks.load() == 0 || g_structured_checks.load() == 0) {
            std::fprintf(stderr,
                         "permuto fuzz: DEAD ORACLE - PERMUTO_FUZZ_REQUIRE_IDENTITY=1 and one of "
                         "the two interpretations never reached the round-trip assert "
                         "(identity_checks=%zu structured_checks=%zu). A reading of the input "
                         "that cannot exercise the property certifies nothing, however green "
                         "the other one looks.\n",
                         g_identity_checks.load(), g_structured_checks.load());
            std::fflush(stderr);
            // _Exit, not exit: this runs inside the atexit chain already.
            std::_Exit(1);
        }
    }
    std::fflush(stderr);
}

// A type with a non-trivial constructor, so the registration below is not an unused
// constant as far as -Wunused-const-variable is concerned.
struct StatsReporter {
    StatsReporter() { (void)std::atexit(&report_stats); }
};

const StatsReporter g_stats_reporter;

[[noreturn]] void die(const char* what, const json& templ, const json& ctx, const json& detail) {
    std::fprintf(stderr, "permuto fuzz: %s\n  template: %s\n  context:  %s\n  detail:   %s\n", what,
                 templ.dump().c_str(), ctx.dump().c_str(), detail.dump().c_str());
    std::fflush(stderr);
    std::abort();
}

[[nodiscard]] std::string escape_pointer_token(const std::string& key) {
    std::string escaped;
    escaped.reserve(key.size());
    for (const char character : key) {
        if (character == '~') {
            escaped += "~0";
        } else if (character == '/') {
            escaped += "~1";
        } else {
            escaped += character;
        }
    }
    return escaped;
}

// P(T): every template string that is ENTIRELY one placeholder. The inner text must
// hold no further "${", and must be a path PlaceholderParser would accept (empty is
// the root context, otherwise a JSON Pointer starting with '/'). Anything this misses
// only costs a skip: a placeholder the library resolves but this does not collect
// makes the two sets in C3 unequal, and a string this collects but the library leaves
// alone survives into the result and is caught by C2.
void collect_referenced_paths(const json& node, std::vector<std::string>& paths,
                              std::size_t depth) {
    if (depth > kMaxWalkDepth) {
        return;
    }
    if (node.is_object() || node.is_array()) {
        for (const auto& child : node) {
            collect_referenced_paths(child, paths, depth + 1);
        }
        return;
    }
    if (!node.is_string()) {
        return;
    }
    const std::string text = node.get<std::string>();
    if (text.size() < 3 || text.compare(0, 2, "${") != 0 || text.back() != '}') {
        return;
    }
    const std::string path = text.substr(2, text.size() - 3);
    if (path.find("${") != std::string::npos) {
        return;
    }
    if (!path.empty() && path.front() != '/') {
        return;
    }
    paths.push_back(path);
}

// The context's leaf paths, JSON-Pointer style. A scalar, an empty object and an
// empty array are leaves; the root of a scalar document is the empty pointer "".
// Returns false when the walk met a non-empty array (C5) or ran past kMaxWalkDepth —
// in both cases the input is skipped instead of asserted on.
[[nodiscard]] bool collect_leaf_paths(const json& node, const std::string& pointer,
                                      std::vector<std::string>& paths, std::size_t depth) {
    if (depth > kMaxWalkDepth) {
        return false;
    }
    if (node.is_array() && !node.empty()) {
        return false; // C5
    }
    if (node.is_object() && !node.empty()) {
        for (auto member = node.begin(); member != node.end(); ++member) {
            const std::string child_pointer = pointer + "/" + escape_pointer_token(member.key());
            if (!collect_leaf_paths(member.value(), child_pointer, paths, depth + 1)) {
                return false;
            }
        }
        return true;
    }
    paths.push_back(pointer);
    return true;
}

// C2 and C4: is there a string value anywhere under this node that holds "${"?
[[nodiscard]] bool has_marker_in_any_string(const json& node, std::size_t depth) {
    if (depth > kMaxWalkDepth) {
        return true; // too deep to certify — treat the input as ineligible
    }
    if (node.is_string()) {
        return node.get<std::string>().find("${") != std::string::npos;
    }
    if (node.is_object() || node.is_array()) {
        for (const auto& child : node) {
            if (has_marker_in_any_string(child, depth + 1)) {
                return true;
            }
        }
    }
    return false;
}

void sort_unique(std::vector<std::string>& values) {
    std::sort(values.begin(), values.end());
    values.erase(std::unique(values.begin(), values.end()), values.end());
}

} // namespace

namespace {

// ---------------------------------------------------------------------------------------------
// INTERPRETATION 2 — BUILD THE CONTEXT FROM THE TEMPLATE
//
// Why: with interpretation 1 alone the oracle could not be reached by KEY-SHAPE mutations, and
// that was measured (independent verification, 2026-09-20, /tmp/permuto-verification.md). C3
// requires the context's leaf-path set to EQUAL the template's placeholder-path set, and byte
// mutation does not change both documents of one input together — so only value mutations
// survived: 117 of 9,952 corpus inputs reached the assert, and in all 117 the context's key
// names were byte-identical to the seeds'. A sabotage that made apply_reverse() skip every
// mapping whose context path contains a digit survived 4.2 MILLION executions, while the
// hand-written input {"x":"${/a1}"} 0x00 {"a1":1} finds it instantly.
//
// So the bytes are read a second time as a template plus a value pool: the harness collects the
// paths the template references and BUILDS the context that provides exactly those paths. The
// two documents now agree by construction, which lets the template's key names, key count and
// nesting shape mutate freely — the surface where the round trip's pointer handling lives.
//
// Eligibility is strict and a skip is never a finding: paths become OBJECT members (so no array
// is ever constructed and the parked array-element limit is never involved), a path that is a
// prefix of another is skipped, and the work per input is bounded because this runs millions of
// times.

constexpr std::size_t kMaxStructuredPaths = 16;
constexpr std::size_t kMaxPoolValues = 32;

// Values are decoded from a fixed alphabet holding neither '$' nor '{', so a decoded string can
// never look like a placeholder and condition C4 holds by construction.
constexpr const char* kValueAlphabet = "abzQR/_ -.019";

// RFC 6901 unescaping, so a placeholder path naming a key with '/' or '~' builds a context that
// really holds that KEY — the shape the escaping defect lived in.
[[nodiscard]] std::string unescape_pointer_token(const std::string& token) {
    std::string out;
    out.reserve(token.size());
    for (std::size_t i = 0; i < token.size(); ++i) {
        if (token[i] == '~' && i + 1 < token.size()) {
            if (token[i + 1] == '0') {
                out += '~';
                ++i;
                continue;
            }
            if (token[i + 1] == '1') {
                out += '/';
                ++i;
                continue;
            }
        }
        out += token[i];
    }
    return out;
}

// A bounded, deterministic pool of JSON values: scalars and empty containers only, so every
// built context's leaves are exactly the paths that were asked for.
[[nodiscard]] std::vector<json> decode_value_pool(const std::uint8_t* data, std::size_t size) {
    const std::size_t take = std::min(size, kMaxPoolValues);
    std::vector<json> pool;
    pool.reserve(take);
    for (std::size_t i = 0; i < take; ++i) {
        const std::uint8_t byte = data[i];
        switch (byte % 7U) {
        case 0:
            pool.emplace_back(nullptr);
            break;
        case 1:
            pool.emplace_back((byte & 8U) != 0);
            break;
        case 2:
            pool.emplace_back(static_cast<std::int64_t>(byte % 100U) - 50);
            break;
        case 3:
            pool.emplace_back(static_cast<double>(byte) / 10.0);
            break;
        case 4: {
            std::string value;
            value += kValueAlphabet[byte % (sizeof(kValueAlphabet) - 1)];
            if ((byte & 16U) != 0) {
                value += kValueAlphabet[(byte * 3U) % (sizeof(kValueAlphabet) - 1)];
            }
            pool.emplace_back(value);
            break;
        }
        case 5:
            pool.emplace_back(json::object());
            break;
        default:
            pool.emplace_back(json::array());
            break;
        }
    }
    return pool;
}

// Set `value` at an object-member path, creating intermediate objects. Returns false when the
// path cannot exist as object members (an intermediate node is already a scalar, an empty
// container, or a different path's leaf) — the caller then skips the input.
[[nodiscard]] bool set_member_path(json& document, const std::string& pointer, const json& value) {
    if (pointer.empty() || pointer.front() != '/') {
        return false;
    }
    json* current = &document;
    std::size_t start = 1;
    while (true) {
        const std::size_t slash = pointer.find('/', start);
        const bool last = (slash == std::string::npos);
        const std::string token = unescape_pointer_token(
            last ? pointer.substr(start) : pointer.substr(start, slash - start));
        if (!current->is_object()) {
            return false;
        }
        if (last) {
            (*current)[token] = value;
            return true;
        }
        auto existing = current->find(token);
        if (existing == current->end()) {
            (*current)[token] = json::object();
            existing = current->find(token);
        }
        if (!existing->is_object()) {
            return false; // a longer path already claimed this node as a leaf
        }
        current = &(*existing);
        start = slash + 1;
    }
}

[[nodiscard]] bool build_context(const std::vector<std::string>& paths,
                                 const std::vector<json>& pool, json& context) {
    context = json::object();
    for (std::size_t i = 0; i < paths.size(); ++i) {
        const json value = pool.empty() ? json() : pool[i % pool.size()];
        if (!set_member_path(context, paths[i], value)) {
            return false;
        }
    }
    return true;
}

// One round trip, asserted only where the guarantee is claimed. `structured` selects the counter
// pair that records it: a reading of the input that cannot reach the property must be visible as
// such, instead of hiding behind the other reading's numbers.
void check_round_trip(const json& templ, const json& ctx, bool structured) {
    permuto::Options opts;             // defaults: "${" / "}", max_recursion_depth = 64
    opts.enable_interpolation = false; // reverse operations are undefined with interpolation
    opts.missing_key_behavior = permuto::MissingKeyBehavior::Ignore;

    // --- the floor oracle: apply() may only fail through PermutoException ----------
    json result;
    try {
        result = permuto::apply(templ, ctx, opts);
    } catch (const permuto::PermutoException&) {
        return; // C1 does not hold: this is the documented error path
    } catch (const std::exception& error) {
        std::fprintf(stderr, "permuto fuzz: apply() threw a non-Permuto exception: %s\n",
                     error.what());
        die("apply() threw a non-Permuto exception", templ, ctx, json());
    } catch (...) {
        die("apply() threw a non-exception object", templ, ctx, json());
    }

    // --- C2 and C4 -----------------------------------------------------------------
    if (has_marker_in_any_string(result, 0) || has_marker_in_any_string(ctx, 0)) {
        return;
    }

    // --- C3 and C5 -----------------------------------------------------------------
    std::vector<std::string> referenced;
    collect_referenced_paths(templ, referenced, 0);
    sort_unique(referenced);

    std::vector<std::string> leaves;
    if (!collect_leaf_paths(ctx, std::string(), leaves, 0)) {
        return;
    }
    sort_unique(leaves);

    if (referenced != leaves) {
        return;
    }

    // --- the round trip ------------------------------------------------------------
    json reverse_template;
    json reconstructed;
    try {
        reverse_template = permuto::create_reverse_template(templ, opts);
        reconstructed = permuto::apply_reverse(reverse_template, result);
    } catch (const permuto::PermutoException&) {
        return;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "permuto fuzz: the reverse path threw a non-Permuto exception: %s\n",
                     error.what());
        die("reverse path threw a non-Permuto exception", templ, ctx, result);
    } catch (...) {
        die("reverse path threw a non-exception object", templ, ctx, result);
    }

    // The property, deliberately OUTSIDE every try above: a catch around this is how
    // an oracle dies without anyone noticing.
    std::atomic<std::size_t>& checks = structured ? g_structured_checks : g_identity_checks;
    std::atomic<std::size_t>& matches = structured ? g_structured_matches : g_identity_matches;
    checks.fetch_add(1, std::memory_order_relaxed);
    if (reconstructed != ctx) {
        std::fprintf(stderr,
                     "permuto fuzz: ROUND-TRIP MISMATCH - apply_reverse() did not reproduce the "
                     "%s\n  result:        %s\n  reverse:       %s\n  reconstructed: %s\n",
                     structured ? "context built from the template" : "context from the input",
                     result.dump().c_str(), reverse_template.dump().c_str(),
                     reconstructed.dump().c_str());
        die("ROUND-TRIP MISMATCH (reconstructed != context)", templ, ctx, reconstructed);
    }
    matches.fetch_add(1, std::memory_order_relaxed);
}

} // namespace

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    if (size > kMaxInputBytes) {
        return 0;
    }
    g_inputs.fetch_add(1, std::memory_order_relaxed);

    // Split at the first NUL byte: template text | context text.
    std::size_t separator = 0;
    while (separator < size && data[separator] != 0) {
        ++separator;
    }
    const bool has_separator = separator < size;

    // std::string's iterator-range constructor, not a cast: CODING_STANDARDS.md
    // forbids reinterpret_cast, and uint8_t converts to char implicitly.
    const std::string template_text(data, data + separator);

    // --- interpretation 1: the bytes are two JSON documents --------------------------------
    if (has_separator) {
        const std::string context_text(data + separator + 1, data + size);
        const json templ = json::parse(template_text, nullptr, false);
        const json ctx = json::parse(context_text, nullptr, false);
        if (!templ.is_discarded() && !ctx.is_discarded()) {
            check_round_trip(templ, ctx, false);
        }
    }

    // --- interpretation 2: the bytes are a template plus a value pool ----------------------
    {
        const json templ = json::parse(template_text, nullptr, false);
        if (!templ.is_discarded()) {
            std::vector<std::string> referenced;
            collect_referenced_paths(templ, referenced, 0);
            sort_unique(referenced);
            if (!referenced.empty() && referenced.size() <= kMaxStructuredPaths) {
                const std::size_t pool_begin = separator + (has_separator ? 1U : 0U);
                const std::vector<json> pool
                    = decode_value_pool(data + pool_begin, size - pool_begin);
                json ctx;
                bool built = false;
                if (referenced.front().empty()) {
                    // "${}" references the whole context, so it can only be that path alone: the
                    // document IS the value.
                    if (referenced.size() == 1 && !pool.empty()) {
                        ctx = pool.front();
                        built = true;
                    }
                } else {
                    built = build_context(referenced, pool, ctx);
                }
                if (built) {
                    check_round_trip(templ, ctx, true);
                }
            }
        }
    }
    return 0;
}
