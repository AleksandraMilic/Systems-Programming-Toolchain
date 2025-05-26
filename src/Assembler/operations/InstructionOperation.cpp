#include "../../../inc/Assembler/operations/InstructionOperation.hpp"
#include "../../../inc/Assembler/Assembler.hpp"
#include <sstream>
#include <iostream>
#include <string>


// TO DO: FORWARD REFERENCE: +4 OR +8 ???
// DD CD AB OPCODEMOD

InstructionOperation::InstructionOperation(const std::string& name, const std::vector<Operand>& ops)
    : instrName(name), category(detectCategory(name)) {
    processOperands(ops);
}
InstructionCategory InstructionOperation::detectCategory(const std::string& name) {
    if (name == "XCHG" || name == "ADD" || name == "SUB" || name == "MUL" || name == "DIV" 
        || name == "AND" || name == "OR" || name == "XOR" || name == "NOT" || name == "SHL" || name == "SHR")
        return InstructionCategory::ARITHMETIC_LOGIC_XCHG;
    if (name == "HALT" || name == "INT" || name == "IRET" || name == "RET")
        return InstructionCategory::HALT_INT_IRET_RET;
    if (name == "CSRRD" || name == "CSRWR")
        return InstructionCategory::CSR;
    if (name == "LD" || name == "ST")
        return InstructionCategory::LD_ST;
    if (name == "PUSH" || name == "POP")
        return InstructionCategory::PUSH_POP;
    if (name == "BEQ" || name == "BNE" || name == "BGT")
        return InstructionCategory::BRANCH;
    if (name == "JMP")
        return InstructionCategory::JUMP;
    if (name == "CALL")
        return InstructionCategory::CALL;
    return InstructionCategory::ARITHMETIC_LOGIC_XCHG;
}
void InstructionOperation::processOperands(const std::vector<Operand>& ops) {
    switch (category) {
        case InstructionCategory::ARITHMETIC_LOGIC_XCHG:
            if (instrName != "NOT"){
                gpr1 = ops[0];
                gpr2 = ops[1];
            } else { gpr1 = ops[0]; }
            break;
        case InstructionCategory::LD_ST:
            if (instrName == "LD") {
                operand = ops[0];
                gpr1 = ops[1];
            } else if (instrName == "ST") {
                gpr1 = ops[0];
                operand = ops[1];
            }    
            break;
        case InstructionCategory::CSR:
            if (instrName == "CSRRD"){
                csr = ops[0];
                gpr1 = ops[1];
            } else if (instrName == "CSRWR") {
                gpr1 = ops[0];
                csr = ops[1];
            }       
            break;
        case InstructionCategory::PUSH_POP:
            gpr1 = ops[0];
            break;
        case InstructionCategory::BRANCH:
            gpr1 = ops[0];
            gpr2 = ops[1];
            operand = ops[2];
            break;
        case InstructionCategory::JUMP:
        case InstructionCategory::CALL:
            operand = ops[0];
            break;
        default:
            if (!ops.empty()) gpr1 = ops[0];
            if (ops.size() > 1) gpr2 = ops[1];
            break;
    }
}
std::string InstructionOperation::operandToString(const Operand& op) const {
    std::ostringstream oss;
    switch (op.type) {
        case OperandType::IMMEDIATE_LITERAL:
            oss << "$" << op.val;
            break;
        case OperandType::IMMEDIATE_IDENT:
            oss << "$" << op.symbol;
            break;
        case OperandType::DIR_LITERAL:
            oss << op.val;
            break;
        case OperandType::DIR_IDENT:
            oss << op.symbol;
            break;
        case OperandType::CSR_IMMEDIATE:
            if (op.val==0){ oss << "%status, value: " << op.val;}
            else if (op.val==1){ oss << "%handler, value: " << op.val;}
            else if (op.val==2){ oss << "%cause, value: " << op.val;}
            break;
        case OperandType::REGISTER_IMMEDIATE:
            oss << "%r" << op.val;
            break;
        case OperandType::REGISTER_INDIRECT:
            oss << "[%r" << op.val << "]";
            break;
        case OperandType::REGISTER_INDIRECT_LITERAL:
            oss << "[%r" << op.val << " + " << op.displacement << "]";
            break;
        case OperandType::REGISTER_INDIRECT_SYMBOL:
            oss << "[%r" << op.val << " + " << op.symbol << "]";
            break;
    }
    return oss.str();
}
void InstructionOperation::print() const {
    std::ostringstream oss;
    oss << "Instrukcija: " << instrName << " ";
    if (gpr1.type != OperandType::NONE || gpr1.val) oss << operandToString(gpr1) << " ";
    if (gpr2.type != OperandType::NONE || gpr2.val) oss << operandToString(gpr2) << " ";
    if (csr.type != OperandType::NONE || csr.val) oss << operandToString(csr) << " ";
    if (operand.type != OperandType::NONE || operand.val) oss << operandToString(operand);
    //std::cout << oss.str() << std::endl;
}

