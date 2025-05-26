#include "../../inc/Linker/Linker.hpp"
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <algorithm>

uint32_t Linker::section_idx = 0; // Define and initialize section_idx
uint32_t Linker::symbol_idx = 0;  // Define and initialize symbol_idx

/* TO DO:
- dodavanje nula
- patching
--- zasto ne moze sym.val u hex?

*/



Linker::Linker() : outputFileName("output.o") {}
// memory management - destructor
Linker::~Linker() {
    cleanup();
}
void Linker::cleanup() {
    // Clean up sections
    for (auto& [name, section] : sections) {
        delete section;
    }
    sections.clear();

    // Clean up ObjFiles
    for (auto& [fileName, objFile] : inputFilesMap) {
        // Clean up sections inside ObjFiles
        for (auto& [sectionName, section] : objFile->sections) {
            delete section;
        }
        delete objFile; // Free the ObjFiles object
    }
    inputFilesMap.clear();
}

void Linker::processCommandLineArgs(int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-o") {
            if (i + 1 < argc) {
                outputFileName = argv[++i];
            } else {
                throw std::invalid_argument("Missing output file name after -o");
            }
        } else if (arg.find("-place=") == 0) {
            std::string placement = arg.substr(7);
            size_t atPos = placement.find('@');
            if (atPos == std::string::npos) {
                throw std::invalid_argument("Invalid -place format. Expected <section_name><address>");
            }
            std::string sectionName = placement.substr(0, atPos);
            uint32_t address = std::stoul(placement.substr(atPos + 1), nullptr, 0);
            sectionPlacement[sectionName] = address;
        } else if (arg == "-hex") {
            generateHex = true;
        } else if (arg == "-relocatable") {
            generateRelocatable = true;
        } else {
            inputFiles.push_back(arg);
        }
    }

    if (generateHex == generateRelocatable) {
        throw std::invalid_argument("Exactly one of -hex or -relocatable must be specified.");
    }
}

void Linker::linking() {
    parseInput();

    mapSections();
    // print start address of the sections  
    symbolDetermination();
    // printSymbolTable();
    generateOutput();
    
    // printSectionTable();
    // printRelocationTable();
    // printMachineCode();
    //    printObjectFile();


}

