#pragma once

#include <cstdint>
using namespace std;

class ALU
{
public:

    uint8_t add_integer(uint8_t op1, uint8_t op2);

    uint8_t add_float(uint8_t op1, uint8_t op2);

    uint8_t bitwise_OR(uint8_t op1, uint8_t op2);

    uint8_t bitwise_AND(uint8_t op1, uint8_t op2);

    uint8_t bitwise_XOR(uint8_t op1, uint8_t op2);

    uint8_t rotate_Right(uint8_t value, int count);

};