void InstructionOperation::addInstruction(std::string opcode, std::string mod, uint8_t A, uint8_t B, uint8_t C, uint32_t D) const {
    auto &assembler = Assembler::getInstance();
    auto *currentSection = assembler.getCurrentSection();
    if (!currentSection) {
        std::cerr << "Error: Current section is null." << std::endl;
        return;
    }
    // cout << "Adding instruction: " << opcode << " " << mod << " " << (int)A << " " << (int)B << " " << (int)C << " " << D << endl;
    if (OPCODE.find(opcode) == OPCODE.end()) {
        std::cerr << "Error: Opcode '" << opcode << "' not found in OPCODE map." << std::endl;
        return;
    }
    uint8_t opcodeValue = OPCODE.at(opcode);// const metdod of OPCODE[opcode];
    uint8_t modifierValue = MOD.at(mod);

    uint32_t displacement = D & 0xFFF;
    uint32_t instruction = 0;
    instruction |= (opcodeValue << 28);       
    instruction |= (modifierValue << 24);     
    instruction |= ((A & 0xF) << 20);                 
    instruction |= ((B & 0xF) << 16);                 
    instruction |= ((C & 0xF) << 12);
    // instruction |= ((displacement >> 8) & 0xF) << 8; 
    // instruction |= (displacement & 0xFF);  
    instruction |= displacement; // 12 bits for displacement
    // prints intruction in hex format
    // cout << "Instruction: " << std::hex << instruction << std::dec << endl;
    // little-endian storage
    currentSection->machineCode.push_back(instruction & 0xFF);         // Byte 1 (LSB) - DD
    currentSection->machineCode.push_back((instruction >> 8) & 0xFF);  // Byte 2 - CD
    currentSection->machineCode.push_back((instruction >> 16) & 0xFF); // Byte 3 - AB
    currentSection->machineCode.push_back((instruction >> 24) & 0xFF); // Byte 4 (MSB)- OPCODEMOD
    currentSection->updateLocCounter(4); // Update the location counter by 4 bytes for each instruction
}
void InstructionOperation::execute() const {
    auto &assembler = Assembler::getInstance();
    auto *currentSection = assembler.getCurrentSection();

    // Call the appropriate function based on the instruction name
    if (instrName == "HALT") {
        executeHalt();
    } else if (instrName == "INT") {
        executeInt();
    } else if (instrName == "IRET") {
        executeIret();
    } else if (instrName == "RET") {
        executeRet();
    } else if (instrName == "PUSH") {
        executePush();
    } else if (instrName == "POP") {
        executePop();
    } else if (instrName == "LD") {
        executeLoad();
    } else if (instrName == "ST") {
        executeStore();
    } else if (instrName == "CSRRD") {
        executeCsrRead();
    } else if (instrName == "CSRWR") {
        executeCsrWrite();
    } else if (instrName == "XCHG") {
        executeXCHG();
    } else if (instrName == "ADD" || instrName == "SUB" || instrName == "MUL" || instrName == "DIV" ||
               instrName == "AND" || instrName == "OR" || instrName == "XOR" || instrName == "NOT" ||
               instrName == "SHL" || instrName == "SHR") {
        executeArithmeticLogic();
    } else if (instrName == "BEQ" || instrName == "BNE" || instrName == "BGT") {
        executeBranch();
    } else if (instrName == "JMP") {
        executeJump();
    } else if (instrName == "CALL") {
        executeCall();
    } else {
        std::cerr << "Error: Unknown instruction '" << instrName << "'." << std::endl;
    }
}

