#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <string>
#include "../../inc/Assembler/Assembler.hpp"

// Declare external variables and functions from the parser and lexer
extern FILE* yyin;
extern int yyparse();

int main(int argc, char** argv) {
    // Check if at least the input file is provided
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <input_file> [-o <output_file>]" << std::endl;
        return 1;
    }

    std::string inputFile;
    std::string outputFile = "output.o"; // Default output file name

    // Parse command-line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-o") {
            if (i + 1 < argc) {
                outputFile = argv[++i]; // Get the next argument as the output file name
                std::cout << "Output file set to: " << outputFile << std::endl; // Debug print
            } else {
                std::cerr << "Error: Missing output file name after -o" << std::endl;
                return 1;
            }
        } else {
            inputFile = arg; // Treat as the input file
        }
    }

    // Check if the input file is provided
    if (inputFile.empty()) {
        std::cerr << "Error: No input file specified" << std::endl;
        return 1;
    }

    // Open the input file
    FILE* file = fopen(inputFile.c_str(), "r");
    if (!file) {
        std::cerr << "Error opening file: " << inputFile << std::endl;
        return 1;
    }

    // Set the input file for the lexer
    yyin = file;

    // Start parsing
    if (yyparse() == 0) {
        // If parsing succeeds, assemble the operations
        Assembler& assembler = Assembler::getInstance();
        assembler.setOutputFile(outputFile); // Set the output file name
        assembler.assemble(); // Assemble the operations
        fclose(file);
        return 0;
    } else {
        // If parsing fails, return an error
        fclose(file);
        return 1;
    }
}