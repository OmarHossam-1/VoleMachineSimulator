#pragma once
#include <string>
#include <cstdint>

using namespace std;

class Screen
{
private:
    string buffer;

public:
    void write(uint8_t value);
    string getOutput() const;
    void clear();
};

