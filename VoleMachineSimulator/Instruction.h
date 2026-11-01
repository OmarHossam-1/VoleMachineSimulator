#pragma once

#include <cstdint>
using namespace std;

struct Instruction
{
    uint8_t opcode;
    uint8_t r;
    uint8_t x;
    uint8_t y;
    uint16_t raw_value;

    static Instruction decode(uint16_t raw_value);

    uint8_t xy() const;
};