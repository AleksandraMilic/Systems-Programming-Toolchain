#ifndef HELPER_STRUCTURES_HPP
#define HELPER_STRUCTURES_HPP

#include <string>
#include <sstream>
#include <unordered_map>
using namespace std;


enum BIND {
    LOC,
    GLOB,
    EXT,
    ABS
};

enum RelocType { 
    R_X86_64_32, // 32-bit absolute relocation
};

enum class SymbolType {
    SCTN,
    NOTYP
};


enum  OperandType {
    // Immediate addresing  -  Nothing needs to be fetched from memory
    // Direct Addressing  -  Operand of an instruction refers directly to a location in memory
    // Indirect Addressing  -  The operand contains an address that points to a memory location holding the actual value
    NONE,

    CSR_IMMEDIATE, // for sys reg

    IMMEDIATE_LITERAL, IMMEDIATE_IDENT,   // $5 - constant;  $symbol - value of the symbol/label 
    DIR_LITERAL, DIR_IDENT,               // 5 - value in memory at address 5; symbol - value in memory on the address stored in symbol
    
    REGISTER_IMMEDIATE,              // %rx – value in register rx 
    REGISTER_INDIRECT,               // [%rx] - value in memory at the address stored in rx
    
    REGISTER_INDIRECT_LITERAL,       // [%reg + 4] – register + literal_displacement
    
    // OVO JE ZA C NIVO!!!!!! ld i st [reg + symbol]
    REGISTER_INDIRECT_SYMBOL,        // [%reg + var] – register + symbol_displacement
     
};


struct Operand {
    OperandType type;
    int32_t val; // reg or literal
    std::string symbol;     
    int32_t displacement;  

    // Default constructor
    Operand() : type(NONE), val(0), symbol(""), displacement(0) {}

    // Constructor for IMMEDIATE_LITERAL and DIR_LITERAL, AND ALSO REGISTER_IMMEDIATE and REGISTER_INDIRECT, CSWR
    Operand(OperandType t, int32_t v) : type(t), val(v), displacement(0) {}

    // Constructor for IMMEDIATE_IDENT and DIR_IDENT
    Operand(OperandType t, const std::string& sym) : type(t), val(0), symbol(sym), displacement(0) {}

    // Constructor for REGISTER_INDIRECT with literal displacement
    Operand(OperandType t, int32_t reg, int32_t disp) : type(t), val(reg), displacement(disp) {}

    // Constructor for REGISTER_INDIRECT with symbol displacement --- C NIVO
    Operand(OperandType t, int32_t reg, const std::string& sym) : type(t), val(reg), symbol(sym), displacement(0) {}

};



struct IdentOrLiteral
{
    /* type of element of array that stores both literals and symbols (identifiers) */
    char* indentifier;
    int32_t literal;
    bool isIdent;

    IdentOrLiteral(char* i, int32_t l, bool b): indentifier(i), literal(l), isIdent(b) {}
    
};



// @brief Disclaimer: The pool is not used in the current implementation of the assembler.
// struct Pool {
//     std::unordered_map<int32_t, std::vector<uint32_t>> literalAddresses; // Map of literal values to their offsets
//     std::unordered_map<std::string, std::vector<uint32_t>> symbolAddresses; // Map of symbol names to their offsets

//     void addLiteral(int32_t literal, uint32_t offset) {
//         literalAddresses[literal].push_back(offset);
//     }
//     void addSymbol(const std::string &symbol, uint32_t offset) {
//         symbolAddresses[symbol].push_back(offset);
//     }

//     // Get all offsets for a specific literal
//     const std::vector<uint32_t>& getliteralAddresses(int32_t literal) const {
//         static const std::vector<uint32_t> empty; // Return an empty vector if the literal is not found
//         auto it = literalAddresses.find(literal);
//         return (it != literalAddresses.end()) ? it->second : empty;
//     }

//     // Get all offsets for a specific symbol
//     const std::vector<uint32_t>& getsymbolAddresses(const std::string &symbol) const {
//         static const std::vector<uint32_t> empty; // Return an empty vector if the symbol is not found
//         auto it = symbolAddresses.find(symbol);
//         return (it != symbolAddresses.end()) ? it->second : empty;
//     }

//     bool hasLiteral(int32_t literal) const {
//         return literalAddresses.find(literal) != literalAddresses.end();
//     }
//     bool hasSymbol(const std::string &symbol) const {
//         return symbolAddresses.find(symbol) != symbolAddresses.end();
//     }


// };



#endif // HELPER_STRUCTURES_HPP
