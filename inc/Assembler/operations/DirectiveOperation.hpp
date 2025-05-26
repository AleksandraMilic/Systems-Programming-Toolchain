#ifndef DIRECTIVE_OPERATION_HPP
#define DIRECTIVE_OPERATION_HPP

#include "Operation.hpp"
// #include "../structures/helper_structures.hpp"
#include <vector>
#include <string>
#include <optional>  // For C++17 std::optional

class DirectiveOperation : public Operation {
public:
    std::string directive;

    std::vector<std::string> idList;           // For directives: GLOBAL, EXTERN
    std::vector<IdentOrLiteral> idLiteralList;   // For directive: WORD
    std::string symbol;                          // For directive: SECTION
    std::optional<long> literal;                 // For directives: SKIP, ASCII

    // Constructor for directives with no arguments (e.g. .end)
    DirectiveOperation(const std::string &d)
    : directive(d), literal(std::nullopt) {}

    // Constructor for GLOBAL and EXTERN directives (identifier list)
    DirectiveOperation(const std::string &d, const std::vector<std::string> &ids)
    : directive(d), idList(ids), literal(std::nullopt) {}

    // Constructor for SECTION and ASCII?? directives directive 
    DirectiveOperation(const std::string &d, const std::string &sym)
    : directive(d), symbol(sym), literal(std::nullopt) {}

    // Constructor for WORD directive (list of IdentOrLiteral)
    DirectiveOperation(const std::string &d, const std::vector<IdentOrLiteral> &list)
    : directive(d), idLiteralList(list), literal(std::nullopt) {}

    // Constructor for SKIP  (literal value)
    DirectiveOperation(const std::string &d, long lit)
    : directive(d), literal(lit) {}

    // void execute() const override {
    //     /* TO DO */
    // }
    
    // Declaration of print() that will be defined in DirectiveOperation.cpp
    void print() const override;
    void execute() const override;

    void global_execute() const; // Declaration of the global_execute method
    void extern_execute() const; // Declaration of the global_execute method
    void section_execute() const; // Declaration of the global_execute method
    void word_execute() const; // Declaration of the global_execute method
    void skip_execute() const; // Declaration of the global_execute method
    void end_execute() const; // Declaration of the global_execute method
    void ascii_execute() const; // Declaration of the global_execute method
    
};

#endif // DIRECTIVE_OPERATION_HPP
