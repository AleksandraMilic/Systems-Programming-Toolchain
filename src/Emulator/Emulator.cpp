#include "../../inc/Emulator/Emulator.hpp"

/*check again:
-4 or 0xffc in instruction operation - push !!!
*/



Emulator::Emulator(const std::string& inputFileName) : inputFileName(inputFileName) {
    // Initialize memory mapped registers? - nivo B
    // for(uint32_t i = 0xffffff00; i < 0xffffffff; i++) {
    //     memory[i] = 0;
    // }    
    registers.fill(0); 
    pc = 0x40000000;
    sp = 0;
    handler = 0;
    status = 0;
    cause = 0;
}

void Emulator::loadMemory() {
    std::ifstream inputFile(inputFileName);

    std::string line;
    while (std::getline(inputFile, line)) {
        if (line.empty() || line[0] == '#') continue; // Skip empty lines or comments

        std::istringstream iss(line);
        std::string addressStr;
        iss >> addressStr; // Read the address (e.g., "0000:")

        // Remove the colon from the address string
        if (addressStr.back() == ':') {
            addressStr.pop_back();
        } else {
            throw std::runtime_error("Error: Invalid address format in line: " + line);
        }

        uint32_t address;
        try {
            address = std::stoul(addressStr, nullptr, 16); // Convert address to integer
        } catch (const std::exception& e) {
            throw std::runtime_error("Error: Invalid address in line: " + line);
        }

        // Find the colon and get the rest of the line (the bytes)
        size_t colonPos = line.find(':');
        if (colonPos == std::string::npos) {
            throw std::runtime_error("Error: Invalid address format in line: " + line);
        }
        std::string bytesPart = line.substr(colonPos + 1);

        // Now split bytesPart by whitespace
        std::istringstream bytesStream(bytesPart);
        std::string byteStr;
        size_t offset = 0;
        while (bytesStream >> byteStr) {
            // Remove any whitespace
            byteStr.erase(0, byteStr.find_first_not_of(" \r\n\t"));
            byteStr.erase(byteStr.find_last_not_of(" \r\n\t") + 1);

            if (byteStr.empty()) continue;
            if (byteStr.size() != 2 || !isxdigit(byteStr[0]) || !isxdigit(byteStr[1])) {
                throw std::runtime_error("Error: Invalid byte format in line: " + line);
            }
            uint8_t byte = static_cast<uint8_t>(std::stoul(byteStr, nullptr, 16));
            // if (address + offset >= MEMORY_SIZE) {
            //     throw std::runtime_error("Error: Memory address out of bounds.");
            // }
            memory[address + offset] = byte;
            ++offset;
        }
    }

    inputFile.close();
    std::cout << "Memory loading complete.\n";
    
}

void Emulator::execute() {
    while (!halted) {
        executeInstruction();
        printProcessorState();
    }
}

