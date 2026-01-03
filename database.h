#ifndef DATABASE_H
#define DATABASE_H

#include <string>
#include <vector>
#include <sqlite3.h>

using namespace std;

struct inventoryBox
{
    string box, item;
    int quantity;
};


sqlite3 *openDatabase(const string& dbName);
void closeDatabase(sqlite3* db);
void createTables(sqlite3* db);

void addBox(sqlite3 *db, const string &name);
int addItem(sqlite3 *db, const string &name);
void addOrUpdateInventory(sqlite3 *db, int boxId, int itemId, int quantity);

vector<InventoryRow> getInventory(sqlite3* db);

#endif