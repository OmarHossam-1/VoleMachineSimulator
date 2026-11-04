# pragma once
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
    bool loadProgram(string filename,uint8_t startAddress);
    void run();
    void step()const;
    void reset();
    string getScreen();
    void displayStatus()const;

};


