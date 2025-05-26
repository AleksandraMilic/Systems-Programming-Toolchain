#include "../../../inc/Assembler/operations/LabelOperation.hpp"
#include "../../../inc/Assembler/Assembler.hpp"

#include <iostream>
#include <sstream>

LabelOperation::LabelOperation(const std::string &lab)//, uint32_t addr)
    : label(lab) {}

void LabelOperation::print() const {
    std::ostringstream oss;
    oss << "Label: " << label; // << " -> " << address;
    std::cout << oss.str() << std::endl;
}

void LabelOperation::execute() const {
    auto &assembler = Assembler::getInstance();
    auto *currentSection = assembler.getCurrentSection();
    auto &symbolTable = assembler.getSymbolTable();

    // Check if the label already exists in the symbol table
    auto l = symbolTable.find(label);
    if (l != symbolTable.end()) {
        if (l->second.defined) {
            std::cerr << "Error: Label '" << label << "' is already defined." << std::endl;
            return;
        } else {
            // cout << "Label '" << label << "' already exists, updating its location." << std::endl;
            // Update the global/extern symbol with the current location counter
            symbolTable[label].value = currentSection->locCounter;
            symbolTable[label].defined = true;
            symbolTable[label].type = SymbolType::NOTYP; //???
            symbolTable[label].ndx = currentSection->ndx; //???
        }
    } else {
        // Add the label to the symbol table
        // cout << "Creating new label symbol: " << label << std::endl;
        assembler.addSymbol(label, currentSection->locCounter, false, false, true, BIND::LOC, SymbolType::NOTYP, currentSection->ndx);
    }
}