// ***** HALT/INT/IRET/RET INSTRUCTIONS ****
void InstructionOperation::executeHalt() const {
    //std::cout << "Executing HALT instruction." << std::endl;
    addInstruction("HALT", "HALT", 0, 0, 0, 0); 
}
void InstructionOperation::executeInt() const {
    //std::cout << "Executing INT instruction." << std::endl;
    addInstruction("INT", "INT", 0, 0, 0, 0); 
}
void InstructionOperation::executeIret() const {
    //std::cout << "Executing IRET instruction." << std::endl;
    addInstruction("CSRWR", "CSRWR_MEM", 0, 14, 0 , 4); // CSR0 = MEM[SP+4] 
    // First, we need to pop STATUS from the stack.  
    // If we pop PC first, the context changes and we won’t get a chance to restore STATUS.  
    // That’s why we pop STATUS first using an instruction with no side effects on SP,  
    // then pop PC, which atomically increases SP by 8 (with a single POP instruction).
    addInstruction("POP", "POP", 15, 14, 0, 8);  // PC = MEM[SP], SP = SP + 8
}
void InstructionOperation::executeRet() const {
    //std::cout << "Executing RET instruction." << std::endl;
    // Implemented as POP instruction
    addInstruction("POP", "POP", 15, 14, 0, 4); // PC = MEM[SP], SP = SP + 4
}

// ***** PUSH/POP INSTRUCTIONS ****
void InstructionOperation::executePush() const {
    // Implement PUSH instruction
    //std::cout << "Executing PUSH instruction." << std::endl;
    // -4 == 0xFFC
    addInstruction("PUSH", "PUSH", 14, 0, gpr1.val, 0xFFC); // SP = SP - 4, MEM[SP] = gpr1
}   
void InstructionOperation::executePop() const {
    // Implement POP instruction 
    //std::cout << "Executing POP instruction." << std::endl;
    addInstruction("POP", "POP", gpr1.val, 14, 0, 4); // gpr1 = MEM[SP], SP = SP + 4
}

