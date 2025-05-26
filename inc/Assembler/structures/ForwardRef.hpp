#ifndef FORWARD_REF_HPP
#define FORWARD_REF_HPP

#include <string>
#include <cstdint>

struct ForwardRef {
    std::vector<uint32_t> patchOffsets; // List of offsets in the section's machine code where the symbol needs to be patched

    void addBackpatchOffset(uint32_t offset) {
        patchOffsets.push_back(offset);
    }
};
#endif // FORWARD_REF_HPP