#ifndef EMULATOR_HPP
#define EMULATOR_HPP

#include <cstdint>
#include <string>
#include <vector>
#include <array>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <map>


class Emulator {
public:
    Emulator(const std::string& inputFileName);
    void loadMemory();
    void execute();
    void printProcessorState() const;
    void printMemory() const;

private:
    static constexpr size_t REGISTER_COUNT = 16;
    static constexpr size_t CSR_COUNT = 3;      // Control and Status Registers

    std::string inputFileName;
    std::map<uint32_t,uint8_t> memory;
    std::array<uint32_t, REGISTER_COUNT> registers{};
    std::array<uint32_t, CSR_COUNT> csr{};
    bool halted = false;

    // Add sp and pc as references to registers
    uint32_t& sp = registers[14]; // Stack Pointer
    uint32_t& pc = registers[15]; // Program Counter

    // Add status, handler, and cause as references to csr
    uint32_t& status = csr[0];    // Status Register
    uint32_t& handler = csr[1];   // Interrupt Handler Address
    uint32_t& cause = csr[2];     // Cause Register

    void executeInstruction();
    uint32_t fetchWord(uint32_t address);
    void storeWord(uint32_t address, uint32_t value);
};

#endif // EMULATOR_HPP