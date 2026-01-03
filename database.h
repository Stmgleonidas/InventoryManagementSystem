#ifndef DATABASE_H
#define DATABASE_H

#include <string>
#include <vector>
#include <sqlite3.h>

using namespace std;

struct inventoryRow
{
    int quantity, boxId;
    string box, item;
};


sqlite3 *openDatabase(const string& dbName);
void closeDatabase(sqlite3* db);
void createTables(sqlite3* db);

void addBox(sqlite3 *db, const string &name);
int addItem(sqlite3 *db, const string &name);
void addInventory(sqlite3 *db, int boxId, int itemId, int quantity);
void removeQuantity(sqlite3* db, int boxId, int itemId, int amount);
void deleteInventory(sqlite3* db, int boxId, int itemId);


vector<inventoryRow> getInventory(sqlite3* db);

#endif
