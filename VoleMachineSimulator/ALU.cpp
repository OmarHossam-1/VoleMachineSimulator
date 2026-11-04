#include "ALU.h"
#include <cstdint>
#include <cstring>

using namespace std;

uint8_t ALU::addInteger(uint8_t op1, uint8_t op2)
{
    return static_cast<uint8_t>(op1 + op2);
}

uint32_t ALU::addFloat(uint32_t op1, uint32_t op2)
{
    float num1, num2, result;

    memcpy(&num1, &op1, sizeof(float));
    memcpy(&num2, &op2, sizeof(float));

    result = num1 + num2;

    uint32_t output;
    memcpy(&output, &result, sizeof(float));

    return output;
}

uint8_t ALU::bitwiseOr(uint8_t op1, uint8_t op2)
{
    return static_cast<uint8_t>(op1 | op2);
}

uint8_t ALU::bitwiseAnd(uint8_t op1, uint8_t op2)
{
    return static_cast<uint8_t>(op1 & op2);
}

uint8_t ALU::bitwiseXor(uint8_t op1, uint8_t op2)
{
    return static_cast<uint8_t>(op1 ^ op2);
}

uint8_t ALU::rotateRight(uint8_t value, int count)
{
    count = ((count % 8) + 8) % 8;

    if (count == 0)
        return value;

    return static_cast<uint8_t>(
        (value >> count) | (value << (8 - count))
    );
}