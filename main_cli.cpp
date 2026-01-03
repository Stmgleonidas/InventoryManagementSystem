#include <iostream>
#include "database.h"
using namespace std;

int main()
{
    sqlite3 *db = openDatabase("inventory.db");
    createTables(db);

    auto inventory = getInventory(db);

    for (const auto& row:inventory) 
    {
        cout << row.box << " | "<< row.item << " | "<< row.quantity << "\n";
    }

    closeDatabase(db);
    return 0;
}
