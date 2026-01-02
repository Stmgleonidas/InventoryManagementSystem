#include <iostream>
#include "sqlite3.h"
#include <string>

using namespace std;



void exec(sqlite3* db, const char* sql) 
{
    char* errMsg = nullptr;

    if (sqlite3_exec(db, sql, nullptr, nullptr, &errMsg) != SQLITE_OK) 
    {
        cerr << "SQL error: " << errMsg << "\n";
        sqlite3_free(errMsg);
    }
}


void createTables()
{
    exec(db, "CREATE TABLE IF NOT EXISTS ")
};

int main(void)
{
    sqlite *db;
    char *errorMsg=nullptr;

    int rc=sqlite3_open("test.db", &db);

    if(rc)
    {
        cerr << "Can't open db" << sqlite3_errmsg<<endl;return 1;
    }

    
}