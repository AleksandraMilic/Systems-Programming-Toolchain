#include "../../inc/Linker/Linker.hpp"
#include <iostream>
#include <vector>
#include <string>

int main(int argc, char** argv) {
    try {
        // Check if at least one input file is provided
        if (argc < 2) {
            std::cerr << "Usage: " << argv[0] << " [-o <output_file>] [-place=<section>@<address>] [-hex] <input_files>..." << std::endl;
            return 1;
        }

        // Create a Linker instance
        Linker linker;

        linker.processCommandLineArgs(argc, argv);
        linker.linking();

        // If no exceptions are thrown, print success message
        std::cout << "Linking completed successfully!" << std::endl;

    } catch (const std::exception& ex) {
        // Print any errors encountered
        std::cerr << "Error: " << ex.what() << std::endl;
        return 1;
    }

    return 0;
}