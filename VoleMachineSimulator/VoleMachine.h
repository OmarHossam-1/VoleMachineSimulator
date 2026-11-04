# pragma once
#include "ControlUnit.h"
#include "Instruction.h"
#include "Memory.h"
#include "RegisterFile.h"
#include "ALU.h"
#include "Screen.h"
#include <cstdint>
#include <string>

class VoleMachine {
private:
    Memory memory;
    RegisterFile registers;
    ALU alu;
    Screen screen;
    ControlUnit cu;
public:
    VoleMachine();
    bool loadProgram(std::string filename,uint8_t startAddress);
    void run();
    void step();
    void reset();
    std::string getScreen();
    void displayStatus()const;
    void setPC(uint8_t newPC);
    void setRegister(int idx, uint8_t val);
    void setMemoryCell(int addr, uint8_t val);
    uint8_t getRegister(int idx) const;
    uint8_t getMemoryCell(int addr) const;

};