// FUNCTIONS FOR LINKING PROCESS
void Linker::parseInput() {
    for (const auto& fileName : inputFiles) {
        std::ifstream input(fileName);
        if (!input.is_open()) {
            throw std::runtime_error("Error opening input file: " + fileName);
        }

        // std::cout << "Parsing file: " << fileName << std::endl;

        // Create a new ObjFiles structure for this input file
        ObjFiles* objFile = new ObjFiles();

        std::string line;
        std::string currentSection;

        while (std::getline(input, line)) {
            // Skip empty lines
            if (line.empty()) continue;

            // Parse the symbol table
            if (line == "#.symtab") {
                // std::cout << "Parsing symbol table..." << std::endl;
                std::getline(input, line); // Skip the header line
                while (std::getline(input, line) && !line.empty() && line != "#end") {
                    std::istringstream iss(line);
                    Symbol symbol;
                    std::string name;
                    iss >> symbol.idx  >> symbol.value;
                    std::string type, bind;
                    uint32_t ndx;
                    iss >> type >> bind >> ndx >> name;

                    symbol.type = (type == "SCTN") ? SymbolType::SCTN : SymbolType::NOTYP;
                    symbol.bind = (bind == "LOC") ? BIND::LOC : (bind == "GLOB") ? BIND::GLOB : BIND::EXT;
                    symbol.ndx = ndx;
                    symbol.defined = (symbol.ndx != -1);
                    symbol.name = name;

                    // Add symbol to the ObjFiles structure
                    objFile->symbols[name] = symbol;
                }
            }
            // Parse the section table
            else if (line == "#.sectab") {
                // std::cout << "Parsing section table..." << std::endl;
                std::getline(input, line); // Skip the header line
                while (std::getline(input, line) && !line.empty() && line != "#end") {
                    std::istringstream iss(line);
                    std::string sectionName;
                    uint32_t startAddress, size;
                    iss >> sectionName >> startAddress >> std::hex >> size;

                    // Add section to the ObjFiles structure
                    Section* section = new Section;
                    section->name = sectionName;
                    section->startAddress = startAddress;   
                    section->size = size;
                    objFile->sections[sectionName] = section;
                }
            }
            // Parse relocation tables
            else if (line.find("#.rela.") == 0) {
                currentSection = line.substr(7); // Extract section name
                // std::cout << "Parsing relocation table for section: " << currentSection << std::endl;
                //********************** */
                if (std::find(sectionOrder.begin(), sectionOrder.end(), currentSection) == sectionOrder.end()) {
                    sectionOrder.push_back(currentSection); // Store the order of sections
                }


                std::getline(input, line); // Skip the header line
                while (std::getline(input, line) && !line.empty() && line != "#end") {
                    std::istringstream iss(line);
                    std::string symbolName;
                    std::string typeStr;
                    uint32_t offset;
                    uint32_t addend;
                    iss >> std::hex >> offset >> typeStr >> symbolName >> std::dec >> addend;

                    Relocation relocation = {symbolName, offset, RelocType::R_X86_64_32, addend};

                    // Add relocation to the ObjFiles structure
                    objFile->sections[currentSection]->relocations.push_back(relocation);
                }
            }
            // Parse machine code for sections
            else if (line.find("#.machineCode.") == 0) {
                currentSection = line.substr(14); // Extract section name
                // std::cout << "Parsing machine code for section: " << currentSection << " from file: " << fileName << std::endl;

                while (std::getline(input, line) && !line.empty() && line != "#end") {
                    std::istringstream iss(line);
                    uint32_t offset;
                    iss >> std::hex >> offset; // Read offset (not used)
                    std::string byte;
                    while (iss >> byte) {
                        // Store machine code in the ObjFiles structure
                        objFile->sections[currentSection]->machineCode.push_back(static_cast<uint8_t>(std::stoi(byte, nullptr, 16)));
                    }
                }
            }
        }

        // Add the ObjFiles structure to the inputFiles map
        inputFilesMap[fileName] = objFile;

        input.close();
    }
}

void Linker::mapSections() {
    /* Method for mapping sections
    and appending relocations and machine code to the global sections map.

    DISCLAIMER: DO NOT ADD SYMBOLS TO GLOBAL SYMBOLS MAP IN THIS METHOD!!!*/

    for (string objName : inputFiles) {
        for (const auto& [sectionName, section] : inputFilesMap[objName]->sections) {
            
            // if the section isnt defined in the global sections map, map the section to the global sections map
            if (!sections.count(sectionName)) {
                sections[sectionName] = new Section{sectionName, 0, Linker::section_idx++};
                // Set the start address for the section
                // if (sectionPlacement.count(sectionName)) { // if section placement is specified in command line
                //     sections[sectionName]->startAddress = sectionPlacement[sectionName];
                // } //else {
                //     sections[sectionName]->startAddress = getNextAvailableAddress();
                //     cout << "Section " << sectionName << " start address: " << std::hex << sections[sectionName]->startAddress << std::endl;    
                // }
                // add section to global symbol table
                globalSymbols[sectionName] = Symbol(sectionName, Linker::symbol_idx++, sections[sectionName]->startAddress, false, false, true, BIND::LOC, SymbolType::SCTN, sections[sectionName]->ndx);

                // Append relocations to the section - no corrections needed
                for (const auto& relocation : section->relocations) {
                    sections[sectionName]->relocations.push_back(relocation);
                } 
            }
            else {
                // add previous size of the section to the offset; 
                // append need to be updated, bcs we updated value of the symbol (append ~ value)
                for (const auto& relocation : section->relocations) {
                    bool isGlobal = inputFilesMap[objName]->symbols[relocation.symbol].bind == BIND::GLOB || inputFilesMap[objName]->symbols[relocation.symbol].bind == BIND::EXT;
                    sections[sectionName]->relocations.push_back({relocation.symbol, relocation.offset + sections[sectionName]->size, relocation.type, 
                                                                    isGlobal ? 0 : relocation.addend+sections[sectionName]->size});
                    } 
            // update size and append machine code to the section
            }
            sections[sectionName]->size += section->size;
            sections[sectionName]->appendData(section->machineCode);
            // cout << "Section " << sectionName << " size: " << std::hex << sections[sectionName]->size << std::endl;
        }
    }
}

