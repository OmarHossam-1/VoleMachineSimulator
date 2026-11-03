#ifndef MEMORY_H #define MEMORY_H
#include <string>
class Memory { private: unsigned char memory[256];
public: Memory();
unsigned char getCell(int address);
void setCell(int address, unsigned char value);
void clear();
bool loadProgramFromFile(std::string filename);
};
#endif