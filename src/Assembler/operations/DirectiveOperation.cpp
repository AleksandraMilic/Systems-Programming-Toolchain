#include "../../../inc/Assembler/operations/DirectiveOperation.hpp"
#include "../../../inc/Assembler/Assembler.hpp"

#include <sstream>
#include <iostream>

void DirectiveOperation::print() const {
    std::ostringstream oss;
    oss << "Directive: "
        << (directive == "GLOBAL"  ? ".global" :
            directive == "EXTERN"  ? ".extern" :
            directive == "SECTION" ? ".section" :
            directive == "WORD"    ? ".word" :
            directive == "SKIP"    ? ".skip" :
            directive == "END"     ? ".end" :
            directive == "ASCII"   ? ".ascii" : directive);
    
    for (const auto &s : idList)
        oss << " " << s;
    
    for (const auto &item : idLiteralList)
        oss << " " << (item.isIdent ? item.indentifier : std::to_string(item.literal));
    
    if (!symbol.empty())
        oss << " " << symbol;
    
    if (literal.has_value())
        oss << " " << literal.value();
    
    // std::cout << oss.str() << std::endl;
}

void DirectiveOperation::execute() const {
    if (directive == "GLOBAL") {
        global_execute();
    } else if (directive == "EXTERN") {
        extern_execute();
    } else if (directive == "SECTION") {
        section_execute();
    } else if (directive == "WORD") {
        word_execute();
    } else if (directive == "SKIP") {
        skip_execute();
    } else if (directive == "END") {
        end_execute();
    } else if (directive == "ASCII") {
        ascii_execute();
    } else {
        std::cerr << "Error: Unknown directive '" << directive << "'." << std::endl;
    }
}
    
void DirectiveOperation::global_execute() const {
    auto &assembler = Assembler::getInstance();
    auto &symbolTable = assembler.getSymbolTable();

    for (const auto &symb : idList) {
        auto s = symbolTable.find(symb);
        if (s != symbolTable.end()) {
            // cout << "Bind of symbol : " << symb << " updated?" <<endl;
            symbolTable[symb].isGlobal = true;
            symbolTable[symb].bind = BIND::GLOB;
        } else {
        // Create a new global symbol
        // cout << "Symbol: " << symb << " added" <<endl;
        assembler.addSymbol(symb, 0, true, false, false, BIND::GLOB, SymbolType::NOTYP);

        }
    }
}
void DirectiveOperation::extern_execute() const {
    auto &assembler = Assembler::getInstance();
    auto &symbolTable = assembler.getSymbolTable();

    for (const auto &symb : idList) {
        auto s = symbolTable.find(symb);
        if (s != symbolTable.end()) {
            // cout << "Bind of symbol : " << symb << " updated?" <<endl;
            symbolTable[symb].isExtern = true;
            symbolTable[symb].bind = BIND::EXT;
        } 
        else {    
        // Create a new external symbol
        // cout << "Symbol: " << symb << " added" <<endl;

        assembler.addSymbol(symb, 0, false, true, false, BIND::EXT, SymbolType::NOTYP);
        }
    }
}
void DirectiveOperation::section_execute() const {
    auto &assembler = Assembler::getInstance();
    auto &symbolTable = assembler.getSymbolTable();
    auto &sectionTable = assembler.getSectionTable();

    if (symbol.empty()) {
        std::cerr << "Error: empty name." << std::endl;
        return;
    }
    Section* currentSection = assembler.getOrCreateSection(symbol);
    //Set THIS section as the current section in the assembler
    assembler.setCurrentSection(currentSection);

    //Add/update the section symbol in the section table
    if ( symbolTable.find(symbol) == symbolTable.end()) { //sectionTable.find(symbol) == sectionTable.end() --> already checked in assembler.getOrCreateSection(symbol)
        // Create a new symbol and add it to symbolTbl
        // cout << "Creating new section symbol: " << symbol << endl;
        assembler.addSymbol(symbol, 0, true, false, false, BIND::LOC, SymbolType::SCTN, sectionTable[symbol]->ndx);
    } else {
        std::cerr << "Error: section " << symbol << " already defined" << std::endl;  
        return;  
    }
}
void DirectiveOperation::word_execute() const {
    auto &assembler = Assembler::getInstance();
    auto *currentSection = assembler.getCurrentSection();

    if (!currentSection) {
        std::cerr << "Error: No current section for .word directive." << std::endl;
        return;
    }

    for (const auto &item : idLiteralList) {
        // cout << "Word: " << (item.isIdent ? item.indentifier : std::to_string(item.literal)) << std::endl;
        if (item.isIdent) {
            auto &symbolTable = assembler.getSymbolTable();
            auto s = symbolTable.find(item.indentifier);
            if (s == symbolTable.end() || !s->second.defined) {
                // Add a forward reference for the undefined symbol
                currentSection->forwardRefs[item.indentifier].addBackpatchOffset(currentSection->locCounter);
                // Reserve space in the section for the address
            } else {
                // Make relocation entry for the symbol
                // cout << "Relocation entry: " << item.indentifier << endl;
                currentSection->relocations.push_back(Relocation(
                    item.indentifier,                         // Symbol
                    currentSection->locCounter + 8,         // Offset
                    RelocType::R_X86_64_32,                 // Type
                    (symbolTable[item.indentifier].bind == BIND::GLOB || symbolTable[item.indentifier].bind == BIND::EXT) ? 
                    0 : symbolTable[item.indentifier].value   // Addend
                ));
            }
            currentSection->machineCode.insert(currentSection->machineCode.end(), {0, 0, 0, 0});
        } else {
            // Handle literal values
            uint32_t value = static_cast<uint32_t>(item.literal);
            currentSection->machineCode.push_back(static_cast<uint8_t>(value & 0xFF));
            currentSection->machineCode.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
            currentSection->machineCode.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
            currentSection->machineCode.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));

            currentSection->updateLocCounter(4); // Update the location counter by 4 bytes for each word

        }
    }
    
}
void DirectiveOperation::skip_execute() const {
    auto &assembler = Assembler::getInstance();
    auto *currentSection = assembler.getCurrentSection();
    if (literal.has_value()) {
        size_t sizeToSkip = literal.value();
        currentSection->machineCode.insert(currentSection->machineCode.end(), sizeToSkip, 0); // Fill with zeroes
        currentSection->updateLocCounter(sizeToSkip); // Update the location counter
    } else {
        std::cerr << "Error: .skip directive requires a literal value." << std::endl;
    }
}
void DirectiveOperation::end_execute() const {
    auto &assembler = Assembler::getInstance();
    auto *currentSection = assembler.getCurrentSection();    
    //Perform backpatching for forward references and "pool backpatching" for literals and symbols in the pool
    assembler.backpatching();
    // std::cout << "End of directive. Backpatching completed." << std::endl;
}
void DirectiveOperation::ascii_execute() const {
    auto &assembler = Assembler::getInstance();
    auto *currentSection = assembler.getCurrentSection();
    if (!symbol.empty()) {
        //NOTE: ASCII string is stored in the symbol field!!!
        currentSection->machineCode.insert(currentSection->machineCode.end(), symbol.begin(), symbol.end());
        currentSection->machineCode.push_back('\0'); // Null-terminate the string
        currentSection->updateLocCounter(symbol.size() + 1); //+1 for the null terminator
    } else {
        std::cerr << "Error: .ascii directive requires a string." << std::endl;
    }
}


