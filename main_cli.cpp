#include <iostream>
#include "database.h"

using namespace std;

void printMenu()
{
    cout << "\n===== INVENTORY MANAGEMENT =====\n";
    cout << "1. Add box\n";
    cout << "2. Add item to box\n";
    cout << "3. Show full inventory\n";
    cout << "4. Remove quantity from item\n";
    cout << "5. Delete item completely\n";
    cout << "0. Exit\n";
    cout << "================================\n";
    cout << "Choice: ";
}

int main()
{
    sqlite3* db = openDatabase("inventory.db");
    if (!db)
    {
        cout << "Failed to open database\n";
        return 1;
    }

    createTables(db);

    int choice = -1;

    while (choice != 0)
    {
        printMenu();
        cin >> choice;

        switch(choice)
        {
            case 1:{
                string boxName;
                cout << "Box name: ";
                cin.ignore();
                getline(cin, boxName);

                addBox(db, boxName);
                cout << "Box added.\n";
                break;
            }
            case 2:{
                string itemName;
                int boxId, quantity;

                cout << "Item name: ";
                cin.ignore();
                getline(cin, itemName);

                cout << "Box ID: ";
                cin >> boxId;

                cout << "Quantity: ";
                cin >> quantity;

                int itemId = addItem(db, itemName);
                addInventory(db, boxId, itemId, quantity);

                cout << "Item added to box.\n";
                break;
            }
            case 3:{
                auto inventory = getInventory(db);

                if (!inventory.empty())
                {
                    cout << "\nBox Id | Box Name | Item | QTY\n"<< "----------------------\n";
                    for (const auto& row : inventory){cout << row.boxId <<" | "<< row.box << " | " << row.item << " | "<< row.quantity << "\n";}
                } else
                {
                    cout << "Inventory is empty.\n";
                }
                break;
            }
            case 4:{
                int boxId, itemId, amount;

                cout << "Box ID: ";
                cin >> boxId;

                cout << "Item ID: ";
                cin >> itemId;

                cout << "Amount to remove: ";
                cin >> amount;

                removeQuantity(db, boxId, itemId, amount);
                cout << "Quantity updated.\n";
                break;
            }
            case 5:{
                int boxId, itemId;

                cout << "Box ID: ";
                cin >> boxId;

                cout << "Item ID: ";
                cin >> itemId;

                deleteInventory(db, boxId, itemId);
                cout << "Item deleted.\n";
                break;
            }
            case 0:
                cout << "Exiting...\n";
            
            default:
                cout << "Invalid option.\n";
        }
    }

    closeDatabase(db);
    return 0;
}
