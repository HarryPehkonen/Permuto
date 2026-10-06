#include "reverse_processor.hpp"
#include "json_pointer.hpp"

namespace permuto {

namespace {
// RFC 6901: inside a JSON Pointer token, '~' is escaped as "~0" and '/' as "~1" — in that
// order, in a single pass, so a key holding a literal "~1" does not come back as a '/'.
//
// A reverse template names each value by its path in the RESULT document, and an object
// member's key is one token of that pointer. Composed raw, a key holding '/' turns
// "/a/b" into a pointer that addresses members a -> b: the lookup in apply_reverse finds
// nothing and the value is dropped from the reconstruction, silently. REQUIREMENTS.md
// FR-3.1 makes the escaped form the only supported path syntax ("/user~1role" for keys
// with slashes), so this is the reverse direction of a rule the repo already states.
// Found by fuzz/fuzz_permuto.cpp's round-trip oracle, 2026-09-20; see INCIDENTS.md.
std::string escape_pointer_token(const std::string& token) {
    std::string escaped;
    escaped.reserve(token.size());
    for (const char character : token) {
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
} // namespace

ReverseProcessor::ReverseProcessor(const Options& options)
    : options_(options), parser_(options.start_marker, options.end_marker) {
    options_.validate();

    // Reverse operations only work without interpolation
    if (options_.enable_interpolation) {
        throw std::invalid_argument("Reverse operations require interpolation to be disabled");
    }
}

nlohmann::json
ReverseProcessor::create_reverse_template(const nlohmann::json& template_json) const {
    auto mappings = analyze_template(template_json);

    // Create reverse template as a JSON object mapping result paths to context paths
    nlohmann::json reverse_template = nlohmann::json::object();

    for (const auto& mapping : mappings) {
        reverse_template[mapping.result_path] = mapping.context_path;
    }

    return reverse_template;
}

nlohmann::json ReverseProcessor::apply_reverse(const nlohmann::json& reverse_template,
                                               const nlohmann::json& result_json) const {
    nlohmann::json context = nlohmann::json::object();

    // Process each mapping in the reverse template
    for (auto it = reverse_template.begin(); it != reverse_template.end(); ++it) {
        const std::string& result_path = it.key();
        const std::string& context_path = it.value().get<std::string>();

        // Get value from result at result_path
        auto result_value = get_at_path(result_json, result_path);
        if (result_value) {
            // Set value in context at context_path
            set_at_path(context, context_path, *result_value);
        }
    }

    return context;
}

std::vector<PathMapping> ReverseProcessor::analyze_template(const nlohmann::json& template_json,
                                                            const std::string& current_path) const {
    std::vector<PathMapping> mappings;

    if (template_json.is_object()) {
        analyze_object(template_json, current_path, mappings);
    } else if (template_json.is_array()) {
        analyze_array(template_json, current_path, mappings);
    } else if (template_json.is_string()) {
        analyze_string(template_json.get<std::string>(), current_path, mappings);
    }
    // Primitives (numbers, booleans, null) don't contain placeholders

    return mappings;
}

void ReverseProcessor::analyze_object(const nlohmann::json& obj, const std::string& current_path,
                                      std::vector<PathMapping>& mappings) const {
    for (auto it = obj.begin(); it != obj.end(); ++it) {
        std::string new_path = current_path + "/" + escape_pointer_token(it.key());

        auto sub_mappings = analyze_template(it.value(), new_path);
        mappings.insert(mappings.end(), sub_mappings.begin(), sub_mappings.end());
    }
}

void ReverseProcessor::analyze_array(const nlohmann::json& arr, const std::string& current_path,
                                     std::vector<PathMapping>& mappings) const {
    for (size_t i = 0; i < arr.size(); ++i) {
        std::string new_path = current_path + "/" + std::to_string(i);

        auto sub_mappings = analyze_template(arr[i], new_path);
        mappings.insert(mappings.end(), sub_mappings.begin(), sub_mappings.end());
    }
}

void ReverseProcessor::analyze_string(const std::string& str, const std::string& current_path,
                                      std::vector<PathMapping>& mappings) const {
    // Only process exact-match placeholders (interpolation disabled)
    auto exact_path = parser_.extract_exact_placeholder(str);
    if (exact_path) {
        PathMapping mapping;
        mapping.context_path = *exact_path;
        mapping.result_path = current_path;
        mappings.push_back(mapping);
    }
}

void ReverseProcessor::set_at_path(nlohmann::json& target, const std::string& path,
                                   const nlohmann::json& value) const {
    if (path.empty()) {
        target = value;
        return;
    }

    auto tokens = path_to_tokens(path);
    nlohmann::json* current = &target;

    for (size_t i = 0; i < tokens.size(); ++i) {
        const std::string& token = tokens[i];
        bool is_last = (i == tokens.size() - 1);

        if (is_last) {
            // Set the final value
            if (current->is_null()) {
                *current = nlohmann::json::object();
            }
            (*current)[token] = value;
        } else {
            // Navigate or create intermediate objects
            if (current->is_null()) {
                *current = nlohmann::json::object();
            }
            if (!current->is_object()) {
                throw std::runtime_error("Cannot navigate through non-object");
            }
            if (current->find(token) == current->end()) {
                (*current)[token] = nlohmann::json::object();
            }
            current = &(*current)[token];
        }
    }
}

std::optional<nlohmann::json> ReverseProcessor::get_at_path(const nlohmann::json& source,
                                                            const std::string& path) const {
    try {
        JsonPointer pointer(path);
        return pointer.resolve(source);
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

std::vector<std::string> ReverseProcessor::path_to_tokens(const std::string& path) const {
    if (path.empty()) {
        return {};
    }

    if (path[0] != '/') {
        throw std::invalid_argument("Path must start with '/'");
    }

    // ONE tokenizer in this codebase. JsonPointer already splits a pointer and unescapes
    // its tokens ("~1" -> '/', "~0" -> '~'), and parse_path() is where the empty-token
    // rules live. This function used to carry a second, subtly different copy of that
    // logic — and the copy is what silently dropped an empty-key member on the context
    // side of a round trip (a second defect from the same harness run; see INCIDENTS.md).
    return JsonPointer(path).tokens();
}
} // namespace permuto