// ***** LOAD/STORE INSTRUCTIONS ****
void InstructionOperation::executeLoad() const {
    // Implement LD (Load) instruction 
    //std::cout << "Executing LOAD instruction." << std::endl;
    auto &assembler = Assembler::getInstance();
    auto *currentSection = assembler.getCurrentSection();

    if (operand.type == OperandType::IMMEDIATE_IDENT) { // LD $SYMBOL, gpr1
        auto &symbolTable = assembler.getSymbolTable();
        auto s = symbolTable.find(operand.symbol);
        // -4 ?????
        // Symbol is not defined or not in the same section => make a forward reference

        if (s == symbolTable.end() || !s->second.defined 
            || symbolTable[operand.symbol].ndx != currentSection->ndx) {
            // cout << "Forward reference: " << operand.symbol << endl;
            currentSection->forwardRefs[operand.symbol].addBackpatchOffset(currentSection->locCounter + 8); // ?? +8
        }
        else {
            // Make relocation entry for the symbol
            // cout << "Relocation entry: " << operand.symbol << endl;
            currentSection->relocations.push_back(Relocation(
                operand.symbol,                         // Symbol
                currentSection->locCounter + 8,         // Offset
                RelocType::R_X86_64_32,                 // Type
                (symbolTable[operand.symbol].bind == BIND::GLOB || symbolTable[operand.symbol].bind == BIND::EXT) ? 
                0 : symbolTable[operand.symbol].value   // Addend
            ));
        }
        addInstruction("LD","LD_REG_MEM", gpr1.val, 15, 0, 4); // ld [pc+4], gpr1;          gpr1 <= mem[pc+4]
        addInstruction("JMP", "JMP_LITERAL", 15, 0, 0, 4); // jmp pc+4
        allocateAndAddValue(0);  // Placeholder for the symbol value 

    } else if (operand.type == OperandType::IMMEDIATE_LITERAL) {  // LD $LITERAL, gpr1
        addInstruction("LD","LD_REG_MEM", gpr1.val, 15, 0, 4); // ld [pc+4], gpr1 ---> gpr1 = mem[pc+4]
        addInstruction("JMP", "JMP_LITERAL", 15, 0, 0, 4); // jmp pc+4
        // Allocate space for the literal value
        allocateAndAddValue(operand.val); 
    } else if (operand.type == OperandType::DIR_LITERAL) {
        addInstruction("LD", "LD_REG_MEM", gpr1.val, 15, 0, 8); // ld [pc+8], gpr1 ---> gpr1 = mem[pc+8]
        addInstruction("LD", "LD_REG_MEM", gpr1.val, gpr1.val, 0, 0); // ld [gpr1], gpr1 ---> gpr1 = mem[gpr1]
        addInstruction("JMP", "JMP_LITERAL", 15, 0, 0, 4); // jmp pc+4
        // Allocate space for the literal value
        allocateAndAddValue(operand.val); 
    } else if (operand.type == OperandType::DIR_IDENT) {
        auto &symbolTable = assembler.getSymbolTable();
        auto s = symbolTable.find(operand.symbol);
        // Symbol is not defined or not in the same section => make a forward reference
        if (s == symbolTable.end() || !s->second.defined 
            || symbolTable[operand.symbol].ndx != currentSection->ndx) {

            currentSection->forwardRefs[operand.symbol].addBackpatchOffset(currentSection->locCounter + 12); // ?? +8
        }
        else {
            // Make relocation entry for the symbol
            currentSection->relocations.push_back(Relocation(
                operand.symbol,                         // Symbol
                currentSection->locCounter + 12,         // Offset
                RelocType::R_X86_64_32,                 // Type
                (symbolTable[operand.symbol].bind == BIND::GLOB || symbolTable[operand.symbol].bind == BIND::EXT) ? 
                0 : symbolTable[operand.symbol].value   // Addend
            ));
        }
        addInstruction("LD", "LD_REG_MEM", gpr1.val, 15, 0, 8); // ld [pc+8], gpr1 ---> gpr1 = mem[pc+8]
        addInstruction("LD", "LD_REG_MEM", gpr1.val, gpr1.val, 0, 0); // ld [gpr1], gpr1 ---> gpr1 = mem[gpr1]
        addInstruction("JMP", "JMP_LITERAL", 15, 0, 0, 4); // jmp pc+4
        allocateAndAddValue(0);  // Placeholder for the symbol value 

    
    } else if (operand.type == OperandType::REGISTER_IMMEDIATE) { // reg in reg 
        addInstruction("LD", "LD_REG", gpr1.val, operand.val, 0, 0); // LD reg, gpr1 
    } else if (operand.type == OperandType::REGISTER_INDIRECT) { 
        addInstruction("LD", "LD_REG_MEM", gpr1.val, 0, operand.val, 0); // LD [reg], gpr1    
    } else if (operand.type == OperandType::REGISTER_INDIRECT_LITERAL) {
        if (operand.displacement > 0xFFF || operand.displacement < -0x800) {
            // Check if the displacement is within the range of 12 bits
            std::cerr << "Error: Displacement out of range." << std::endl;
            return;
        }
        addInstruction("LD", "LD_REG_MEM", gpr1.val, 0, operand.val, operand.displacement); // LD [reg+d], gpr1
    }
    
}
void InstructionOperation::executeStore() const {
    // Implement ST (Store) instruction 
    //std::cout << "Executing STORE instruction." << std::endl;
    auto &assembler = Assembler::getInstance();
    auto *currentSection = assembler.getCurrentSection();

    if (operand.type == OperandType::IMMEDIATE_IDENT || operand.type == OperandType::IMMEDIATE_LITERAL) {
        std::cerr << "Error: ST instruction cannot use immediate operand." << std::endl;
        return;     
    }
    if (operand.type == OperandType::DIR_IDENT) { 
        auto &symbolTable = assembler.getSymbolTable();
        auto s = symbolTable.find(operand.symbol);
        // -4 ?????
        // Symbol is not defined or not in the same section => make a forward reference
        if (s == symbolTable.end() || !s->second.defined 
            || symbolTable[operand.symbol].ndx != currentSection->ndx) {

            currentSection->forwardRefs[operand.symbol].addBackpatchOffset(currentSection->locCounter + 8); // ?? +8
        }
        else {
            // Make relocation entry for the symbol
            currentSection->relocations.push_back(Relocation(
                operand.symbol,                         // Symbol
                currentSection->locCounter + 8,         // Offset
                RelocType::R_X86_64_32,                 // Type
                (symbolTable[operand.symbol].bind == BIND::GLOB || symbolTable[operand.symbol].bind == BIND::EXT) ? 
                0 : symbolTable[operand.symbol].value   // Addend
            ));
        }
        addInstruction("ST","ST_MEM_MEM", 15, 0, gpr1.val, 4); // st gpr1, [[pc+4]]
        addInstruction("JMP", "JMP_LITERAL", 15, 0, 0, 4); // jmp pc+4
        allocateAndAddValue(0);  // Placeholder for the symbol value 
  
    } else if (operand.type == OperandType::DIR_LITERAL) {
        addInstruction("ST", "ST_MEM_MEM", 15, 0, gpr1.val, 4); // st gpr1, [[pc+4]]
        addInstruction("JMP", "JMP_LITERAL", 15, 0, 0, 4); // jmp pc+4
        // Allocate space for the literal value
        allocateAndAddValue(operand.val); 
    
    } else if (operand.type == OperandType::REGISTER_IMMEDIATE) { // reg in reg ---> LD, LD_REG
        addInstruction("LD", "LD_REG", operand.val, gpr1.val, 0, 0); // ST gpr1, operand ---> LD gpr1, operand
    } else if (operand.type == OperandType::REGISTER_INDIRECT) { 
        addInstruction("ST", "ST_MEM", operand.val, 0, gpr1.val, 0); // ST gpr1, [reg]    
    } else if (operand.type == OperandType::REGISTER_INDIRECT_LITERAL) {
        if (operand.displacement > 0xFFF || operand.displacement < -0x800) {
            // Check if the displacement is within the range of 12 bits
            std::cerr << "Error: Displacement out of range." << std::endl;
            return;
        }
        addInstruction("ST", "ST_MEM", operand.val, 0, gpr1.val, operand.displacement); // ST gpr1, [reg + displacement]

    }
    // C NIVO !!!!!!!!!!! 

}