void Linker::symbolDetermination() {
    /* Method determines the values ​​of all symbols, according to the previously formed content map
    to generate a symbol table */
    
    // init to zero??
    std::unordered_map<std::string, std::uint32_t> definedSections; // Set to keep track of sections and its current size
    std::set<std::string> definedSymbols;


    for (string objName : inputFiles) {
        for (const auto& [sectionName, section] : inputFilesMap[objName]->sections) {
            // cout << "Processing section: " << sectionName << endl;
            for (const auto& [symbolName, symbol] : inputFilesMap[objName]->symbols) {
                // skip sections
                if (symbol.type == SymbolType::SCTN) {
                    continue;
                }
                if (symbol.ndx != inputFilesMap[objName]->symbols[sectionName].ndx) {
                    // cout<< "skip symbol: " << symbolName << endl;
                    continue; // skip if symbol isnt in the same section
                }
                if (symbol.bind == BIND::EXT) {
                    // cout<< "skip external symbol: " << symbolName << endl;
                    continue; // skip extern symbols --- DISCLAIMER: DONT PUT THEM IN THE SYMBOL TABLE
                }
                // update ndx because of the sections' mapping 
                uint32_t ndx = sections[sectionName]->ndx;
                uint32_t value = symbol.value; 
                
                // **** DETERMINE LOCAL SYMBOLS ***
                if (definedSections.count(sectionName)) { // more occurrences of the current section => need to update the value of the symbol
                    value += definedSections[sectionName];
                }

                // **** DETERMINE GLOBAL SYMBOLS ***
                if (definedSymbols.count(symbolName)) {
                    // cout<< "defining global symbol: " << symbolName << endl;
                    if (symbol.bind == BIND::GLOB && globalSymbols.find(symbolName) != globalSymbols.end() 
                        && globalSymbols[symbolName].bind == BIND::GLOB) {
                            // cout<< "duplicate symbol: " << symbolName << endl;
                        throw std::runtime_error("Multiple definitions of symbol: " + symbolName);
                    }
                }

                definedSymbols.insert(symbolName);  // add symbol to the set of defined symbols
                // cout<< "defining symbol: " << symbolName << endl;
                globalSymbols[symbolName] = Symbol(symbolName, Linker::symbol_idx++, value, symbol.isGlobal, symbol.isExtern, true, symbol.bind, symbol.type, ndx);
                   
            } definedSections[sectionName] += section->size; // update the size of the section    
        }
    }
}


