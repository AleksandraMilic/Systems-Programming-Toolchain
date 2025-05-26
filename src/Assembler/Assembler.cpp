#include "../../inc/Assembler/Assembler.hpp"
#include <iomanip>
#include <algorithm>

// Private constructor - initializes members
Assembler::Assembler() : currentSection(nullptr) {
    // Open machine code file for writing output
    // output.open(fout, std::ios::out);
    // if (!output) {
    //     std::cerr << "Error opening machine code output file: " << fout << std::endl;
    // }
}

//memory management - destructor
void Assembler::cleanup() {
    for (auto& [name, section] : sectionTable) {
        delete section; // Free the memory for each Section
    }
    sectionTable.clear(); // Clear the map
}
Assembler::~Assembler() {
    cleanup();
}



// Returns the singleton instance of Assembler
Assembler& Assembler::getInstance() {
    static Assembler instance;
    return instance;
}
// Adds an operation to the operations list
void Assembler::addOperation(std::unique_ptr<Operation> op) {
    // updateLocCounter(op->byte_size);
    operations.push_back(std::move(op));
}
// For testing, we print all stored operations
void Assembler::printOperations() const {
    std::cout << "Printing all operations:" << std::endl;
    for (const auto& op : operations) {
        op->print();
    }
}
// Assemble by simply printing the operations (for testing purposes)
void Assembler::assemble() {
    std::cout << "Executing all operations:" << std::endl;
    for (const auto& op : operations) {
        op->execute();
    }
    // Print the symbol table
    // printSymbolTable();
    // Print the section table
    // printSectionTable();
    // Print the relocation table
    // printRelocationTable();
    // Print the machine code
    // printMachineCode();
    // Write the symbol table, reloc data, and machine code to the output ELF file
    writeOutput();
}

Section* Assembler::getOrCreateSection(const std::string &name) {
    if (sectionTable.find(name) == sectionTable.end()) {
        sectionTable[name] = new Section(name);
    }
    return sectionTable[name];
}
void Assembler::setCurrentSection(Section* section) {
    currentSection = section;
}
Section* Assembler::getCurrentSection() {
    return currentSection;
}
void Assembler::addSymbol(const std::string &name, uint32_t value, bool isGlobal, bool isExtern, bool defined, BIND bind, SymbolType type, uint32_t ndx) {
    symbolTable[name] = Symbol(name, value, isGlobal, isExtern, defined, bind, type, ndx);
}

void Assembler::printSymbolTable() const {
    std::cout << "#.symtab" << std::endl;
    std::cout << "Idx Value     Type    Bind   Ndx Name" << std::endl; // Removed "Size"

    // Create a vector of symbols and sort by idx
    std::vector<std::pair<std::string, Symbol>> sortedSymbols(symbolTable.begin(), symbolTable.end());
    std::sort(sortedSymbols.begin(), sortedSymbols.end(), [](const auto& a, const auto& b) {
        return a.second.idx < b.second.idx;
    });

    for (const auto& [name, symbol] : sortedSymbols) {
        std::cout << std::setw(3) << std::to_string(symbol.idx) << " " // Print symbol.idx in ascending order
                  << std::setw(8) << std::setfill('0') << std::hex << symbol.value << " "
                  << (symbol.type == SymbolType::SCTN ? "SCTN" : "NOTYP") << " " // Removed std::setw and std::setfill
                  << (symbol.bind == BIND::LOC ? "LOC" : symbol.bind == BIND::GLOB ? "GLOB" : "EXT") << " " // Removed std::setw and std::setfill
                  << std::setw(3) << (symbol.ndx == -1 ? "UND" : std::to_string(symbol.ndx)) << " "
                  << symbol.name << std::endl;
    }
}