// ***** CONTROL AND STATUS REGISTER INSTRUCTIONS ****
void InstructionOperation::executeCsrRead() const { // LOAD DATA FROM CSR INTO GPR (i.e. read from CSR)
    // Implement CSRRD (Control and Status Register Read) instruction  
    //std::cout << "Executing CSRRD instruction." << std::endl;
    addInstruction("CSRRD", "CSRRD", gpr1.val, csr.val, 0 , 0);
    // A (left), B (right)
}
void InstructionOperation::executeCsrWrite() const { // WRITE DATA FROM GPR INTO CSR (i.e. write to CSR)
    // Implement CSRWR (Control and Status Register Write) instruction  
    //std::cout << "Executing CSRWR instruction." << std::endl;
    addInstruction("CSRWR", "CSRWR", csr.val, gpr1.val, 0 , 0);
}
// ***** EXCHANGE INSTRUCTION ****
void InstructionOperation::executeXCHG() const {
    // Implement XCHG (Exchange) 
    //std::cout << "Executing XCHG instruction." << std::endl;
    addInstruction("XCHG", "XCHG", 0, gpr2.val,  gpr1.val , 0);
}
// ***** ARITHMETIC/LOGIC/BITWISE INSTRUCTION ****
void InstructionOperation::executeArithmeticLogic() const {
    // Implement arithmetic and logic instructions (ADD, SUB, MUL, DIV, AND, OR, XOR, NOT, SHL, SHR) 
    //std::cout << "Executing Arithmetic/Logic instruction: " << instrName << std::endl;
    if (instrName == "NOT") {
        addInstruction(instrName, instrName, gpr1.val, gpr1.val, 0 , 0);
    } else {
        addInstruction(instrName, instrName, gpr2.val, gpr2.val,  gpr1.val , 0);
    }
}