// Generate global machine code by combining all sections
void Linker::generateGlobalMachineCode() {
    // std::cout << "Generating global machine code..." << std::endl;
    // assign values from sectionPlacement
    uint32_t maxAddress = 0;
    uint32_t startOffsetNextSection = 0;
    for (const auto& [sectionName, address] : sectionPlacement) {
        if (sections.count(sectionName)) {
            sections[sectionName]->startAddress = address;
            if (maxAddress < address) {
                maxAddress = address;
                startOffsetNextSection = sections[sectionName]->size;
            }
        } else {
            throw std::runtime_error("Section " + sectionName + " not found in sections map.");
        }
    }
    // initialize start address of the sections that are not in the sectionPlacement
    if (!sectionPlacement.empty()) {
        maxAddress += startOffsetNextSection; // add size to the address ---> ready for the next section
    }
    for (const std::string& sectionName : sectionOrder) {
        if (sectionPlacement.count(sectionName)) {
            continue; // Skip sections already in sectionPlacement
        }
        sections[sectionName]->startAddress = maxAddress;
        maxAddress += sections[sectionName]->size;
        // std::cout << "Section: " << sectionName << ", Start Address: " << std::setw(8) << std::setfill('0') << std::right << std::hex << sections[sectionName]->startAddress 
        //             << ", Size: " << std::setw(8) << std::setfill('0') << std::right << std::hex << sections[sectionName]->size << std::endl;
        }
    

    // Sort sections by their start addresses
    std::vector<std::pair<std::string, Section*>> sortedSections(sections.begin(), sections.end());
    std::sort(sortedSections.begin(), sortedSections.end(), [](const auto& a, const auto& b) {
        return a.second->startAddress < b.second->startAddress;
    });

    uint32_t currentAddress = 0;

    for (const auto& [sectionName, section] : sortedSections) {
        // Fill gaps between sections with zeros
        if (section->startAddress > currentAddress) {
            uint32_t gapSize = section->startAddress - currentAddress;
            globalMachineCode.insert(globalMachineCode.end(), gapSize, 0);
            currentAddress += gapSize;
        }

        // Append the machine code of the current section
        globalMachineCode.insert(globalMachineCode.end(), section->machineCode.begin(), section->machineCode.end());
        currentAddress += section->machineCode.size();
    }

    // std::cout << "Global machine code generated successfully." << std::endl;
}



void Linker::resolveReloc() {
    // generate global machine code
    generateGlobalMachineCode(); 

    // Update the symbol values with the start address of the sections
    for (auto& [name, symbol] : globalSymbols) {
        uint32_t ndx = symbol.ndx;
        // get the section name from the symbol 
        std::string sectionName = std::find_if(sections.begin(), sections.end(), 
            [ndx](const auto& pair) { return pair.second->ndx == ndx; })->first;
        symbol.value += sections[sectionName]->startAddress;
        
    }

    // Update the machine code with the relocation entries
    for (const auto& [sectionName, section] : sections) {
        // std::cout << "Resolving relocations for section: " << sectionName << std::endl;
        for (const auto& relocation : section->relocations) {
            if (relocation.type == RelocType::R_X86_64_32) {
                uint32_t offset = section->startAddress + relocation.offset;
                uint32_t symbolValue = globalSymbols[relocation.symbol].value;

                // Validate the offset
                if (offset + 3 >= globalMachineCode.size()) {
                    throw std::runtime_error("Error: Relocation offset out of bounds for global machine code.");
                }

                // Update the machine code at the specified offset with the symbol value
                // std::cout << "Updating machine code at offset: " << std::hex << offset
                //           << " with symbol value: " << std::hex << symbolValue << std::endl;

                globalMachineCode[offset] = static_cast<uint8_t>(symbolValue & 0xFF);
                globalMachineCode[offset + 1] = static_cast<uint8_t>((symbolValue >> 8) & 0xFF);
                globalMachineCode[offset + 2] = static_cast<uint8_t>((symbolValue >> 16) & 0xFF);
                globalMachineCode[offset + 3] = static_cast<uint8_t>((symbolValue >> 24) & 0xFF);
            } else {
                throw std::runtime_error("Unsupported relocation type: " + std::to_string(static_cast<int>(relocation.type)));
            }
        }
    }
}


