%{
#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <memory>           // for std::make_unique
#include "../inc/Assembler/structures/helper_structures.hpp"
#include "../inc/Assembler/Assembler.hpp"
using namespace std;

void yyerror(const char* s){ cout << s << endl; }
extern int yylex();
extern int yyparse();
extern FILE* yyin;

Operand ld_st_op = Operand(IMMEDIATE_LITERAL,0,0);
vector<std::string> idList;
vector<IdentOrLiteral> idLiteralList;


%}

%union {
    int32_t num;
    char* str;
}

%token GLOBAL EXTERN SECTION WORD SKIP END ASCII
%token HALT INT IRET CALL RET JMP BNE BGT BEQ PUSH POP XCHG ADD SUB MUL DIV AND OR XOR NOT SHL SHR CSRRD CSRWR LD ST
%token <num> REG
%token <num> LITERAL_HEXA LITERAL_DEC
%token <num> CSR      /* Declaration for CSR token */
%token <str> IDENT

%token COMMA COLON DOLLAR LBRACKET RBRACKET PLUS MINUS EOL DOT COMMENT PERCENT

%type <num> literal
%type <num> gpr
%type <str> id_list
%type <str> id_and_literal_list
%type <str> operand

%%

program:
    /* empty */
    | program line
    ;

line:
    instruction EOL
    | instruction COMMENT
    | directive EOL
    | directive COMMENT
    | COMMENT
    | EOL
    | label line
    | label EOL
    | label COMMENT

    ;
 
directive:
      GLOBAL id_list { 
          // //cout << "Parsed .global with symbols: " << $2 << endl; 
          Assembler::getInstance().addOperation(std::make_unique<DirectiveOperation>("GLOBAL", idList));
          idList.clear(); 
      }
    | EXTERN id_list { 
          // //cout << "Parsed .extern with symbols: " << $2 << endl; 
          Assembler::getInstance().addOperation(std::make_unique<DirectiveOperation>("EXTERN", idList));
          idList.clear(); 
      }
    | SECTION IDENT { 
          // //cout << "Parsed .section: " << $2 << endl; 
          Assembler::getInstance().addOperation(std::make_unique<DirectiveOperation>("SECTION", $2));
          free($2); 
      }
    | WORD id_and_literal_list { 
          // //cout << "Parsed .word with values: " << $2 << endl; 
          Assembler::getInstance().addOperation(std::make_unique<DirectiveOperation>("WORD", idLiteralList));
          idLiteralList.clear(); 
      }
    | SKIP literal { 
          // //cout << "Parsed .skip with literal value: " << $2 << endl; 
          Assembler::getInstance().addOperation(std::make_unique<DirectiveOperation>("SKIP", $2));
      }
    | END { 
          // //cout << "Parsed .end" << endl; 
          Assembler::getInstance().addOperation(std::make_unique<DirectiveOperation>("END", ""));
      }
    | ASCII IDENT { 
          // //cout << "Parsed .ascii with string: " << $2 << endl; 
          Assembler::getInstance().addOperation(std::make_unique<DirectiveOperation>("ASCII", $2));
          free($2); 
      }
    ;

