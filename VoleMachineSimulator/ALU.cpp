#include "ALU.h"
#include <cstdint>
#include <cmath>
using namespace std;

uint8_t ALU::add_integer(uint8_t op1, uint8_t op2)
{
    return static_cast<uint8_t>(op1 + op2);
}

uint8_t ALU::add_float(uint8_t op1, uint8_t op2)
{
    return static_cast<uint8_t>(op1 + op2);
}

uint8_t ALU::bitwise_OR(uint8_t op1, uint8_t op2)
{
    return static_cast<uint8_t>(op1 | op2);
}

uint8_t ALU::bitwise_AND(uint8_t op1, uint8_t op2)
{
    return static_cast<uint8_t>(op1 & op2);
}

uint8_t ALU::bitwise_XOR(uint8_t op1, uint8_t op2)
{
    return static_cast<uint8_t>(op1 ^ op2);
}

uint8_t ALU::rotate_Right(uint8_t value, int count)
{
    count = ((count % 8) + 8) % 8;

    if (count == 0)
        return value;

    return static_cast<uint8_t>(
        (value >> count) | (value << (8 - count))
        );
}