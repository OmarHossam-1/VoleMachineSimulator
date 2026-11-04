#include <string>
#include <cstdio>
#include "VoleMachine.h"

using namespace std;

VoleMachine::VoleMachine() {
    reset();
}

bool VoleMachine::loadProgram(string filename, uint8_t startAddress) {
    return memory.loadProgramFromFile(filename, startAddress);
}

void VoleMachine::reset() {
    memory.clear();
    registers.clear(); 
    screen.clear();
    cu.reset();
}

void VoleMachine::step() { 
    if (cu.isHalted()) {
        return;
    }
    cu.fetch(memory);
    cu.decode();
    cu.execute(memory, registers, alu, screen);
}

void VoleMachine::run() {
    while (!cu.isHalted()) {
        step();
    }
}

string VoleMachine::getScreen() {
    return screen.getOutput();
}

void VoleMachine::displayStatus() const {
    printf("PC: 0x%02X | IR: 0x%04X | Halted: %s\n",
        cu.getPC(),
        cu.getIR(),
        cu.isHalted() ? "Yes" : "No");
}