instruction:
      HALT { ////cout << "Parsed halt instruction" << endl; 
                Assembler::getInstance().addOperation(std::make_unique<InstructionOperation>("HALT", std::vector<Operand>{}));}
    | INT  { ////cout << "Parsed int instruction" << endl; 
              Assembler::getInstance().addOperation(std::make_unique<InstructionOperation>("INT", std::vector<Operand>{}));}
    | IRET { ////cout << "Parsed iret instruction" << endl; 
              Assembler::getInstance().addOperation(std::make_unique<InstructionOperation>("IRET", std::vector<Operand>{}));}
    | RET  { ////cout << "Parsed ret instruction" << endl; 
              Assembler::getInstance().addOperation(std::make_unique<InstructionOperation>("RET", std::vector<Operand>{}));}

    | CALL IDENT { ////cout << "Parsed call instruction with ident: " << $2 << endl;           
          std::vector<Operand> operands = { Operand(OperandType::IMMEDIATE_IDENT, $2) };
          Assembler::getInstance().addOperation(std::make_unique<InstructionOperation>("CALL", operands)); free($2); }
    | CALL literal { ////cout << "Parsed call instruction with literal: " << $2 << endl;
          std::vector<Operand> operands = { Operand(OperandType::IMMEDIATE_LITERAL, $2) }; 
          Assembler::getInstance().addOperation(std::make_unique<InstructionOperation>("CALL", operands));}
    | JMP IDENT { ////cout << "Parsed jmp instruction with operand: " << $2 << endl; 
          std::vector<Operand> operands = { Operand(OperandType::IMMEDIATE_IDENT, $2) };
          Assembler::getInstance().addOperation(std::make_unique<InstructionOperation>("JMP", operands)); free($2); }
    | JMP literal { ////cout << "Parsed jmp instruction with operand: " << $2 << endl; 
          std::vector<Operand> operands = { Operand(OperandType::IMMEDIATE_LITERAL, $2) };
          Assembler::getInstance().addOperation(std::make_unique<InstructionOperation>("JMP", operands)); }
  
