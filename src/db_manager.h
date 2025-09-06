#ifndef DB_MANAGER_H
#define DB_MANAGER_H

#include <sqlite3.h>
#include <gtk/gtk.h> // For GtkTreeStore

// Enum for the columns in the column list store
enum {
    COLUMN_COL_NAME,
    COLUMN_COL_TYPE,
    NUM_COL_COLS
};

sqlite3* db_init(const char *db_path);
void db_close(sqlite3 *db);

void db_populate_schema_store(sqlite3 *db, GtkTreeStore *treestore);
GtkListStore* db_get_table_data_as_list_store(sqlite3 *db, const char *table_name);
int db_create_table(sqlite3 *db, const char *table_name, GtkListStore *cols_store);

#endif // DB_MANAGER_H
