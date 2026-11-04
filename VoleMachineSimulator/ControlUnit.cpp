#include "ControlUnit.h"

ControlUnit::ControlUnit() {
    reset();
}
void ControlUnit::reset() {
    pc=0x00;
    ir=0x0000;
    halted=false;
}

void ControlUnit::setPC(uint8_t newPC) {
    pc = newPC;
    halted = false;
}

bool ControlUnit::isHalted() const {
    return halted;
}

uint8_t ControlUnit::getPC() const {
    return pc;
}

uint16_t ControlUnit::getIR()const {
    return ir;
}

void ControlUnit::fetch(Memory &memory) {
    uint8_t first_byte = memory.getCell(pc);
    uint8_t second_byte = memory.getCell(pc+1);
    ir= static_cast<uint16_t>(first_byte << 8) |second_byte;
    pc+=2;
}

void ControlUnit::decode() {
  current = Instruction::decode(ir);
}

void ControlUnit::execute(Memory &memory, RegisterFile &registers, ALU &alu, Screen &screen) {
    switch (current.opcode) {
        case 1: { // LOAD R, [XY]
            uint8_t value_address = (current.x<< 4|current.y);
            uint8_t value = memory.getCell(value_address);
            registers.setRegister(current.r, value);
            break;
        }
        case 2: { // LOAD R, XY --> Direct
            uint8_t value = (current.x << 4) | current.y;
            registers.setRegister(current.r, value);
            break;
        }
        case 3: { // STORE R, [XY]
            uint8_t address = (current.x<< 4|current.y);
            uint8_t value = registers.getRegister(current.r);

            if (current.x ==0x0f && current.y ==0x0f) {
                screen.write(value);
            }
            else {
                memory.setCell(address, value);

            }
            break;
        }
        case 4: { // MOVE R, S

            uint8_t value = registers.getRegister(current.x);
            registers.setRegister(current.y, value);
            break;
        }

        case 5: { // ADD Integer

            uint8_t val_S= registers.getRegister(current.x);
            uint8_t val_T= registers.getRegister(current.y);
            uint8_t result = alu.addInteger(val_S,val_T);
            registers.setRegister(current.r,result);
            break;
        }
        case 6: { // ADD Float
            uint8_t val_S= registers.getRegister(current.x);
            uint8_t val_T= registers.getRegister(current.y);
            uint8_t result = alu.addFloat(val_S,val_T);
            registers.setRegister(current.r,result);
            break;
        }
        case 7: { // OR
            uint8_t val_S= registers.getRegister(current.x);
            uint8_t val_T= registers.getRegister(current.y);
            uint8_t result = alu.bitwiseOr(val_S,val_T);
            registers.setRegister(current.r,result);
            break;
        }
        case 8: { // AND
            uint8_t val_S=registers.getRegister(current.x);
            uint8_t val_T = registers.getRegister(current.y);
            uint8_t result=alu.bitwiseAnd(val_S,val_T);
            registers.setRegister(current.r,result);
            break;
        }
        case 9: { // XOR
            uint8_t val_S = registers.getRegister(current.x);
            uint8_t val_T = registers.getRegister(current.y);
            uint8_t result = alu.bitwiseXor(val_S,val_T);
            registers.setRegister(current.r,result);
            break;

        }
        case 10: { // ROTATE
            uint8_t val_R = registers.getRegister(current.r);
            uint8_t result = alu.rotateRight(val_R,current.y);
            registers.setRegister(current.r,result);
            break;
        }
        case 11: { // JUMP R, XY
            uint8_t val_R = registers.getRegister(current.r);
            uint8_t val_R0 = registers.getRegister(0);
            if (val_R == val_R0) {
                uint8_t XY_address = (current.x << 4)|current.y;
                pc = XY_address;

            }
            break;
        }
        case 12: { // HALT...
            halted = true;
            break;
        }
        case 13: {
            if (registers.getRegister(current.r) > registers.getRegister(0)) {
                pc = current.address;
            }
            break;
        }
        default:
        {
            std::cerr << "Error: Invalid Instruction encountered!\n";
            halted = true;
            break;
        }
    }
}
