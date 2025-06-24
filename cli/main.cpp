#include <iostream>
#include <fstream>
#include <permuto/permuto.hpp>

void print_usage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [OPTIONS] <template.json> <context.json>\n";
    std::cout << "       " << program_name << " --reverse [OPTIONS] <template.json> <result.json>\n";
    std::cout << "\nOptions:\n";
    std::cout << "  --help                Show this help message\n";
    std::cout << "  --version             Show version information\n";
    std::cout << "  --reverse             Perform reverse operation\n";
    std::cout << "  --interpolation       Enable string interpolation (default: off)\n";
    std::cout << "  --no-interpolation    Disable string interpolation\n";
    std::cout << "  --missing-key=MODE    Set missing key behavior (ignore|error)\n";
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
        if (argc < 2) {
            print_usage(argv[0]);
            return 1;
        }
        
        // Parse command line arguments
        permuto::Options options;
        bool reverse_mode = false;
        std::vector<std::string> files;
        
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            
            if (arg == "--help") {
                print_usage(argv[0]);
                return 0;
            } else if (arg == "--version") {
                print_version();
                return 0;
            } else if (arg == "--reverse") {
                reverse_mode = true;
            } else if (arg == "--interpolation") {
                options.enable_interpolation = true;
            } else if (arg == "--no-interpolation") {
                options.enable_interpolation = false;
            } else if (arg.substr(0, 14) == "--missing-key=") {
                std::string mode = arg.substr(14);
                if (mode == "ignore") {
                    options.missing_key_behavior = permuto::MissingKeyBehavior::Ignore;
                } else if (mode == "error") {
                    options.missing_key_behavior = permuto::MissingKeyBehavior::Error;
                } else {
                    std::cerr << "Invalid missing key mode: " << mode << std::endl;
                    return 1;
                }
            } else if (arg.substr(0, 8) == "--start=") {
                options.start_marker = arg.substr(8);
            } else if (arg.substr(0, 6) == "--end=") {
                options.end_marker = arg.substr(6);
            } else if (arg.substr(0, 12) == "--max-depth=") {
                try {
                    options.max_recursion_depth = std::stoull(arg.substr(12));
                } catch (const std::exception&) {
                    std::cerr << "Invalid max depth value: " << arg.substr(12) << std::endl;
                    return 1;
                }
            } else if (arg[0] != '-') {
                files.push_back(arg);
            } else {
                std::cerr << "Unknown option: " << arg << std::endl;
                return 1;
            }
        }
        
        if (files.size() != 2) {
            std::cerr << "Error: Exactly 2 files required\n";
            print_usage(argv[0]);
            return 1;
        }
        
        // Validate options
        options.validate();
        
        // Load files
        auto file1 = load_json_file(files[0]);
        auto file2 = load_json_file(files[1]);
        
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
        std::cout << result.dump(2) << std::endl;
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}