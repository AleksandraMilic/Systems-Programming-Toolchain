#ifndef SECTION_HPP
#define SECTION_HPP

#include <string>
#include <vector>
#include <cstdint>
#include "Relocation.hpp" // Include the file where Relocation is defined
#include "ForwardRef.hpp" // Include the file where ForwardRef is defined



struct Section {
    std::string name;
    uint32_t startAddress = 0;         // početna adresa sekcije
    std::vector<uint8_t> machineCode;     // binarni podaci sekcije
    std::vector<Relocation> relocations; // relocation entries
    std::unordered_map<std::string, ForwardRef> forwardRefs; // For forward references
    uint32_t locCounter;
    uint32_t ndx;
    uint32_t size = 0;
    
    static uint32_t nextNdx; 

    Section() : startAddress(0) {}
    // for the linker
    Section(const std::string &n,uint32_t addr, uint32_t idx)
    : name(n), startAddress(addr), ndx(idx) {}
    // for the assembler
    Section(const std::string &n, uint32_t addr = 0)
        : name(n), startAddress(addr), ndx(nextNdx++) {}
    
    void appendData(const std::vector<uint8_t>& bytes) {
        machineCode.insert(machineCode.end(), bytes.begin(), bytes.end());
    }
    void updateLocCounter(uint32_t bytes) {
        locCounter += bytes;
    }
    uint32_t getLocCounter() const { return locCounter; }
};




#endif // SECTION_HPP
