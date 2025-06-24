#include <iostream>
#include <fstream>
#include <permuto/permuto.hpp>

namespace {
    // Command line option constants
    const std::string HELP_OPTION = "--help";
    const std::string VERSION_OPTION = "--version";
    const std::string REVERSE_OPTION = "--reverse";
    const std::string INTERPOLATION_OPTION = "--interpolation";
    const std::string NO_INTERPOLATION_OPTION = "--no-interpolation";
    const std::string MISSING_KEY_OPTION = "--missing-key=";
    const std::string START_MARKER_OPTION = "--start=";
    const std::string END_MARKER_OPTION = "--end=";
    const std::string MAX_DEPTH_OPTION = "--max-depth=";
    
    // Missing key behavior values
    const std::string IGNORE_VALUE = "ignore";
    const std::string ERROR_VALUE = "error";
    const std::string REMOVE_VALUE = "remove";
    
    // Program constants
    const int MIN_ARGC = 2;
    const int FIRST_ARG_INDEX = 1;
    const size_t REQUIRED_FILE_COUNT = 2;
    const size_t FIRST_FILE_INDEX = 0;
    const size_t SECOND_FILE_INDEX = 1;
    const int JSON_INDENT = 2;
    const char OPTION_PREFIX = '-';
    
    // Exit codes
    const int EXIT_SUCCESS_CODE = 0;
    const int EXIT_ERROR_CODE = 1;
}

void print_usage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [OPTIONS] <template.json> <context.json>\n";
    std::cout << "       " << program_name << " --reverse [OPTIONS] <template.json> <result.json>\n";
    std::cout << "\nOptions:\n";
    std::cout << "  --help                Show this help message\n";
    std::cout << "  --version             Show version information\n";
    std::cout << "  --reverse             Perform reverse operation\n";
    std::cout << "  --interpolation       Enable string interpolation (default: off)\n";
    std::cout << "  --no-interpolation    Disable string interpolation\n";
    std::cout << "  --missing-key=MODE    Set missing key behavior (ignore|error|remove)\n";
    std::cout << "  --start=MARKER        Set start marker (default: ${)\n";
    std::cout << "  --end=MARKER          Set end marker (default: })\n";
    std::cout << "  --max-depth=N         Set max recursion depth (default: 64)\n";
}

void print_version() {
    std::cout << "Permuto CLI v1.0.0\n";
    std::cout << "JSON template processing tool\n";
}

nlohmann::json load_json_file(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + filename);
    }
    
    nlohmann::json json;
    file >> json;
    return json;
}

int main(int argc, char* argv[]) {
    try {
        if (argc < MIN_ARGC) {
            print_usage(argv[0]);
            return EXIT_ERROR_CODE;
        }
        
        // Parse command line arguments
        permuto::Options options;
        bool reverse_mode = false;
        std::vector<std::string> files;
        
        for (int i = FIRST_ARG_INDEX; i < argc; ++i) {
            std::string arg = argv[i];
            
            if (arg == HELP_OPTION) {
                print_usage(argv[0]);
                return EXIT_SUCCESS_CODE;
            } else if (arg == VERSION_OPTION) {
                print_version();
                return EXIT_SUCCESS_CODE;
            } else if (arg == REVERSE_OPTION) {
                reverse_mode = true;
            } else if (arg == INTERPOLATION_OPTION) {
                options.enable_interpolation = true;
            } else if (arg == NO_INTERPOLATION_OPTION) {
                options.enable_interpolation = false;
            } else if (arg.substr(0, MISSING_KEY_OPTION.length()) == MISSING_KEY_OPTION) {
                std::string mode = arg.substr(MISSING_KEY_OPTION.length());
                if (mode == IGNORE_VALUE) {
                    options.missing_key_behavior = permuto::MissingKeyBehavior::Ignore;
                } else if (mode == ERROR_VALUE) {
                    options.missing_key_behavior = permuto::MissingKeyBehavior::Error;
                } else if (mode == REMOVE_VALUE) {
                    options.missing_key_behavior = permuto::MissingKeyBehavior::Remove;
                } else {
                    std::cerr << "Invalid missing key mode: " << mode << std::endl;
                    return EXIT_ERROR_CODE;
                }
            } else if (arg.substr(0, START_MARKER_OPTION.length()) == START_MARKER_OPTION) {
                options.start_marker = arg.substr(START_MARKER_OPTION.length());
            } else if (arg.substr(0, END_MARKER_OPTION.length()) == END_MARKER_OPTION) {
                options.end_marker = arg.substr(END_MARKER_OPTION.length());
            } else if (arg.substr(0, MAX_DEPTH_OPTION.length()) == MAX_DEPTH_OPTION) {
                try {
                    options.max_recursion_depth = std::stoull(arg.substr(MAX_DEPTH_OPTION.length()));
                } catch (const std::exception&) {
                    std::cerr << "Invalid max depth value: " << arg.substr(MAX_DEPTH_OPTION.length()) << std::endl;
                    return EXIT_ERROR_CODE;
                }
            } else if (arg[0] != OPTION_PREFIX) {
                files.push_back(arg);
            } else {
                std::cerr << "Unknown option: " << arg << std::endl;
                return EXIT_ERROR_CODE;
            }
        }
        
        if (files.size() != REQUIRED_FILE_COUNT) {
            std::cerr << "Error: Exactly " << REQUIRED_FILE_COUNT << " files required\n";
            print_usage(argv[0]);
            return EXIT_ERROR_CODE;
        }
        
        // Validate options
        options.validate();
        
        // Load files
        auto file1 = load_json_file(files[FIRST_FILE_INDEX]);
        auto file2 = load_json_file(files[SECOND_FILE_INDEX]);
        
        nlohmann::json result;
        
        if (reverse_mode) {
            // Reverse operation: template + result -> context
            auto reverse_template = permuto::create_reverse_template(file1, options);
            result = permuto::apply_reverse(reverse_template, file2);
        } else {
            // Forward operation: template + context -> result
            result = permuto::apply(file1, file2, options);
        }
        
        // Pretty print result
        std::cout << result.dump(JSON_INDENT) << std::endl;
        
        return EXIT_SUCCESS_CODE;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_ERROR_CODE;
    }
}