// ***** BRANCH INSTRUCTIONS ****
void InstructionOperation::executeBranch() const {
    // Implement branch instructions (BEQ, BNE, BGT) 
    //std::cout << "Executing Branch instruction: " << instrName << std::endl;
    auto &assembler = Assembler::getInstance();
    auto *currentSection = assembler.getCurrentSection();

    if (operand.type == OperandType::IMMEDIATE_IDENT) {
        auto &symbolTable = assembler.getSymbolTable();
        auto s = symbolTable.find(operand.symbol);
        // -4 ?????
        // Symbol is not defined or not in the same section => make a forward reference
        if (s == symbolTable.end() || !s->second.defined 
            || symbolTable[operand.symbol].ndx != currentSection->ndx) {

            currentSection->forwardRefs[operand.symbol].addBackpatchOffset(currentSection->locCounter + 8); // ?? +8
        }
        else {
            // Make relocation entry for the symbol
            currentSection->relocations.push_back(Relocation(
                operand.symbol,                         // Symbol
                currentSection->locCounter + 8,         // Offset
                RelocType::R_X86_64_32,                 // Type
                (symbolTable[operand.symbol].bind == BIND::GLOB || symbolTable[operand.symbol].bind == BIND::EXT) ? 
                0 : symbolTable[operand.symbol].value   // Addend
            ));
        } 
        // branch gpr1, gpr2, LITERAL -----> 
        addInstruction(instrName, instrName+"_IDENT", 15, gpr1.val, gpr2.val, 4); // beq [pc+4]
        addInstruction("JMP", "JMP_LITERAL", 15, 0, 0, 4); // jmp pc+4
        allocateAndAddValue(0);  // Placeholder for the symbol value 

    } else if (operand.type == OperandType::IMMEDIATE_LITERAL) {
            // branch gpr1, gpr2, LITERAL -----> 
            addInstruction(instrName, instrName+"_IDENT", 15, gpr1.val, gpr2.val, 4); // beq [pc+4]
            addInstruction("JMP", "JMP_LITERAL", 15, 0, 0, 4); // jmp pc+4
            // allocate space for the LITERAL value
            allocateAndAddValue(operand.val); 
    }
}
// ***** JUMP INSTRUCTION ****
void InstructionOperation::executeJump() const {
    // Implement JMP instruction
    //std::cout << "Executing JMP instruction." << std::endl;
    auto &assembler = Assembler::getInstance();
    auto *currentSection = assembler.getCurrentSection();

    if (operand.type == OperandType::IMMEDIATE_IDENT) {
        auto &symbolTable = assembler.getSymbolTable();
        auto s = symbolTable.find(operand.symbol);
        // -4 ?????
        // Symbol is not defined or not in the same section => make a forward reference
        if (s == symbolTable.end() || !s->second.defined 
            || symbolTable[operand.symbol].ndx != currentSection->ndx) {

            currentSection->forwardRefs[operand.symbol].addBackpatchOffset(currentSection->locCounter + 4); // ?? +4
        }
        else {
            // Make relocation entry for the symbol
            currentSection->relocations.push_back(Relocation(
                operand.symbol,                         // Symbol
                currentSection->locCounter + 4,         // Offset
                RelocType::R_X86_64_32,                 // Type
                (symbolTable[operand.symbol].bind == BIND::GLOB || symbolTable[operand.symbol].bind == BIND::EXT) ? 
                0 : symbolTable[operand.symbol].value   // Addend
            ));
        } 
        // jmp [pc]
        addInstruction("JMP", "JMP_IDENT", 15, 0, 0, 0); 
        allocateAndAddValue(0);  // Placeholder for the symbol value 

    } else if (operand.type == OperandType::IMMEDIATE_LITERAL) {
            // JMP LITERAL -----> PUSH PC; PC<=mem[PC]
            addInstruction("JMP", "JMP_IDENT", 15, 0, 0, 0); // jmp [pc]
            // allocate space for the LITERAL value
            allocateAndAddValue(operand.val); 
    }

}
// ***** CALL INSTRUCTION ****
void InstructionOperation::executeCall() const {
    // Implement CALL instruction 
    //std::cout << "Executing CALL instruction." << std::endl;
    auto &assembler = Assembler::getInstance();
    auto *currentSection = assembler.getCurrentSection();

    if (operand.type == OperandType::IMMEDIATE_IDENT) {
        auto &symbolTable = assembler.getSymbolTable();
        auto s = symbolTable.find(operand.symbol);
        // -4 ?????
        // Symbol is not defined or not in the same section => make a forward reference
        if (s == symbolTable.end() || !s->second.defined 
            || symbolTable[operand.symbol].ndx != currentSection->ndx) {
            currentSection->forwardRefs[operand.symbol].addBackpatchOffset(currentSection->locCounter + 8); // ?? +8
        } 
        else {
            // Make relocation entry for the symbol
            currentSection->relocations.push_back(Relocation(
                operand.symbol,                         // Symbol
                currentSection->locCounter + 8,         // Offset
                RelocType::R_X86_64_32,                 // Type
                (symbolTable[operand.symbol].bind == BIND::GLOB || symbolTable[operand.symbol].bind == BIND::EXT) ? 
                0 : symbolTable[operand.symbol].value   // Addend
            ));
        }         
        // call [pc+4]
        addInstruction("CALL", "CALL_IDENT", 15, 0, 0, 4);
        // jmp pc+4
        addInstruction("JMP", "JMP_LITERAL", 15, 0, 0, 4); 
        allocateAndAddValue(0);  // Placeholder for the symbol value 

    } else if (operand.type == OperandType::IMMEDIATE_LITERAL) {
            // call [pc+4]
            addInstruction("CALL", "CALL_IDENT", 15, 0, 0, 4);
            // jmp pc+4
            addInstruction("JMP", "JMP_LITERAL", 15, 0, 0, 4); 
            // allocate space for the LITERAL value
            allocateAndAddValue(operand.val); 
    }

}