void Emulator::executeInstruction() {
    uint32_t instruction = fetchWord(pc);
    //print registers

    pc += 4; // Advance the program counter

    uint8_t opcode = (instruction >> 28) & 0xF; // 4 bits for opcode
    uint8_t mode = (instruction >> 24) & 0xF;   // 4 bits for mode
    uint8_t regA = (instruction >> 20) & 0xF;   // 4 bits for regA
    uint8_t regB = (instruction >> 16) & 0xF;   // 4 bits for regB
    uint8_t regC = (instruction >> 12) & 0xF;   // 4 bits for regC
    uint16_t DDD = instruction & 0xFFF;         // 12 bits for DDD
    int32_t DDD_signed = (DDD & 0x800) ? (DDD | 0xFFFFF000) : DDD;
    // print the instruction
    std::cout << "INSTRUCTION: 0x" << std::hex << std::setw(8) << std::setfill('0') << instruction
              << " (PC: 0x" << pc - 4 << ")" << std::endl;
    // print opcode, mode, regA, regB, regC, DDD
    std::cout << "Opcode: 0x" << std::hex << std::setw(2) << std::setfill('0') << (int)opcode
              << ", Mode: 0x" << std::hex << std::setw(1) << std::setfill('0') << (int)mode
              << ", regA: 0x" << std::hex << std::setw(1) << std::setfill('0') << (int)regA
              << ", regB: 0x" << std::hex << std::setw(1) << std::setfill('0') << (int)regB
              << ", regC: 0x" << std::hex << std::setw(1) << std::setfill('0') << (int)regC
              << ", DDD: 0x" << std::hex << std::setw(3) << std::setfill('0') << DDD
              << std::endl;
    switch (opcode) {
        case 0x00: // HALT
            if (mode != 0 || regA != 0 || regB != 0 || regC != 0 || DDD != 0) {
                throw std::runtime_error("Error: Invalid HALT instruction.");
            }
            std::cout << "------------------------------------------------------------" << std::endl
                      << "Emulated processor executed halt instruction" << std::endl;
            printProcessorState();
            halted = true;
            return;

        case 0x01: // INTERRUPT
            if (mode != 0 || regA != 0 || regB != 0 || regC != 0 || DDD != 0) {
                throw std::runtime_error("Error: Invalid INTERRUPT instruction.");
            }
            
            sp -= 4;
            storeWord(sp, status);
            sp -= 4;
            storeWord(sp, pc);cause = 4;
            status &= ~0x1; // Clear the least significant bit
            pc = handler;
            break;

        case 0x02: // CALL
            if (regC != 0) {
                throw std::runtime_error("Error: Invalid CALL instruction.");
            }
            sp -= 4;
            storeWord(sp, pc);
            if (mode == 0) {
                pc = registers[regA] + registers[regB] + DDD_signed;
            } else if (mode == 1) {
                pc = fetchWord(registers[regA] + registers[regB] + DDD_signed);
            } else {
                throw std::runtime_error("Error: Invalid CALL mode.");
            }
            break;

        case 0x03: // JUMP
            switch (mode) {
                case 0:
                    pc = registers[regA] + DDD_signed;
                    break;
                case 1:
                    if (registers[regB] == registers[regC]) {
                        pc = registers[regA] + DDD_signed;
                    }
                    break;
                case 2:
                    if (registers[regB] != registers[regC]) {
                        pc = registers[regA] + DDD_signed;
                    }
                    break;
                case 3:
                    if ((int32_t)registers[regB] > (int32_t)registers[regC]) {
                        pc = registers[regA] + DDD_signed;
                    }
                    break;
                case 8:
                    pc = fetchWord(registers[regA] + DDD_signed);
                    break;
                case 9:
                    if (registers[regB] == registers[regC]) {
                        pc = fetchWord(registers[regA] + DDD_signed);
                    }
                    break;
                case 10:
                    if (registers[regB] != registers[regC]) {
                        pc = fetchWord(registers[regA] + DDD_signed);
                    }
                    break;
                case 11:
                    if ((int32_t)registers[regB] > (int32_t)registers[regC]) {
                        pc = fetchWord(registers[regA] + DDD_signed);
                    }
                    break;
                default:
                    throw std::runtime_error("Error: Invalid JUMP mode.");
            }
            break;

        case 0x04: // SWAP
            if (mode != 0 || regA != 0 || DDD != 0) {
                throw std::runtime_error("Error: Invalid SWAP instruction.");
            }
            std::swap(registers[regB], registers[regC]);
            registers[0] = 0; // Ensure r0 is always 0
            break;

        case 0x05: // ALU Operations
            if (DDD != 0) {
                throw std::runtime_error("Error: Invalid ALU instruction.");
            }
            if (regA == 0) break; // r0 is always 0
            switch (mode) {
                case 0:
                    registers[regA] = registers[regB] + registers[regC];
                    break;
                case 1:
                    registers[regA] = registers[regB] - registers[regC];
                    break;
                case 2:
                    registers[regA] = registers[regB] * registers[regC];
                    break;
                case 3:
                    registers[regA] = registers[regB] / registers[regC];
                    break;
                default:
                    throw std::runtime_error("Error: Invalid ALU mode.");
            }
            break;

        case 0x06: // Logical Operations
            if (DDD != 0) {
                throw std::runtime_error("Error: Invalid Logical instruction.");
            }
            if (regA == 0) break; // r0 is always 0
            switch (mode) {
                case 0:
                    registers[regA] = ~registers[regB];
                    break;
                case 1:
                    registers[regA] = registers[regB] & registers[regC];
                    break;
                case 2:
                    registers[regA] = registers[regB] | registers[regC];
                    break;
                case 3:
                    registers[regA] = registers[regB] ^ registers[regC];
                    break;
                default:
                    throw std::runtime_error("Error: Invalid Logical mode.");
            }
            break;

        case 0x07: // Shift Operations
            if (DDD != 0) {
                throw std::runtime_error("Error: Invalid Shift instruction.");
            }
            if (regA == 0) break; // r0 is always 0
            switch (mode) {
                case 0:
                    registers[regA] = registers[regB] << registers[regC];
                    break;
                case 1:
                    registers[regA] = registers[regB] >> registers[regC];
                    break;
                default:
                    throw std::runtime_error("Error: Invalid Shift mode.");
            }
            break;

        case 0x08: // Memory Store
            switch (mode) {
                case 0:
                    storeWord(registers[regA] + registers[regB] + DDD_signed, registers[regC]);
                    break;
                case 1:
                    if (regA != 0) {
                        registers[regA] += DDD_signed;
                    }
                    storeWord(registers[regA], registers[regC]);
                    break;
                case 2:
                    storeWord(fetchWord(registers[regA] + registers[regB] + DDD_signed), registers[regC]);
                    break;

                default:
                    throw std::runtime_error("Error: Invalid Memory Store mode.");
            }
            break;

        case 0x09: // CSR Operations
            switch (mode) {
                case 0:
                    if (regA != 0) registers[regA] = csr[regB];
                    break;
                case 1:
                    if (regA != 0) registers[regA] = registers[regB] + DDD_signed;
                    break;
                case 2:
                    if (regA != 0) registers[regA] = fetchWord(registers[regB] + registers[regC] + DDD_signed);
                    break;
                case 3:
                    if (regA != 0) {
                        registers[regA] = fetchWord(registers[regB]);
                        registers[regB] += DDD_signed;
                    }
                    break;
                case 4:
                    csr[regA] = registers[regB];
                    break;
                case 5:
                    csr[regA] = registers[regB] | DDD;
                    break;
                case 6:
                    csr[regA] = fetchWord(registers[regB] + registers[regC] + DDD_signed);
                    break;
                case 7:
                    csr[regA] = fetchWord(registers[regB]);
                    registers[regB] += DDD_signed;
                    break;
                default:
                    throw std::runtime_error("Error: Invalid CSR mode.");
            }
            break;

        default:
            throw std::runtime_error("Error: Unknown opcode encountered.");
    }
}