// print section table
void Assembler::printSectionTable() const {
    std::cout << "#.sectab" << std::endl;
    std::cout << "Name   StartAddr   Size" << std::endl;

    for (const auto& [sectionName, section] : sectionTable) {
        std::cout << std::left << sectionName << " "
                  << std::setw(8) << std::setfill('0') << std::right << std::hex << section->startAddress << " "
                  << std::setw(8) << std::setfill('0') << section->machineCode.size() << std::endl;
    }
}

void Assembler::printRelocationTable() const {
    for (const auto& [sectionName, section] : sectionTable) {
        std::cout << "#.rela." << sectionName << std::endl;
        std::cout << "Offset     Type           Symbol Addend" << std::endl;

        // Sort relocations by offset in ascending order
        std::vector<Relocation> sortedRelocations = section->relocations;
        std::sort(sortedRelocations.begin(), sortedRelocations.end(), [](const Relocation& a, const Relocation& b) {
            return a.offset < b.offset;
        });

        for (const auto& relocation : sortedRelocations) {
            std::cout << std::setw(8) << std::setfill('0') << std::right << std::hex << relocation.offset << " "
                      << std::setw(14) << relocation.type << " "
                      << relocation.symbol << " "
                      << std::hex << relocation.addend << std::endl;
        }
    }
}

void Assembler::printMachineCode() const {
    for (const auto& [sectionName, section] : sectionTable) {
        std::cout << "#." << sectionName << std::endl;

        for (size_t i = 0; i < section->machineCode.size(); i++) {
            if (i % 16 == 0) {
                if (i > 0) std::cout << std::endl;
                std::cout << std::setw(8)  << std::hex << i << " ";
            }
            std::cout << std::setw(2) << std::setfill('0') << std::right <<  std::hex
                      << static_cast<int>(section->machineCode[i]) << " ";
        }
        std::cout << std::endl;
    }
}

// ****** BACKPATCHING FUNCTION ******
// This function resolves forward references and updates the machine code with the correct values
// Also creates relocation entries for undefined symbols
// It iterates through the forward references in the current section and checks if the symbols are defined

void Assembler::backpatching() {
    // Iterate through all sections in the section table
    for (auto& [sectionName, section] : sectionTable) {
        if (!section) {
            std::cerr << "Error: Section '" << sectionName << "' is null." << std::endl;
            continue;
        }

        // std::cout << "Processing forward references for section: " << sectionName << std::endl;

        // Handle forward references for the current section
        for (auto& entry : section->forwardRefs) {
            const std::string& symbolName = entry.first;
            auto& patchOffsets = entry.second.patchOffsets;

            // Find the symbol in the symbol table
            auto s = symbolTable.find(symbolName);
            if (s == symbolTable.end()) {
                std::cerr << "Error: Symbol '" << symbolName << "' not found in symbol table during backpatching." << std::endl;
                continue;
            }

            // std::cout << "Backpatching symbol: " << symbolName
            //           << ", defined: " << s->second.defined
            //           << ", idx: " << s->second.idx
            //           << ", ndx: " << s->second.ndx << std::endl;

            // if (!s->second.defined || s->second.ndx != section->ndx) {
            //     // Symbol is not defined or not in the current section => create relocation entries
            for (uint32_t offset : patchOffsets) {
                // std::cout << "Creating relocation entry for symbol: " << symbolName
                //             << ", offset: " << offset << std::endl;
                section->relocations.push_back(Relocation(
                    symbolName, // Symbol name
                    offset,
                    RelocType::R_X86_64_32, // Relocation type (adjust as needed)
                    (symbolTable[symbolName].bind == BIND::GLOB || symbolTable[symbolName].bind == BIND::EXT)
                        ? 0
                        : symbolTable[symbolName].value
                ));
                // }
            // } else {
            //     // Symbol is defined in the current section, resolve all offsets
            //     uint32_t symbolValue = s->second.value;
            //     std::cout << "Adding symbol value to machine code: " << symbolValue << std::endl;
            //     for (uint32_t offset : patchOffsets) {
            //         // Write the symbol value (little-endian) into the machine code
            //         section->machineCode[offset]     = static_cast<uint8_t>(symbolValue & 0xFF);
            //         section->machineCode[offset + 1] = static_cast<uint8_t>((symbolValue >> 8) & 0xFF);
            //         section->machineCode[offset + 2] = static_cast<uint8_t>((symbolValue >> 16) & 0xFF);
            //         section->machineCode[offset + 3] = static_cast<uint8_t>((symbolValue >> 24) & 0xFF);
            //     }
            // }
}

        // Update the section in the sectionTable
    }
}
}

