#ifndef RELOCATION_HPP
#define RELOCATION_HPP

#include <cstdint>
#include <string>
#include "../structures/helper_structures.hpp"

// Struktura za relokacioni zapis – u ovom primeru samo za tip ABS32
struct Relocation {
    std::string symbol;
    uint32_t offset;
    RelocType type; // ABS32 or PCREL
    uint32_t addend;

    Relocation () = default; // Default constructor
    Relocation(const std::string& sym, uint32_t off, RelocType t, uint32_t add)
        : symbol(sym), offset(off), type(t), addend(add) {}


};

#endif // RELOCATION_HPP