uint32_t Emulator::fetchWord(uint32_t address)  {
    std::cout << "************ PC ************* " << address << std::endl;
    if (memory.find(address) == memory.end()) {
        throw std::runtime_error("Error: Memory address out of bounds.");
    }
    return (memory[address + 3] << 24) | (memory[address + 2] << 16) |
           (memory[address + 1] << 8) | memory[address];
}

void Emulator::storeWord(uint32_t address, uint32_t value) {
    std::cout << "STORED WORD: 0x" << std::hex << std::setw(8) << std::setfill('0') << value
              << " at address: 0x" << std::hex << std::setw(8) << std::setfill('0') << address
              << std::endl;
    memory[address]     = value & 0xFF;
    memory[address + 1] = (value >> 8) & 0xFF;
    memory[address + 2] = (value >> 16) & 0xFF;
    memory[address + 3] = (value >> 24) & 0xFF;
}

void Emulator::printProcessorState() const {
    std::cout << "Emulated processor executed halt instruction\n";
    std::cout << "Emulated processor state:\n";
    for (size_t i = 0; i < REGISTER_COUNT; ++i) {
        std::cout << "r" << i << "=0x" << std::setw(8) << std::setfill('0') << std::hex << registers[i] << " ";
        if ((i + 1) % 4 == 0) std::cout << "\n";
    }
    std::cout << "CSR: ";
    for (size_t i = 0; i < CSR_COUNT; ++i) {
        std::cout << "csr" << i << "=0x" << std::setw(8) << std::setfill('0') << std::hex << csr[i] << " ";
        if ((i + 1) % 4 == 0) std::cout << "\n";
    }
    std::cout << "\n";
}

void Emulator::printMemory() const {
    std::ofstream output("emuls_output.e");
    if (!output.is_open()) {
        std::cerr << "Error: Could not open emulator_output.emu for writing." << std::endl;
        return;
    }
    int i = 0;
    output << "Memory:";
    for (const auto& par : memory) {
        if (i % 4 == 0) output << std::endl << std::hex << std::setw(8) << std::setfill('0') << par.first << ":";
        output << " " << std::hex << std::setw(2) << std::setfill('0') << (int)par.second;
        ++i;
    }
    output << std::endl;
    output.close();
}