#include "Instruction.h"
#include <cstdint>

Instruction Instruction::decode(uint16_t raw_value)
{
    Instruction inst;

    inst.raw_value = raw_value;
    inst.opcode = (raw_value >> 12) & 0xF;
    inst.r = (raw_value >> 8) & 0xF;
    inst.x = (raw_value >> 4) & 0xF;
    inst.y = raw_value & 0xF;

    return inst;
}

uint8_t Instruction::xy() const
{
    return static_cast<uint8_t>((x << 4) | y);
}