| BNE gpr COMMA gpr COMMA IDENT { ////cout << "Parsed bne instruction: " << $2 << ", " << $4 << ", " << $6 << endl; 
          std::vector<Operand> operands = { Operand(REGISTER_IMMEDIATE, $2), Operand(REGISTER_IMMEDIATE, $4), Operand(OperandType::IMMEDIATE_IDENT, $6) };
          Assembler::getInstance().addOperation(std::make_unique<InstructionOperation>("BNE", operands)); free($6); }
    | BGT gpr COMMA gpr COMMA IDENT { ////cout << "Parsed bgt instruction: " << $2 << ", " << $4 << ", " << $6 << endl; 
          std::vector<Operand> operands = { Operand(REGISTER_IMMEDIATE, $2), Operand(REGISTER_IMMEDIATE, $4), Operand(OperandType::IMMEDIATE_IDENT, $6) };
          Assembler::getInstance().addOperation(std::make_unique<InstructionOperation>("BGT", operands)); free($6); }
    | BEQ gpr COMMA gpr COMMA IDENT { ////cout << "Parsed beq instruction: " << $2 << ", " << $4 << ", " << $6 << endl; 
          std::vector<Operand> operands = { Operand(REGISTER_IMMEDIATE, $2), Operand(REGISTER_IMMEDIATE, $4), Operand(OperandType::IMMEDIATE_IDENT, $6) };
          Assembler::getInstance().addOperation(std::make_unique<InstructionOperation>("BEQ", operands)); free($6); }
    | BNE gpr COMMA gpr COMMA literal { ////cout << "Parsed bne instruction: " << $2 << ", " << $4 << ", " << $6 << endl; 
          std::vector<Operand> operands = { Operand(REGISTER_IMMEDIATE, $2), Operand(REGISTER_IMMEDIATE, $4), Operand(OperandType::IMMEDIATE_LITERAL, $6) };
          Assembler::getInstance().addOperation(std::make_unique<InstructionOperation>("BNE", operands)); }
    | BGT gpr COMMA gpr COMMA literal { ////cout << "Parsed bgt instruction: " << $2 << ", " << $4 << ", " << $6 << endl; 
          std::vector<Operand> operands = { Operand(REGISTER_IMMEDIATE, $2), Operand(REGISTER_IMMEDIATE, $4), Operand(OperandType::IMMEDIATE_LITERAL, $6) };
          Assembler::getInstance().addOperation(std::make_unique<InstructionOperation>("BGT", operands)); }
    | BEQ gpr COMMA gpr COMMA literal { ////cout << "Parsed beq instruction: " << $2 << ", " << $4 << ", " << $6 << endl; 
          std::vector<Operand> operands = { Operand(REGISTER_IMMEDIATE, $2), Operand(REGISTER_IMMEDIATE, $4), Operand(OperandType::IMMEDIATE_LITERAL, $6) };
          Assembler::getInstance().addOperation(std::make_unique<InstructionOperation>("BEQ", operands)); }

    | PUSH gpr { ////cout << "Parsed push instruction with register: " << $2 << endl; 
          std::vector<Operand> operands = { Operand(REGISTER_IMMEDIATE, $2) };
          Assembler::getInstance().addOperation(std::make_unique<InstructionOperation>("PUSH", operands)); }
    | POP gpr { //cout << "Parsed pop instruction with register: " << $2 << endl; 
          std::vector<Operand> operands = { Operand(REGISTER_IMMEDIATE, $2) };
          Assembler::getInstance().addOperation(std::make_unique<InstructionOperation>("POP", operands)); }

    | XCHG gpr COMMA gpr { //cout << "Parsed xchg instruction with registers: " << $2 << " and " << $4 << endl; 
          std::vector<Operand> operands = { Operand(REGISTER_IMMEDIATE, $2), Operand(REGISTER_IMMEDIATE, $4) };
          Assembler::getInstance().addOperation(std::make_unique<InstructionOperation>("XCHG", operands)); }
    | ADD gpr COMMA gpr  { //cout << "Parsed add instruction with registers: " << $2 << ", " << $4 << endl; 
          std::vector<Operand> operands = { Operand(REGISTER_IMMEDIATE, $2), Operand(REGISTER_IMMEDIATE, $4) };
          Assembler::getInstance().addOperation(std::make_unique<InstructionOperation>("ADD", operands)); }
    | SUB gpr COMMA gpr  { //cout << "Parsed sub instruction with registers: " << $2 << ", " << $4 << endl; 
          std::vector<Operand> operands = { Operand(REGISTER_IMMEDIATE, $2), Operand(REGISTER_IMMEDIATE, $4) };
          Assembler::getInstance().addOperation(std::make_unique<InstructionOperation>("SUB", operands)); }
    | MUL gpr COMMA gpr  { //cout << "Parsed mul instruction with registers: " << $2 << ", " << $4 << endl; 
          std::vector<Operand> operands = { Operand(REGISTER_IMMEDIATE, $2), Operand(REGISTER_IMMEDIATE, $4) };
          Assembler::getInstance().addOperation(std::make_unique<InstructionOperation>("MUL", operands)); }
    | DIV gpr COMMA gpr  { //cout << "Parsed div instruction with registers: " << $2 << ", " << $4 << endl; 
          std::vector<Operand> operands = { Operand(REGISTER_IMMEDIATE, $2), Operand(REGISTER_IMMEDIATE, $4) };
          Assembler::getInstance().addOperation(std::make_unique<InstructionOperation>("DIV", operands)); }
    | AND gpr COMMA gpr  { //cout << "Parsed and instruction with registers: " << $2 << ", " << $4 << endl; 
          std::vector<Operand> operands = { Operand(REGISTER_IMMEDIATE, $2), Operand(REGISTER_IMMEDIATE, $4) };
          Assembler::getInstance().addOperation(std::make_unique<InstructionOperation>("AND", operands)); }
    | OR gpr COMMA gpr   { //cout << "Parsed or instruction with registers: " << $2 << ", " << $4 << endl; 
          std::vector<Operand> operands = { Operand(REGISTER_IMMEDIATE, $2), Operand(REGISTER_IMMEDIATE, $4) };
          Assembler::getInstance().addOperation(std::make_unique<InstructionOperation>("OR", operands)); }
    | XOR gpr COMMA gpr  { //cout << "Parsed xor instruction with registers: " << $2 << ", " << $4 << endl; 
          std::vector<Operand> operands = { Operand(REGISTER_IMMEDIATE, $2), Operand(REGISTER_IMMEDIATE, $4) };
          Assembler::getInstance().addOperation(std::make_unique<InstructionOperation>("XOR", operands)); }
    | NOT gpr { //cout << "Parsed not instruction with register: " << $2 << endl; 
          std::vector<Operand> operands = { Operand(REGISTER_IMMEDIATE, $2) };
          Assembler::getInstance().addOperation(std::make_unique<InstructionOperation>("NOT", operands)); }
    | SHL gpr COMMA gpr  { //cout << "Parsed shl instruction with registers: " << $2 << ", " << $4 << endl; 
          std::vector<Operand> operands = { Operand(REGISTER_IMMEDIATE, $2), Operand(REGISTER_IMMEDIATE, $4) };
          Assembler::getInstance().addOperation(std::make_unique<InstructionOperation>("SHL", operands)); }
    | SHR gpr COMMA gpr  { //cout << "Parsed shr instruction with registers: " << $2 << ", " << $4 << endl; 
          std::vector<Operand> operands = { Operand(REGISTER_IMMEDIATE, $2), Operand(REGISTER_IMMEDIATE, $4) };
          Assembler::getInstance().addOperation(std::make_unique<InstructionOperation>("SHR", operands)); }

    | CSRRD CSR COMMA gpr { //cout << "Parsed csrrd instruction with register: " << $2 << ", " << $4 << endl; 
          std::vector<Operand> operands = { Operand(CSR_IMMEDIATE, $2), Operand(REGISTER_IMMEDIATE, $4) };
          Assembler::getInstance().addOperation(std::make_unique<InstructionOperation>("CSRRD", operands)); }
    | CSRWR gpr COMMA CSR { //cout << "Parsed csrwr instruction with register: " << $2 << ", " << $4 << endl; 
          std::vector<Operand> operands = { Operand(REGISTER_IMMEDIATE, $2), Operand(CSR_IMMEDIATE, $4) };
          Assembler::getInstance().addOperation(std::make_unique<InstructionOperation>("CSRWR", operands)); }

    | LD operand COMMA gpr { 
        // //cout << "Parsed ld instruction with operand: " << $2 << " and register: " << $4 << endl;
        std::vector<Operand> operands = {  ld_st_op, Operand(REGISTER_IMMEDIATE, $4) };
        Assembler::getInstance().addOperation(std::make_unique<InstructionOperation>("LD", operands)); }
    | ST gpr COMMA operand { 
        // //cout << "Parsed st instruction with register: " << $2 << " and operand: " << $4 << endl;
        std::vector<Operand> operands = {  Operand(REGISTER_IMMEDIATE, $2), ld_st_op  };
        Assembler::getInstance().addOperation(std::make_unique<InstructionOperation>("ST", operands)); }
    ;
    

