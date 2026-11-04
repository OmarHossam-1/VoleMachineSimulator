#include "ControlUnit.h"
#include <iostream>
#include <cstdint>

ControlUnit::ControlUnit() {
    reset();
}

void ControlUnit::reset() {
    pc = 0x00;
    ir = 0x0000;
    halted = false;
}

bool ControlUnit::isHalted() const {
    return halted;
}

void ControlUnit::setPC(uint8_t newPC) {
    pc = newPC;
    halted = false;
}

uint8_t ControlUnit::getPC() const {
    return pc;
}

uint16_t ControlUnit::getIR() const {
    return ir;
}

void ControlUnit::fetch(Memory& memory) {
    uint8_t first_byte = memory.getCell(pc);
    uint8_t second_byte = memory.getCell(pc + 1);
    ir = (static_cast<uint16_t>(first_byte) << 8) | second_byte;
    pc += 2;
}

void ControlUnit::decode() {
    current = Instruction::decode(ir);
}

void ControlUnit::execute(Memory& memory, RegisterFile& registers, ALU& alu, Screen& screen) {
    switch (current.opcode) {
    case 1: { // 1RXY: LOAD R from Memory[XY]
        uint8_t value_address = (current.x << 4) | current.y;
        uint8_t value = memory.getCell(value_address);
        registers.setRegister(current.r, value);
        break;
    }
    case 2: { // 2RXY: LOAD R with immediate value XY
        uint8_t value = (current.x << 4) | current.y;
        registers.setRegister(current.r, value);
        break;
    }
    case 3: { // 3RXY: STORE R to Memory[XY] (00 or FF writes to Screen)
        uint8_t address = (current.x << 4) | current.y;
        uint8_t value = registers.getRegister(current.r);

        if (address == 0x00 || address == 0xFF) {
            screen.write(value);
            memory.setCell(address, value);
        }
        else {
            memory.setCell(address, value);
        }
        break;
    }
    case 4: { // 40RS: MOVE (Copy) bits from Register R (current.x) to Register S (current.y)
        uint8_t value = registers.getRegister(current.x);
        registers.setRegister(current.y, value);
        break;
    }
    case 5: { // 5RST: ADD Integer (2's complement) -> R = S + T
        uint8_t val_S = registers.getRegister(current.x);
        uint8_t val_T = registers.getRegister(current.y);
        uint8_t result = alu.add_integer(val_S, val_T);
        registers.setRegister(current.r, result);
        break;
    }
    case 6: { // 6RST: ADD Floating-Point -> R = S + T
        uint8_t val_S = registers.getRegister(current.x);
        uint8_t val_T = registers.getRegister(current.y);
        uint8_t result = alu.add_float(val_S, val_T);
        registers.setRegister(current.r, result);
        break;
    }
    case 7: { // 7RST: Bitwise OR -> R = S | T
        uint8_t val_S = registers.getRegister(current.x);
        uint8_t val_T = registers.getRegister(current.y);
        uint8_t result = alu.bitwise_OR(val_S, val_T);
        registers.setRegister(current.r, result);
        break;
    }
    case 8: { // 8RST: Bitwise AND -> R = S & T
        uint8_t val_S = registers.getRegister(current.x);
        uint8_t val_T = registers.getRegister(current.y);
        uint8_t result = alu.bitwise_AND(val_S, val_T);
        registers.setRegister(current.r, result);
        break;
    }
    case 9: { // 9RST: Bitwise XOR -> R = S ^ T
        uint8_t val_S = registers.getRegister(current.x);
        uint8_t val_T = registers.getRegister(current.y);
        uint8_t result = alu.bitwise_XOR(val_S, val_T);
        registers.setRegister(current.r, result);
        break;
    }
    case 10: { // AR0X (0xA): ROTATE Right register R by X steps
        uint8_t val_R = registers.getRegister(current.r);
        uint8_t result = alu.rotate_Right(val_R, current.y);
        registers.setRegister(current.r, result);
        break;
    }
    case 11: { // BRXY (0xB): JUMP to XY if R == R0
        uint8_t val_R = registers.getRegister(current.r);
        uint8_t val_R0 = registers.getRegister(0);
        if (val_R == val_R0) {
            uint8_t XY_address = (current.x << 4) | current.y;
            pc = XY_address;
        }
        break;
    }
    case 12: { // C000 (0xC): HALT execution
        halted = true;
        break;
    }
    default: {
        halted = true;
        break;
    }
    }
}