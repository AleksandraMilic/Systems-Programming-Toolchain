#ifndef LABEL_OPERATION_HPP
#define LABEL_OPERATION_HPP

#include "Operation.hpp"
#include <string>

class LabelOperation : public Operation {
public:
    LabelOperation(const std::string &lab);//, uint32_t addr);
    virtual ~LabelOperation() = default;
    virtual void print() const override;
    void execute() const override;

private:
    std::string label;
    // uint32_t address; // current location counter stored here
};

#endif // LABEL_OPERATION_HPP
