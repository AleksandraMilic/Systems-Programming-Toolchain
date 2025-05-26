// InstructionOperation.hpp
#ifndef INSTRUCTION_OPERATION_HPP
#define INSTRUCTION_OPERATION_HPP

#include "Operation.hpp"
#include "../structures/helper_structures.hpp"
#include "../structures/Relocation.hpp"
#include <vector>
#include <string>
#include <cstdint>


enum class InstructionCategory {
    ARITHMETIC_LOGIC_XCHG,
    HALT_INT_IRET_RET,
    CSR,
    LD_ST,
    PUSH_POP,
    BRANCH,
    JUMP,
    CALL
};

class InstructionOperation : public Operation {
public:
    std::string instrName;
    InstructionCategory category;
    Operand gpr1;
    Operand gpr2;
    Operand csr;
    Operand operand;
    Operand ident;

    InstructionOperation(const std::string& name, const std::vector<Operand>& ops);
    static InstructionCategory detectCategory(const std::string& name);
    void processOperands(const std::vector<Operand>& ops);
    void print() const override;

    void addInstruction(std::string opcode, std::string mod, uint8_t A, uint8_t B, uint8_t C, uint32_t D) const;
    void execute() const override;
    void executeHalt() const;
    void executeInt() const;
    void executeIret() const;
    void executeRet() const;
    void executePush() const;
    void executePop() const;
    void executeLoad() const;
    void executeStore() const;
    void executeCsrRead() const;
    void executeCsrWrite() const;
    void executeXCHG() const;
    void executeCall() const;
    void executeJump() const;
    void executeBranch() const;
    void executeArithmeticLogic() const;

    void allocateAndAddValue(uint32_t value) const;

    

private:
    std::string operandToString(const Operand& op) const;
    std::unordered_map<std::string,uint8_t> OPCODE = {
        {"HALT",  0x0}, 
        {"INT",   0x1}, 
        {"IRET",  0x9}, //iret --> CSRWR_MEM (9) + POP 
        // {"RET",   0x9},  // ret-> pop (9)
        // call 0x2
        {"CALL",  0x2},  
        // jump & branches 0x3
        {"JMP",   0x3}, {"BEQ",   0x3}, {"BNE",   0x3}, {"BGT",   0x3},
        // atomic swap
        {"XCHG",  0x4},  
        // arithmetic (OC=5) 
        {"ADD",   0x5},  {"SUB",   0x5},
        {"MUL",   0x5},  {"DIV",   0x5},
        // logical (OC=6)
        {"NOT",   0x6},  {"AND",   0x6},
        {"OR",    0x6},  {"XOR",   0x6},
        // shifts
        {"SHL",   0x7},  {"SHR",   0x7},
        // store (OC=8) and push
        {"ST",    0x8}, {"PUSH",  0x8},
        // load (OC=9), pop and CSR
        {"LD",    0x9}, {"POP",   0x9}, {"CSRRD", 0x9}, {"CSRWR", 0x9}
    };


    std::unordered_map<std::string,uint8_t> MOD = {
        {"HALT",  0x0}, 
        {"INT",   0x0}, 
        // iret: 0x97 (csrwr_mem), 0x93 (pop) (opcode+mod)
        // {"IRET",  0x7},  //iret --> to do: CSRWR_MEM + POP
        // ret: 0x93 (pop) (opcode+mod)
        {"RET",   0x3},  // ret-> pop
        // call: 0x20, 0x21 (opcode+mod)
        {"CALL_LITERAL",  0x0}, {"CALL_IDENT",  0x1}, 
        // jump: 0x30, 0x38 (opcode+mod)
        {"JMP_LITERAL",   0x0}, {"JMP_IDENT",   0x8}, 
        // branches: 0x31, 0x32, 0x33,      0x39, 0xA, 0xB (opcode+mod)
        {"BEQ_LITERAL",   0x1}, {"BNE_LITERAL",   0x2}, {"BGT_LITERAL",   0x3},
        {"BEQ_IDENT",   0x9}, {"BNE_IDENT",   0xA}, {"BGT_IDENT",   0xB},
        // atomic swap: 0x40 (opcode+mod)
        {"XCHG",  0x0},  
        // arithmetic (OC=5) 
        {"ADD",   0x0},  {"SUB",   0x1},
        {"MUL",   0x2},  {"DIV",   0x3},
        // logical (OC=6)
        {"NOT",   0x0},  {"AND",   0x1},
        {"OR",    0x2},  {"XOR",   0x3},
        // shifts
        {"SHL",   0x0},  {"SHR",   0x1},
        
        // store: 0X80, 0x81 (opcode+mod) and push: 0x81
        // Store can't accept IMMED_LITERAL AND IMMED_IDENT !!! 
        {"ST_MEM",  0x0}, {"ST_MEM_MEM",  0x2}, {"PUSH",  0x1}, 

        // load: 0x91, 0x92;    pop: 0x93    and CSR: 0x90, 0x94-0x9 7 (mod)
        {"CSRRD", 0x0}, 
        {"LD_REG",    0x1}, {"LD_REG_MEM",    0x2}, 
        {"POP",   0x3},  
        {"CSRWR", 0x4}, {"CSRWR_OR", 0x5},  {"CSRWR_MEM", 0x6}, {"CSRWR_MEM_POSTINC", 0x7}
    };

};

#endif // INSTRUCTION_OPERATION_HPP
