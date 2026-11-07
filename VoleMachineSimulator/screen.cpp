#include "Screen.h"
#include <string>

void Screen::write(uint8_t value)
{
	if (!buffer.empty()) {
		buffer += " ";
	}
	buffer += std::to_string(static_cast<int>(static_cast<int8_t>(value)));
}

string Screen::getOutput() const
{
	return buffer;
}

void Screen::clear()
{
	buffer.clear();
}
