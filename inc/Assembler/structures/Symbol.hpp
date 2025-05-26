#ifndef SYMBOL_HPP
#define SYMBOL_HPP

#include <string>
#include <cstdint>
#include "../structures/helper_structures.hpp"



struct Symbol {
    std::string name;
    uint32_t idx;
    int value;
    bool isGlobal;
    bool isExtern;
    bool defined;
    BIND bind; // LOC, GLOB, EXT, ABS
    SymbolType type; // SCTN, NOTYP
    uint32_t ndx;

    static uint32_t nextIdx; 
    
    Symbol() 
    : name("") {}

    Symbol(const std::string &n, int val, bool glob, bool ext, bool def, BIND b, SymbolType t, uint32_t nind=-1)
    : name(n), idx(nextIdx++), value(val), isGlobal(glob), isExtern(ext), defined(def), bind(b), type(t), ndx(nind) {}
    
    // FOR LINKER
    Symbol(const std::string &n, uint32_t idx, int val, bool glob, bool ext, bool def, BIND b, SymbolType t, uint32_t nind=-1)
    : name(n), idx(idx), value(val), isGlobal(glob), isExtern(ext), defined(def), bind(b), type(t), ndx(nind) {}


};
#endif // SYMBOL_HPP
