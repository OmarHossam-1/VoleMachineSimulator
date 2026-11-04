#pragma once

#include <string>
#include <cstdint>

class Memory {
private:
    unsigned char memory[256];

public:
    Memory();
    unsigned char getCell(int address);
    void setCell(int address, unsigned char value);
    void clear();
    bool loadProgramFromFile(std::string filename, uint8_t startAddress = 0);
};