// CHECK FOR ERRORS
void Linker::checkForUnresolvedSymbols() {
    // Check for unresolved/undefined symbols
    for (const auto& [name, symbol] : globalSymbols) {
        if (!symbol.defined) {
            throw std::runtime_error("Unresolved symbol: " + name);
        }
    }
}
void Linker::checkForOverlappingSections() {
    // Check for overlapping sections using range for each of sections: [startAddress, startAddress + size]
    std::vector<std::pair<uint32_t, uint32_t>> sectionRanges;
    for (const auto& [name, section] : sections) {
        sectionRanges.emplace_back(section->startAddress, section->startAddress + section->machineCode.size());
    }
    
    // iterate through all pairs of sections and check for overlaps
    for (size_t i = 0; i < sectionRanges.size(); ++i) {
        for (size_t j = i + 1; j < sectionRanges.size(); ++j) {
            if (sectionRanges[i].second > sectionRanges[j].first && sectionRanges[i].first < sectionRanges[j].second) {
                throw std::runtime_error("Overlapping sections detected.");
            }
        }
    }
}


// GET OUTPUT
void Linker::generateOutput() {
    std::ofstream output(outputFileName);
    if (!output.is_open()) {
        throw std::runtime_error("Error opening output file: " + outputFileName);
    }
    // I have checked during linking that there is no multiple definition of symbols

    if (generateHex) {
        resolveReloc();
        // printMachineCode();
        cout << "Relocation resolved." << endl;
        checkForUnresolvedSymbols();
        cout << "Unresolved symbols checked." << endl;
        checkForOverlappingSections(); // check for overlapping sections
        cout << "overlapping sections checked" << endl;
        writeHexOutput(output);
    } else if (generateRelocatable) {
        writeRelocatableOutput(output);
        cout << "Relocatable output generated." << endl;
    }
    output.close();
}

void Linker::writeHexOutput(std::ofstream& output) {
    output << "# Hex Output" << std::endl;
    
    // sort sectionOrder by start address
    std::sort(sectionOrder.begin(), sectionOrder.end(), [this](const std::string& a, const std::string& b) {
        return sections[a]->startAddress < sections[b]->startAddress;
    });

    for (const std::string& sectionName : sectionOrder) {
        Section* section = sections[sectionName];
        uint32_t addr = section->startAddress;
        size_t size = section->machineCode.size();

        // The offset in globalMachineCode where this section starts
        size_t globalOffset = addr;

        for (size_t i = 0; i < size; i += 16) {
            output << std::setw(4) << std::setfill('0') << std::hex << (addr + static_cast<uint32_t>(i)) << ": ";
            for (size_t j = 0; j < 16 && i + j < size; ++j) {
                output << std::setw(2) << std::setfill('0') << std::hex
                       << static_cast<int>(globalMachineCode[globalOffset + i + j]) << " ";
            }
            output << std::endl;
        }
    }
}

void Linker::writeRelocatableOutput(std::ofstream& output) {
    // output << "# Relocatable Output" << std::endl;

    // Write symbol table
    output << "#.symtab" << std::endl;
    output << "Idx Value     Type    Bind   Ndx Name" << std::endl; // Removed "Size"
    // Sort symbols by their index for consistent output
    std::vector<std::pair<std::string, Symbol>> sortedSymbols(globalSymbols.begin(), globalSymbols.end());
    std::sort(sortedSymbols.begin(), sortedSymbols.end(), [](const auto& a, const auto& b) {
        return a.second.idx < b.second.idx;
    });
    for (const auto& [name, symbol] : sortedSymbols) {
        output << std::setw(3) << symbol.idx << " "
               << std::setw(8) << std::setfill('0') << std::right <<  symbol.value << " "
               << (symbol.type == SymbolType::SCTN ? "SCTN" : "NOTYP") << " "
               << (symbol.bind == BIND::LOC ? "LOC" : symbol.bind == BIND::GLOB ? "GLOB" : "EXT") << " "
               << (std::to_string(symbol.ndx)) << " " // UND???
               << name << std::endl;
    }
    output << "#end" << std::endl;

    // Write the section table
    output << "#.sectab" << std::endl;
    output << "Name   StartAddr   Size" << std::endl; 

    for (const auto& [sectionName, section] : sections) {
        output << std::left << sectionName << " "
               << std::setw(8) << std::setfill('0') << section->startAddress << " "
               << std::setw(8) << std::setfill('0') << std::right << std::hex <<  section->machineCode.size() << std::endl;
    }
    output << "#end" << std::endl;


    // Write the relocation tables for each section
    for (const auto& [sectionName, section] : sections) {
        output << "#.rela." << sectionName << std::endl;
        output << "Offset     Type           Symbol Addend" << std::endl;

        // Sort relocations by offset for consistent output
        std::vector<Relocation> sortedRelocations = section->relocations;
        std::sort(sortedRelocations.begin(), sortedRelocations.end(), [](const Relocation& a, const Relocation& b) {
            return a.offset < b.offset;
        });

        for (const auto& relocation : sortedRelocations) {
            output << std::setw(8) << std::setfill('0') << std::right <<  std::hex << relocation.offset << " "
                   << std::setw(14) << relocation.type << " "
                   << relocation.symbol << " "
                   << std::dec << relocation.addend << std::endl;
        }
        output << "#end" << std::endl;

    }

    // Write the machine code for each section
    for (const auto& [sectionName, section] : sections) {
        output << "#.machineCode." << sectionName << std::endl;

        for (size_t i = 0; i < section->machineCode.size(); i++) {
            if (i % 16 == 0) {
                if (i > 0) output << std::endl;
                output << std::setw(8) << std::setfill('0') << std::right << std::hex << i << " ";
            }
            output << std::setw(2) << std::setfill('0') << std::hex
                   << static_cast<int>(section->machineCode[i]) << " ";
        }
        output << std::endl << "#end" << std::endl;
    }

    std::cout << "Output written successfully to the file." << std::endl;
}