// ***** ALLOCATE AND ADD VALUE ****
// This function is used to allocate space for a value in the machine code and add it to the current section.
void InstructionOperation::allocateAndAddValue(uint32_t value) const {
    auto &assembler = Assembler::getInstance();
    auto *currentSection = assembler.getCurrentSection();

    // Split the 32-bit value into 4 bytes (little-endian order)
    uint8_t byte1 = value & 0xFF;         // Least significant byte
    uint8_t byte2 = (value >> 8) & 0xFF;  // Second byte
    uint8_t byte3 = (value >> 16) & 0xFF; // Third byte
    uint8_t byte4 = (value >> 24) & 0xFF; // Most significant byte

    // Add the bytes to the machine code
    currentSection->machineCode.push_back(byte1);
    currentSection->machineCode.push_back(byte2);
    currentSection->machineCode.push_back(byte3);
    currentSection->machineCode.push_back(byte4);

    // Update the location counter by 4 bytes
    currentSection->updateLocCounter(4);
}
// Komentar sa predavanja za pcrel adresiranje:
/*
Neka je data instrukcija: jmp A;
gde je A globalni simbol koji je definisan u nekom drugom fajlu, ali u okviru iste sekcije!
U trenutku asembliranja, simbol A nije definisan, pa se dodaje fwd referenca, a nakon nje se dodaje i relokacioni zapis.
U procesu linkovanja, linker ce zakljuciti da je A definisan u istoj sekciji i da se radi pcrel adresiranje, pa mu taj relokacioni zapis nije potreban.
*/

/*
IPAK NE KORISTIM PCREL
*/