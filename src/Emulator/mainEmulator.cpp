#include "../../inc/Emulator/Emulator.hpp"
#include <iostream>

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <input_filename>\n";
        return 1;
    }

    try {
        // Create an emulator instance with the input file
        Emulator emulator(argv[1]);
        // Emulator emulator("program.hex");

        emulator.loadMemory();
        emulator.printMemory();
        emulator.execute();
        emulator.printProcessorState();
        emulator.printMemory();
        

    } catch (const std::exception& e) {
        // Handle any errors during emulation
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}