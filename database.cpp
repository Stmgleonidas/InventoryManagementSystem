#include <iostream>
#include "database.h"
using namespace std;

//---Open Database---//
sqlite3 *openDatabase(const string& dbName) 
{
    sqlite3 *db;
    if (sqlite3_open(dbName.c_str(), &db)) 
    {
        cerr << "DB error\n";
        return nullptr;
    }
    return db;
}

//---Close Database---//
void closeDatabase(sqlite3 *db) {
    sqlite3_close(db);
}

//---Create Database Tables---//
void createTables(sqlite3 *db) {
    const char* sql = R"
    //---SQL GOES HERE---//
    ";
    sqlite3_exec(db, sql, nullptr, nullptr, nullptr); // <-
}


vector<InventoryRow> getInventory(sqlite3 *db) 
{
    vector<InventoryRow> result;

    const char *sql = R"(
        SELECT boxes.name, items.name, inventory.quantity
        FROM inventory
        JOIN boxes ON boxes.id = inventory.box_id
        JOIN items ON items.id = inventory.item_id;
    )";

    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

    while (sqlite3_step(stmt) == SQLITE_ROW) 
    {
        InventoryRow row;
        row.box = (const char*)sqlite3_column_text(stmt, 0);
        row.item = (const char*)sqlite3_column_text(stmt, 1);
        row.quantity = sqlite3_column_int(stmt, 2);
        result.push_back(row);
    }

    sqlite3_finalize(stmt);
    return result;
}