#include "reverse_processor.hpp"
#include "json_pointer.hpp"
#include <sstream>

namespace permuto {
    ReverseProcessor::ReverseProcessor(const Options& options) 
        : options_(options), parser_(options.start_marker, options.end_marker) {
        options_.validate();
        
        // Reverse operations only work without interpolation
        if (options_.enable_interpolation) {
            throw std::invalid_argument("Reverse operations require interpolation to be disabled");
        }
    }
    
    nlohmann::json ReverseProcessor::create_reverse_template(const nlohmann::json& template_json) const {
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
            std::string new_path = current_path + "/" + it.key();
            
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
        
        std::vector<std::string> tokens;
        std::stringstream ss(path.substr(1)); // Skip leading '/'
        std::string token;
        
        while (std::getline(ss, token, '/')) {
            // Unescape JSON Pointer tokens
            std::string unescaped;
            for (size_t i = 0; i < token.size(); ++i) {
                if (token[i] == '~' && i + 1 < token.size()) {
                    if (token[i + 1] == '0') {
                        unescaped += '~';
                        ++i;
                    } else if (token[i + 1] == '1') {
                        unescaped += '/';
                        ++i;
                    } else {
                        unescaped += token[i];
                    }
                } else {
                    unescaped += token[i];
                }
            }
            tokens.push_back(unescaped);
        }
        
        return tokens;
    }
}