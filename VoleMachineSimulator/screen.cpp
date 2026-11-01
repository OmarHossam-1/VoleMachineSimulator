#include "screen.h"

void Screen::write(uint8_t value)
{
	buffer += static_cast<char>(value);
}

string Screen::getOutput() const
{
	return buffer;
}

void Screen::clear()
{
	buffer.clear();
}