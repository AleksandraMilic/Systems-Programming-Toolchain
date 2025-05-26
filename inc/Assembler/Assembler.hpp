#ifndef ASSEMBLER_HPP
#define ASSEMBLER_HPP

#include <cstdint>
// #include <string>
#include <vector>
#include <memory>
#include <fstream>
#include <unordered_map>
#include <iostream>
#include "structures/Symbol.hpp"
#include "structures/Section.hpp"
#include "structures/ForwardRef.hpp"
#include "structures/Relocation.hpp"
#include "structures/helper_structures.hpp"
 
// Uključivanje operacija
#include "operations/Operation.hpp"
#include "operations/InstructionOperation.hpp"
#include "operations/DirectiveOperation.hpp"
#include "operations/LabelOperation.hpp"

class Assembler {
public:
    //ensures there is only one instance of Assembler
    static Assembler& getInstance();

    void addOperation(std::unique_ptr<Operation> op);
    void assemble();
    void backpatching();

    // Additional method for printing operations to test correctness
    void printOperations() const;

    // execute() can call those functions:
    Section* getOrCreateSection(const std::string &name);
    void setCurrentSection(Section* section);
    Section* getCurrentSection();
    void addSymbol(const std::string &name, uint32_t value, bool isGlobal, bool isExtern, bool defined, BIND bind, SymbolType type, uint32_t ndx = 0);
    
    // Print functions
    void printSymbolTable() const;
    void printRelocationTable() const;
    void printMachineCode() const;
    void printSectionTable() const;

    // Write the symbol table, reloc data, and machine code to the output ELF file
    void writeOutput();
    
    // Cleanup function to free memory and clear data structures
    void cleanup();

    // getters for symbol and section tables
    std::unordered_map<std::string, Symbol>& getSymbolTable() { return symbolTable; }
    std::unordered_map<std::string, Section*>& getSectionTable() { return sectionTable; }

    // Set the output file name
    void setOutputFile(const std::string& filename);
    Assembler();
    ~Assembler(); // Declare the destructor here

private:
    Assembler(const Assembler&) = delete;
    Assembler& operator=(const Assembler&) = delete;

    std::vector<std::unique_ptr<Operation>> operations;
    std::unordered_map<std::string, Symbol> symbolTable;
    std::unordered_map<std::string, Section*> sectionTable;
    
    Section* currentSection;
    std::fstream output;
    std::string fout;

};

#endif // ASSEMBLER_HPP
