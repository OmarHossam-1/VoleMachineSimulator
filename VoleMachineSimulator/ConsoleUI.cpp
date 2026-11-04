#include "ConsoleUI.h"
#include <iostream>
#include <iomanip>

using namespace std;

void ConsoleUI::displayMenu() {
    cout << "\n========== VOLE MACHINE SIMULATOR ==========\n";
    cout << "1. Load Program from file\n";
    cout << "2. Modify Register or Memory Cell\n";
    cout << "3. Set Program Counter (PC)\n";
    cout << "4. Display Machine Status (Memory, Registers, IR)\n";
    cout << "5. Execute Next Instruction (Step)\n";
    cout << "6. Run Entire Program\n";
    cout << "7. Display Screen Output\n";
    cout << "8. Reset Machine\n";
    cout << "0. Exit\n";
    cout << "Enter your choice: ";
}

int ConsoleUI::getUserChoice() {
    int choice;
    cin >> choice;
    return choice;
}

void ConsoleUI::displayRegisters(VoleMachine& machine) {
    cout << "\n--- Registers ---\n";
    for (int i = 0; i < 16; i++) {
        cout << "R" << hex << uppercase << i << ": "
            << setw(2) << setfill('0') << static_cast<int>(machine.getRegister(i)) << "  ";
        if ((i + 1) % 4 == 0) cout << "\n";
    }
    cout << dec;
}

void ConsoleUI::displayMemory(VoleMachine& machine) {
    cout << "\n--- Memory (256 Bytes) ---\n    ";
    for (int col = 0; col < 16; col++) {
        cout << hex << uppercase << setw(2) << setfill('0') << col << " ";
    }
    cout << "\n";
    for (int row = 0; row < 16; row++) {
        cout << hex << uppercase << setw(2) << setfill('0') << (row * 16) << ": ";
        for (int col = 0; col < 16; col++) {
            int addr = row * 16 + col;
            cout << hex << uppercase << setw(2) << setfill('0')
                << static_cast<int>(machine.getMemoryCell(addr)) << " ";
        }
        cout << "\n";
    }
    cout << dec;
}

void ConsoleUI::displayStatus(VoleMachine& machine) {
    cout << "\n--- CPU State ---\n";
    machine.displayStatus();
    displayRegisters(machine);
    displayMemory(machine);
}

void ConsoleUI::displayScreen(VoleMachine& machine) {
    cout << "\n--- Screen Output ---\n";
    cout << machine.getScreen() << "\n";
    cout << "---------------------\n";
}