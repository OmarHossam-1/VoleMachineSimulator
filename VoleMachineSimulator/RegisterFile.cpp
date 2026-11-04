#include "RegisterFile.h"

RegisterFile::RegisterFile() {
    clear();
}

unsigned char RegisterFile::getRegister(int address) {
    if (address >= 0 && address < 16) {
        return registers[address];
    }
    return 0;
}

void RegisterFile::setRegister(int address, unsigned char value) {
    if (address >= 0 && address < 16) {
        registers[address] = value;
    }
}

void RegisterFile::clear() {
    for (int i = 0; i < 16; i++) {
        registers[i] = 0;
    }
}