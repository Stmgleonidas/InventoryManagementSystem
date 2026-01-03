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
void closeDatabase(sqlite3 *db) 
{
    sqlite3_close(db);
}

//---Create Database Tables If They Don't Exist---//
void createTables(sqlite3 *db) 
{
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS boxes (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT UNIQUE);
        CREATE TABLE IF NOT EXISTS items (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT);
        CREATE TABLE IF NOT EXISTS inventory (box_id INTEGER, item_id INTEGER, quantity INTEGER,UNIQUE(box_id, item_id));
    )";
    sqlite3_exec(db, sql, nullptr, nullptr, nullptr);
}


//------- Database Functions -------//


//--- Add Box ---//
void addBox(sqlite3* db, const string& name)
{
    const char* sql = "INSERT OR IGNORE INTO boxes (name) VALUES (?);";
    sqlite3_stmt* stmt;

    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

//--- Update Inventory ---//
void addInventory(sqlite3* db, int boxId, int itemId, int quantity)
{
    const char* sql = R"(
        INSERT INTO inventory (box_id, item_id, quantity)
        VALUES (?, ?, ?)
        ON CONFLICT(box_id, item_id)
        DO UPDATE SET quantity = quantity + excluded.quantity;
    )";

    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

    sqlite3_bind_int(stmt, 1, boxId);
    sqlite3_bind_int(stmt, 2, itemId);
    sqlite3_bind_int(stmt, 3, quantity);

    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}


void removeQuantity(sqlite3* db, int boxId, int itemId, int amount)
{
    const char* sql = R"(
        UPDATE inventory
        SET quantity = quantity - ?
        WHERE box_id = ? AND item_id = ?;
    )";

    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

    sqlite3_bind_int(stmt, 1, amount);
    sqlite3_bind_int(stmt, 2, boxId);
    sqlite3_bind_int(stmt, 3, itemId);

    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    /* Auto-clean zero or negative quantities */
    const char* cleanup = R"(
        DELETE FROM inventory
        WHERE quantity <= 0;
    )";
    sqlite3_exec(db, cleanup, nullptr, nullptr, nullptr);
}


void deleteInventory(sqlite3* db, int boxId, int itemId)
{
    const char* sql = "DELETE FROM inventory WHERE box_id = ? AND item_id = ?;";
    sqlite3_stmt* stmt;

    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, boxId);
    sqlite3_bind_int(stmt, 2, itemId);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}



int addItem(sqlite3* db, const string& name)
{
    const char* sql = "INSERT INTO items (name) VALUES (?);";
    sqlite3_stmt* stmt;

    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return sqlite3_last_insert_rowid(db);
}




vector<inventoryRow> getInventory(sqlite3 *db)
{
    vector<inventoryRow> result;

    const char *sql = R"(
        SELECT inventory.box_id, boxes.name, items.name, inventory.quantity
        FROM inventory
        JOIN boxes ON boxes.id = inventory.box_id
        JOIN items ON items.id = inventory.item_id;
    )";

    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

    while (sqlite3_step(stmt) == SQLITE_ROW) 
    {
        inventoryRow row;
        row.boxId = sqlite3_column_int(stmt, 0);      // box_id
        row.box   = (const char*)sqlite3_column_text(stmt, 1);
        row.item  = (const char*)sqlite3_column_text(stmt, 2);
        row.quantity = sqlite3_column_int(stmt, 3);
        result.push_back(row);
    }

    sqlite3_finalize(stmt);
    return result;
}
