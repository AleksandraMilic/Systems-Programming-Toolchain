%{
#include "parser.hpp"
#include <cstdlib>
#include <cstring>
#include <string>
#include <stdio.h>
using namespace std;
extern int getNumberOFGPR(const char* str);
void yyerror(const char *msg);
%}

%option noyywrap
%option nounput

%%
".global"             { return GLOBAL; }
".extern"             { return EXTERN; }
".section"            { return SECTION; }
".word"               { return WORD; }
".skip"               { return SKIP; }
".end"                { return END; }
".ascii"              { return ASCII; }

"halt"                { return HALT; }
"int"                 { return INT; }
"iret"                { return IRET; }
"call"                { return CALL; }
"ret"                 { return RET; }
"jmp"                 { return JMP; }
"bne"                 { return BNE; }
"bgt"                 { return BGT; }
"beq"                 { return BEQ; }
"push"                { return PUSH; }
"pop"                 { return POP; }
"xchg"                { return XCHG; }
"add"                 { return ADD; }
"sub"                 { return SUB; }
"mul"                 { return MUL; }
"div"                 { return DIV; }
"and"                 { return AND; }
"or"                  { return OR; }
"xor"                 { return XOR; }
"not"                 { return NOT; }
"shl"                 { return SHL; }
"shr"                 { return SHR; }
"csrrd"               { return CSRRD; }
"csrwr"               { return CSRWR; }
"ld"                  { return LD; }
"st"                  { return ST; }

%r([0-9]|1[0-5])      { yylval.num = getNumberOFGPR(yytext); return REG; }
%pc                   { yylval.num = 15; return REG; }
%sp                   { yylval.num = 14; return REG; }
%status               { yylval.num = 0; return CSR; }
%handler              { yylval.num = 1; return CSR; }
%cause                { yylval.num = 2; return CSR; }

0[xX][0-9a-fA-F]+      { yylval.num = strtol(yytext + 2, NULL, 16); return LITERAL_HEXA; }
[0-9]|([1-9][0-9]*)    { yylval.num = strtol(yytext, NULL, 10); return LITERAL_DEC; }
[a-zA-Z_][a-zA-Z0-9_]* { yylval.str = strdup(yytext); return IDENT; }

","                   { return COMMA; }
":"                   { return COLON; }
"%"                   { return PERCENT; }
"$"                   { return DOLLAR; }
"["                   { return LBRACKET; }
"]"                   { return RBRACKET; }
\n                    { return EOL; }
"+"                   { return PLUS; }
"-"                   { return MINUS; }
"."                   { return DOT; }

[\r]*                ;    // Ignore carriage returns (Windows line endings)
[ \t]*               ;    // Ignore spaces and tabs
#.*\n                { return COMMENT; }
.                    { printf("Unexpected character: %s\n", yytext); }
%%

int getNumberOFGPR(const char* str) {
    if (str[0] == '%' && str[1] == 'r') { 
        int regNum = atoi(&str[2]);
        return regNum;
    }
    yyerror("Invalid register name");
    return -1;
}