// ********** print functions ********** //
void Linker::printSymbolTable() const {
    std::cout << "#.symtab" << std::endl;
    std::cout << "Idx Value     Type    Bind   Ndx Name" << std::endl; // Removed "Size"

    // Create a vector of symbols and sort by idx
    std::vector<std::pair<std::string, Symbol>> sortedSymbols(globalSymbols.begin(), globalSymbols.end());
    std::sort(sortedSymbols.begin(), sortedSymbols.end(), [](const auto& a, const auto& b) {
        return a.second.idx < b.second.idx;
    });

    for (const auto& [name, symbol] : sortedSymbols) {
        std::cout << std::setw(3) << std::to_string(symbol.idx) << " " // Print symbol.idx in ascending order
                  << std::setw(8) << std::setfill('0') << std::hex << symbol.value << " "
                  << (symbol.type == SymbolType::SCTN ? "SCTN" : "NOTYP") << " " // Removed std::setw and std::setfill
                  << (symbol.bind == BIND::LOC ? "LOC" : symbol.bind == BIND::GLOB ? "GLOB" : "EXT") << " " // Removed std::setw and std::setfill
                  << std::setw(3) << (symbol.ndx == -1 ? "UND" : std::to_string(symbol.ndx)) << " "
                  << symbol.name << std::endl;
    }
}

void Linker::printSectionTable() const {
    std::cout << "#.sectab" << std::endl;
    std::cout << "Name   StartAddr   Size" << std::endl;

    for (const auto& [sectionName, section] : sections) {
        std::cout << std::left << sectionName << " "
                  << std::setw(8) << std::setfill('0') <<std::right << std::hex << section->startAddress << " "
                  << std::setw(8) << std::setfill('0') <<std::right << section->machineCode.size() << std::endl;
    }
}

void Linker::printRelocationTable() const {
    for (const auto& [sectionName, section] : sections) {
        std::cout << "#.rela." << sectionName << std::endl;
        std::cout << "Offset     Type           Symbol Addend" << std::endl;

        // Sort relocations by offset in ascending order
        std::vector<Relocation> sortedRelocations = section->relocations;
        std::sort(sortedRelocations.begin(), sortedRelocations.end(), [](const Relocation& a, const Relocation& b) {
            return a.offset < b.offset;
        });

        for (const auto& relocation : sortedRelocations) {
            std::cout << std::setw(8) << std::setfill('0') << std::right <<std::hex << relocation.offset << " "
                      << std::setw(14) << relocation.type << " "
                      << relocation.symbol << " "
                      << std::hex << relocation.addend << std::endl;
        }
    }
}

