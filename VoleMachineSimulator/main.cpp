#include <iostream>
#include <string>
#include "VoleMachine.h"
#include "ConsoleUI.h"

using namespace std;

int main() {
    VoleMachine machine;
    ConsoleUI ui;
    bool running = true;

    while (running) {
        ui.displayMenu();
        int choice = ui.getUserChoice();

        switch (choice) {
        case 1: {
            string filename;
            int startAddr = 0;
            cout << "Enter filename: ";
            cin >> filename;
            cout << "Enter start address in Hex (default 00): ";
            cin >> hex >> startAddr >> dec;

            if (machine.loadProgram(filename, static_cast<uint8_t>(startAddr))) {
                machine.setPC(static_cast<uint8_t>(startAddr));
                cout << "[System]: Program loaded successfully.\n";
            }
            else {
                cout << "[Error]: Could not open file.\n";
            }
            break;
        }
        case 2: {
            int type, addr, val;
            cout << "1. Modify Register\n2. Modify Memory Cell\nChoice: ";
            cin >> type;
            if (type == 1) {
                cout << "Register index (Hex 0-F): ";
                cin >> hex >> addr;
                cout << "New value (Hex 00-FF): ";
                cin >> hex >> val >> dec;
                machine.setRegister(addr, static_cast<uint8_t>(val));
            }
            else if (type == 2) {
                cout << "Memory address (Hex 00-FF): ";
                cin >> hex >> addr;
                cout << "New value (Hex 00-FF): ";
                cin >> hex >> val >> dec;
                machine.setMemoryCell(addr, static_cast<uint8_t>(val));
            }
            break;
        }
        case 3: {
            int newPC;
            cout << "Enter new PC (Hex 00-FF): ";
            cin >> hex >> newPC >> dec;
            machine.setPC(static_cast<uint8_t>(newPC));
            break;
        }
        case 4:
            ui.displayStatus(machine);
            break;
        case 5:
            machine.step();
            ui.displayStatus(machine);
            break;
        case 6:
            machine.run();
            ui.displayStatus(machine);
            break;
        case 7:
            ui.displayScreen(machine);
            break;
        case 8:
            machine.reset();
            cout << "[System]: Machine reset complete.\n";
            break;
        case 0:
            running = false;
            break;
        default:
            cout << "[Error]: Invalid option!\n";
        }
    }
    return 0;
}
