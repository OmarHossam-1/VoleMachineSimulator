#include <string>
#include "VoleMachine.h"
using namespace std;
VoleMachine::VoleMachine() {
    reset();
}

void VoleMachine::setPC(uint8_t newPC) {
    cu.setPC(newPC);
}

void VoleMachine::setRegister(int idx, uint8_t val) {
    registers.setRegister(idx, val);
}

void VoleMachine::setMemoryCell(int addr, uint8_t val) {
    memory.setCell(addr, val);
}

uint8_t VoleMachine::getRegister(int idx) const {
    return registers.getRegister(idx);
}

uint8_t VoleMachine::getMemoryCell(int addr) const {
    return memory.getCell(addr);
}

bool VoleMachine::loadProgram(string filename, uint8_t startAddress) {
    return memory.loadProgramFromFile(filename,startAddress);
}

void VoleMachine::reset() {
        registers.clear();
        memory.clear();
        cu.reset();

}

void VoleMachine::step()  {

    if (cu.isHalted()) {
        return;
    }
    cu.fetch(memory);
    cu.decode();
    cu.execute(memory,registers,alu,screen);
}

void VoleMachine::run() {
    while (!(cu.isHalted())) {
        step();
    }

}

string VoleMachine::getScreen() {
    return screen.getOutput();
}


void VoleMachine::displayStatus() const {
    std::cout << "PC: " << static_cast<int>(cu.getPC()) << "\n";

    std::cout << "--- Registers ---\n";
    for (int i = 0; i < 16; ++i) {
        std::cout << "R" << i << ": " << static_cast<int>(registers.getRegister(i)) << " ";
    }
    std::cout << "\n";

    std::cout << "--- Memory ---\n";
    for (int i = 0; i < 256; ++i) {
        std::cout << static_cast<int>(memory.getCell(i)) << " ";
    }
    std::cout << "\n";
}