label:
      IDENT COLON { 
          // //cout << "Parsed label: " << $1 << endl; 
          Assembler::getInstance().addOperation(std::make_unique<LabelOperation>($1));
          free($1); 
      }
    ;

id_list:
      IDENT { idList.push_back($1); }
    | id_list COMMA IDENT { idList.push_back($3); }
    ;

id_and_literal_list:
      IDENT { idLiteralList.push_back(IdentOrLiteral($1, 0, true)); }
    | literal { idLiteralList.push_back(IdentOrLiteral((char*)"", $1, false)); }
    | id_and_literal_list COMMA IDENT { idLiteralList.push_back(IdentOrLiteral($3, 0, true)); }
    | id_and_literal_list COMMA literal { idLiteralList.push_back(IdentOrLiteral((char*)"", $3, false)); }
    ;

operand:
  DOLLAR literal { 
      ld_st_op = Operand(IMMEDIATE_LITERAL, $2);
  }
  | DOLLAR IDENT { 
      ld_st_op = Operand(IMMEDIATE_IDENT, $2);
  }
  | literal { 
      ld_st_op = Operand(DIR_LITERAL, $1);
  }
  | IDENT { 
      ld_st_op = Operand(DIR_IDENT, $1); 
  }
  | gpr { 
      ld_st_op = Operand(REGISTER_IMMEDIATE, $1); 
  }
  | LBRACKET gpr RBRACKET { 
      ld_st_op = Operand(REGISTER_INDIRECT, $2); 
    }
  | LBRACKET gpr PLUS IDENT RBRACKET {          // OVO JE ZA C NIVO!!!!!! ld i st [reg + symbol]
      ld_st_op = Operand(REGISTER_INDIRECT_SYMBOL, $2, $4);
    }
  | LBRACKET gpr PLUS literal RBRACKET { 
      ld_st_op = Operand(REGISTER_INDIRECT_LITERAL, $2, $4);
    }
  ;

gpr:
    REG { $$ = $1; }
    ;

literal:
    LITERAL_HEXA { $$ = $1; }
  | LITERAL_DEC  { $$ = $1; }
  ;

%%