void Linker::printMachineCode() const {
    for (const auto& [sectionName, section] : sections) {
        std::cout << "#.machineCode." << sectionName << std::endl;

        for (size_t i = 0; i < section->machineCode.size(); i++) {
            if (i % 16 == 0) {
                if (i > 0) std::cout << std::endl;
                std::cout << std::setw(8) << std::setfill('0') << std::right << std::hex << i << " ";
            }
            std::cout << std::setw(2) << std::setfill('0') << std::hex
                      << static_cast<int>(section->machineCode[i]) << " ";
        }
        std::cout << std::endl << "#end" << std::endl;
    }
}

void Linker::printObjectFile() const {
    std::cout << "#.objfile" << std::endl;

    for (const auto& [fileName, objFile] : inputFilesMap) {
        std::cout << "File: " << fileName << std::endl;

        // Print symbol table
        std::cout << "#.symtab" << std::endl;
        std::cout << "Idx Value     Type    Bind   Ndx Name" << std::endl;
        for (const auto& [name, symbol] : objFile->symbols) {
            std::cout << std::setw(3) << std::setfill('0') << std::dec << symbol.idx << " "
                      << std::setw(8) << std::setfill('0') << std::hex << symbol.value << " "
                      << (symbol.type == SymbolType::SCTN ? "SCTN" : "NOTYP") << " "
                      << (symbol.bind == BIND::LOC ? "LOC" : symbol.bind == BIND::GLOB ? "GLOB" : "EXT") << " "
                      << std::setw(3) << (symbol.ndx == -1 ? "UND" : std::to_string(symbol.ndx)) << " "
                      << name << std::endl;
        }
        std::cout << "#end" << std::endl;

        // Print section table
        std::cout << "#.sectab" << std::endl;
        std::cout << "Name   StartAddr   Size" << std::endl;
        for (const auto& [sectionName, sec] : objFile->sections) {
            // Assuming `startAddress` and `size` are stored in the `ObjFiles` structure
            uint32_t startAddress = 0; // Replace with actual start address if available
            uint32_t size = 0;         // Replace with actual size if available
            std::cout << std::left << sectionName << " "
                      << std::setw(8) << std::setfill('0') << std::right << std::hex << sec->startAddress << " "
                      << std::setw(8) << std::setfill('0') << std::right << sec->size << std::endl;
                      
        }
        std::cout << "#end" << std::endl;

        // Print relocation tables
        for (const auto& [sectionName, section] : objFile->sections) {
            std::vector<Relocation> sortedRelocations = section->relocations;
            std::sort(sortedRelocations.begin(), sortedRelocations.end(), [](const Relocation& a, const Relocation& b) {
                return a.offset < b.offset;
            });
    
            for (const auto& relocation : sortedRelocations) {
                std::cout << std::setw(8) << std::setfill('0') << std::right <<std::hex << relocation.offset << " "
                          << std::setw(14) << relocation.type << " "
                          << relocation.symbol << " "
                          << std::hex << relocation.addend << std::endl;
            }
        }

        // Print machine code
        for (const auto& [sectionName, sec] : objFile->sections) {
            std::cout << "#.machineCode." << sectionName << std::endl;
            // Assuming machine code is stored in the `ObjFiles` structure
            for (size_t i = 0; i < sec->machineCode.size(); ++i) {
                if (i % 16 == 0) {
                    if (i > 0) std::cout << std::endl;
                    std::cout << std::setw(8) << std::setfill('0') << std::right << std::hex << i << " ";
                }
                std::cout << std::setw(2) << std::setfill('0') << std::hex
                          << static_cast<int>(sec->machineCode[i]) << " ";
            }
            std::cout << std::endl << "#end" << std::endl;
        }
    }

    std::cout << "#end" << std::endl;
}