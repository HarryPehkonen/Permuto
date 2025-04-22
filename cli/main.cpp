// permuto/cli/main.cpp
#include <permuto/permuto.hpp>
#include <permuto/exceptions.hpp>
#include <nlohmann/json.hpp>

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <stdexcept>
#include <cstdlib> // For EXIT_SUCCESS, EXIT_FAILURE

// Basic function to print usage help
void print_usage(const char* prog_name) {
    std::cerr << "Usage: " << prog_name << " <template_file> <context_file> [options]\n\n"
              << "Options:\n"
              << "  --on-missing-key=<value>  Behavior for missing keys ('ignore' or 'error'. Default: ignore)\n"
              << "  --start=<string>          Variable start delimiter (Default: ${)\n"
              << "  --end=<string>            Variable end delimiter (Default: })\n"
              << "  --help                    Show this help message\n"
              << "  --string-interpolation    Enable string interpolation\n"
    ;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    std::string template_filename;
    std::string context_filename;
    permuto::Options options;
    std::vector<std::string> positional_args;

    // --- Manual Argument Parsing ---
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        const std::string START_PREFIX = "--start=";
        const std::string END_PREFIX = "--end=";

        if (arg == "--help") {
            print_usage(argv[0]);
            return EXIT_SUCCESS;
        } else if (arg.rfind("--on-missing-key=", 0) == 0) {
            std::string value = arg.substr(17);
            if (value == "ignore") {
                options.onMissingKey = permuto::MissingKeyBehavior::Ignore;
            } else if (value == "error") {
                options.onMissingKey = permuto::MissingKeyBehavior::Error;
            } else {
                std::cerr << "Error: Invalid value for --on-missing-key: " << value << ". Use 'ignore' or 'error'.\n";
                return EXIT_FAILURE;
            }
        } else if (arg.rfind(START_PREFIX, 0) == 0) {
            options.variableStartMarker = arg.substr(START_PREFIX.length());
        } else if (arg.rfind(END_PREFIX, 0) == 0) {
            options.variableEndMarker = arg.substr(END_PREFIX.length());
        } else if (arg == "--string-interpolation") {
            options.enableStringInterpolation = true;
        } else if (arg.rfind("--", 0) == 0) {
            std::cerr << "Error: Unknown option: " << arg << "\n";
            print_usage(argv[0]);
            return EXIT_FAILURE;
        } else {
            positional_args.push_back(arg);
        }
    }

    if (positional_args.size() != 2) {
        std::cerr << "Error: Expected exactly two positional arguments: <template_file> <context_file>\n";
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }
    template_filename = positional_args[0];
    context_filename = positional_args[1];

    // --- Validate Options ---
    try {
        options.validate();
    } catch (const std::invalid_argument& e) {
        std::cerr << "Error: Invalid options: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }


    // --- Read Files ---
    nlohmann::json template_json;
    nlohmann::json context_json;

    try {
        std::ifstream template_file(template_filename);
        if (!template_file.is_open()) {
            throw std::runtime_error("Could not open template file: " + template_filename);
        }
        template_json = nlohmann::json::parse(template_file, nullptr, true, true); // Allow exceptions, allow comments
    } catch (const std::exception& e) {
        std::cerr << "Error reading or parsing template file '" << template_filename << "': " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    try {
        std::ifstream context_file(context_filename);
        if (!context_file.is_open()) {
            throw std::runtime_error("Could not open context file: " + context_filename);
        }
        context_json = nlohmann::json::parse(context_file, nullptr, true, true); // Allow exceptions, allow comments
    } catch (const std::exception& e) {
        std::cerr << "Error reading or parsing context file '" << context_filename << "': " << e.what() << std::endl;
        return EXIT_FAILURE;
    }


    // --- Process Template ---
    try {
        nlohmann::json result_json = permuto::apply(template_json, context_json, options);

        // --- Output Result (Pretty-printed) ---
        std::cout << result_json.dump(4) << std::endl; // Use dump(4) for pretty printing

    } catch (const permuto::PermutoException& e) {
        std::cerr << "Permuto Error: " << e.what() << std::endl;
        // Specific handling if needed:
        // if (auto cycle_ex = dynamic_cast<const permuto::PermutoCycleException*>(&e)) {
        //     std::cerr << "  Cycle detected: " << cycle_ex->get_cycle_path() << std::endl;
        // } else if (auto missing_ex = dynamic_cast<const permuto::PermutoMissingKeyException*>(&e)) {
        //     std::cerr << "  Missing key/path: " << missing_ex->get_key_path() << std::endl;
        // }
        return EXIT_FAILURE;
    } catch (const std::exception& e) {
        std::cerr << "An unexpected error occurred: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
