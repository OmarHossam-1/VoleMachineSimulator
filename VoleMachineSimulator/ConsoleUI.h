#pragma once
#include "VoleMachine.h"

class ConsoleUI {
public:
    void displayMenu();
    int getUserChoice();
    void displayMemory(VoleMachine &machine);
    void displayRegisters(VoleMachine &machine);
    void displayStatus(VoleMachine &machine);
    void displayScreen(VoleMachine &machine);
};