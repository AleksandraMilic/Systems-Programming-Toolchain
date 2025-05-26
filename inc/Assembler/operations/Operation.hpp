#ifndef OPERATION_HPP
#define OPERATION_HPP

#include "../structures/helper_structures.hpp"
#include <string>

class Operation {
    /* Base class Operation from which we directly derive three subclass:
    DirectiveOperation, InstructionOperation, LabelOperation */
    
public:
    virtual ~Operation() = default;
    virtual void execute() const = 0;
    // virtual std::string toString() const = 0;
    virtual void print() const = 0;
};

#endif // OPERATION_HPP
