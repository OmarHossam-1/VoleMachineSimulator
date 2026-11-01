#include "ConsoleUI.h"
#include <iostream>

using namespace std;

void ConsoleUI::displayMenu()
{
    cout << "\n--- Vole Machine Simulator ---\n";
    cout << "1. Load Program from file\n";
    cout << "2. Modify Register or Memory Cell\n";
    cout << "3. Set Program Counter (PC)\n";
    cout << "4. Display Machine Status (Memory, Registers, IR)\n";
    cout << "5. Execute Next Instruction (Step)\n";
    cout << "6. Run Entire Program\n";
    cout << "7. Display Screen Output\n";
    cout << "0. Exit\n";
    cout << "\nEnter your choice: ";
}

int ConsoleUI::getUserChoice()
{
    int Choice;
    cin >> Choice;
    return Choice;
}

void ConsoleUI::displayMemory()
{
    cout << "\n[System]: Displaying Memory...\n";
}

void ConsoleUI::displayRegisters()
{
    cout << "\n[System]: Displaying Registers and PC/IR...\n";
}

void ConsoleUI::displayStatus()
{
    displayMemory();
    displayRegisters();
}

void ConsoleUI::displayScreen()
{
    cout << "\n[System]: Screen Output goes here.\n";
}