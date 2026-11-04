#ifndef REGISTERFILE_H #define REGISTERFILE_H
class RegisterFile { private: unsigned char registers[16];
public: RegisterFile();
unsigned char getRegister(int address);
void setRegister(int address, unsigned char value);
void clear();
};
#endif
