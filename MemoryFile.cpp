#include "Memory.h"
#include <fstream>
#include <string>

Memory::Memory() {
    clear();
}

unsigned char Memory::getCell(int address) {
    if (address >= 0 && address < 256) {
        return memory[address];
    }
    return 0;
}

void Memory::setCell(int address, unsigned char value) {
    if (address >= 0 && address < 256) {
        memory[address] = value;
    }
}

void Memory::clear() {
    for (int i = 0; i < 256; i++) {
        memory[i] = 0;
    }
}

bool Memory::loadProgramFromFile(std::string filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    std::string line;
    int index = 0;

    while (file >> line && index < 256) {
        if (line.length() == 4) {
            std::string part1 = line.substr(0, 2);
            std::string part2 = line.substr(2, 2);

            unsigned char byte1 = (unsigned char)std::stoi(part1, nullptr, 16);
            unsigned char byte2 = (unsigned char)std::stoi(part2, nullptr, 16);

            memory[index] = byte1;
            memory[index + 1] = byte2;

            index += 2;
        }
    }

    file.close();
    return true;
}