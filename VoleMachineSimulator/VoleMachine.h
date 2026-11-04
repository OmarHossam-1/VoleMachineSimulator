#pragma once
#include "ControlUnit.h"
#include "Instruction.h"
#include "Memory.h"
#include "RegisterFile.h"
#include "ALU.h"
#include "Screen.h"
#include <cstdint>
#include <string>

using namespace std;

class VoleMachine {
private:
    Memory memory;
    RegisterFile registers;
    ALU alu;
    Screen screen;
    ControlUnit cu;

public:
    VoleMachine();
    bool loadProgram(string filename, uint8_t startAddress);
    void run();
    void step(); 
    void reset();
    string getScreen();
    void displayStatus() const;

    uint8_t getPC() const { return cu.getPC(); }
    uint16_t getIR() const { return cu.getIR(); }
    bool isHalted() const { return cu.isHalted(); }

    void setPC(uint8_t newPC) { cu.setPC(newPC); }
    void setRegister(int index, uint8_t value) { registers.setRegister(index, value); }
    void setMemoryCell(int address, uint8_t value) { memory.setCell(address, value); }

    uint8_t getRegister(int index) { return registers.getRegister(index); }
    uint8_t getMemoryCell(int address) { return memory.getCell(address); }
};