void Assembler::writeOutput() {
    if (!output.is_open()) {
        std::cerr << "Error: Output file is not open." << std::endl;
        return;
    }

    // Write the symbol table
    output << "#.symtab" << std::endl;
    output << "Idx Value     Type    Bind   Ndx Name" << std::endl; // Removed "Size"

    // Sort symbols by their index for consistent output
    std::vector<std::pair<std::string, Symbol>> sortedSymbols(symbolTable.begin(), symbolTable.end());
    std::sort(sortedSymbols.begin(), sortedSymbols.end(), [](const auto& a, const auto& b) {
        return a.second.idx < b.second.idx;
    });

    for (const auto& [name, symbol] : sortedSymbols) {
        output << std::setw(3) << symbol.idx << " "
               << std::setw(8) << std::setfill('0') << std::right <<  symbol.value << " "
               << (symbol.type == SymbolType::SCTN ? "SCTN" : "NOTYP") << " "
               << (symbol.bind == BIND::LOC ? "LOC" : symbol.bind == BIND::GLOB ? "GLOB" : "EXT") << " "
               << (std::to_string(symbol.ndx)) << " " // UND???
               << name << std::endl;
    }
    output << "#end" << std::endl;


    // Write the section table
    output << "#.sectab" << std::endl;
    output << "Name   StartAddr   Size" << std::endl; 

    for (const auto& [sectionName, section] : sectionTable) {
        output << std::left << sectionName << " "
               << std::setw(8) << std::setfill('0') << section->startAddress << " "
               << std::setw(8) << std::setfill('0') << std::right << std::hex <<  section->machineCode.size() << std::endl;
    }
    output << "#end" << std::endl;


    // Write the relocation tables for each section
    for (const auto& [sectionName, section] : sectionTable) {
        output << "#.rela." << sectionName << std::endl;
        output << "Offset     Type           Symbol Addend" << std::endl;

        // Sort relocations by offset for consistent output
        std::vector<Relocation> sortedRelocations = section->relocations;
        std::sort(sortedRelocations.begin(), sortedRelocations.end(), [](const Relocation& a, const Relocation& b) {
            return a.offset < b.offset;
        });

        for (const auto& relocation : sortedRelocations) {
            output << std::setw(8) << std::setfill('0') << std::right <<  std::hex << relocation.offset << " "
                   << std::setw(14) << relocation.type << " "
                   << relocation.symbol << " "
                   << std::dec << relocation.addend << std::endl;
        }
        output << "#end" << std::endl;

    }

    // Write the machine code for each section
    for (const auto& [sectionName, section] : sectionTable) {
        output << "#.machineCode." << sectionName << std::endl;

        for (size_t i = 0; i < section->machineCode.size(); i++) {
            if (i % 16 == 0) {
                if (i > 0) output << std::endl;
                output << std::setw(8) << std::setfill('0') << std::right << std::hex << i << " ";
            }
            output << std::setw(2) << std::setfill('0') << std::hex
                   << static_cast<int>(section->machineCode[i]) << " ";
        }
        output << std::endl << "#end" << std::endl;
    }

    std::cout << "Output written successfully to the file." << std::endl;
}

void Assembler::setOutputFile(const std::string& fileName) {
    fout = fileName; // Set the output file name
    output.open(fout, std::ios::out); // Open the file in write mode
    if (!output) {
        std::cerr << "Error: Could not create or open output file: " << fout << std::endl;
        exit(1); // Exit if the file cannot be opened
    }
}

