#pragma once
#include <cstdint>
#include "Instruction.h"
#include "Memory.h"
#include "Register.h"
#include "ALU.h"
#include "Screen.h"

class ControlUnit {
private:
    uint8_t pc;
    uint16_t ir;
    bool halted;
    Instruction current;

public:
    ControlUnit();
    void fetch(Memory& memory);
    void decode();
    void execute(Memory& memory, RegisterFile& registers, ALU& alu, Screen& screen);
    void reset();
    void setPC(uint8_t newPC);

    bool isHalted() const;
    uint8_t getPC() const;
    uint16_t getIR() const;
};