#ifndef LINKER_HPP
#define LINKER_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <map>
#include <fstream>
#include <iostream>
#include "../Assembler/structures/Symbol.hpp"
#include "../Assembler/structures/Section.hpp"
#include "../Assembler/structures/Relocation.hpp"

struct ObjFiles {
    std::map<std::string, Section*> sections; // Map of sections in the object file
    std::unordered_map<std::string, Symbol> symbols; // Map of symbols in the object file
};


class Linker {
public:
    Linker();
    ~Linker();
    void processCommandLineArgs(int argc, char** argv);
    void linking();
    
private:
    //Command-line arguments 
    std::string outputFileName;
    bool generateHex = false;
    bool generateRelocatable = false;
    std::unordered_map<std::string, uint32_t> sectionPlacement;
    static uint32_t section_idx; // Change from int to uint32_t
    static uint32_t symbol_idx;  // Change from int to uint32_t

    //map of input files to be linked 
    std::map<std::string, ObjFiles*> inputFilesMap;

    //Internal data structures
    std::map<std::string, Section*> sections;
    std::unordered_map<std::string, Symbol> globalSymbols;
    std::vector<std::string> inputFiles;
    std::vector<uint8_t> globalMachineCode; // Combined machine code for all sections
    std::vector<std::string> sectionOrder;    // New vector to track insertion order

    //Helper functions for linking process
    void parseInput();
    void mapSections();
    void symbolDetermination();
    void resolveReloc();
    //error checking functions
    void checkForUnresolvedSymbols();
    void checkForOverlappingSections();

    //Writing output
    void generateOutput();
    void writeHexOutput(std::ofstream& output);
    void writeRelocatableOutput(std::ofstream& output);

    //Method for managing sections and symbols
    // uint32_t getNextAvailableAddress(std::string sectionName);
    //method for generating global machine code
    void generateGlobalMachineCode();

    // Print functions
    void printSymbolTable() const;
    void printRelocationTable() const;
    void printMachineCode() const;
    void printSectionTable() const;
    void printObjectFile() const;

    // Cleanup function to free memory
    void cleanup();

};

#endif // LINKER_HPP