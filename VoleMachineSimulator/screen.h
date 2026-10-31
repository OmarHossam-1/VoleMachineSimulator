#pragma once
#include <string>
#include <cstdint>

using namespace std;

class screen
{
private:
    string buffer;

public:
    void write(uint8_t value);
    string getOutput() const;
    void clear();
};

