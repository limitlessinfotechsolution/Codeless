#include "db_manager.h"
#include <stdio.h>

sqlite3* db_init(const char *db_path) {
    sqlite3 *db;
    int rc = sqlite3_open(db_path, &db);

    if (rc) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return NULL;
    } else {
        fprintf(stdout, "Opened database successfully\n");
    }

    // Create a dummy table for testing
    char *err_msg = 0;
    const char *sql = "CREATE TABLE IF NOT EXISTS Users (ID INT PRIMARY KEY NOT NULL, NAME TEXT NOT NULL, AGE INT NOT NULL);";
    rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
    if (rc != SQLITE_OK ) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
    }

    return db;
}

int db_create_table(sqlite3 *db, const char *table_name, GtkListStore *cols_store) {
    GString *sql = g_string_new("");
    g_string_append_printf(sql, "CREATE TABLE %s (", table_name);

    GtkTreeIter iter;
    if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(cols_store), &iter)) {
        do {
            gchar *col_name;
            gchar *col_type;
            gtk_tree_model_get(GTK_TREE_MODEL(cols_store), &iter,
                               COLUMN_COL_NAME, &col_name,
                               COLUMN_COL_TYPE, &col_type,
                               -1);

            g_string_append_printf(sql, "%s %s,", col_name, col_type);

            g_free(col_name);
            g_free(col_type);
        } while (gtk_tree_model_iter_next(GTK_TREE_MODEL(cols_store), &iter));

        // Remove the trailing comma
        g_string_truncate(sql, sql->len - 1);
    }

    g_string_append(sql, ");");

    char *err_msg = 0;
    int rc = sqlite3_exec(db, sql->str, 0, 0, &err_msg);

    if (rc != SQLITE_OK ) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        g_string_free(sql, TRUE);
        return 1;
    }

    g_string_free(sql, TRUE);
    return 0;
}

void db_close(sqlite3 *db) {
    sqlite3_close(db);
}

// Callback for getting tables
static int get_tables_callback(void *data, int argc, char **argv, char **azColName) {
    GtkTreeStore *treestore = (GtkTreeStore*)data;
    GtkTreeIter toplevel;

    gtk_tree_store_append(treestore, &toplevel, NULL);
    gtk_tree_store_set(treestore, &toplevel, 0, argv[0], -1); // Column 0 for name

    return 0;
}


void db_populate_schema_store(sqlite3 *db, GtkTreeStore *treestore) {
    char *err_msg = 0;
    const char *sql = "SELECT name FROM sqlite_master WHERE type='table';";

    // Get all tables and add them as top-level nodes
    sqlite3_exec(db, sql, get_tables_callback, treestore, &err_msg);

    // Now, for each top-level node, get the columns
    GtkTreeIter iter;
    if (!gtk_tree_model_get_iter_first(GTK_TREE_MODEL(treestore), &iter)) {
        return;
    }

    do {
        gchar *table_name;
        gtk_tree_model_get(GTK_TREE_MODEL(treestore), &iter, 0, &table_name, -1);

        char pragma_sql[256];
        snprintf(pragma_sql, sizeof(pragma_sql), "PRAGMA table_info(%s);", table_name);

        sqlite3_stmt *stmt;
        sqlite3_prepare_v2(db, pragma_sql, -1, &stmt, NULL);

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const unsigned char *col_name = sqlite3_column_text(stmt, 1);
            const unsigned char *col_type = sqlite3_column_text(stmt, 2);

            char col_info[512];
            snprintf(col_info, sizeof(col_info), "%s (%s)", col_name, col_type);

            GtkTreeIter child;
            gtk_tree_store_append(treestore, &child, &iter);
            gtk_tree_store_set(treestore, &child, 0, col_info, -1);
        }

        sqlite3_finalize(stmt);
        g_free(table_name);

    } while (gtk_tree_model_iter_next(GTK_TREE_MODEL(treestore), &iter));
}


GtkListStore* db_get_table_data_as_list_store(sqlite3 *db, const char *table_name) {
    GtkListStore *store = NULL;
    sqlite3_stmt *stmt;
    char sql[256];
    snprintf(sql, sizeof(sql), "SELECT * FROM %s;", table_name);

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        return NULL;
    }

    int col_count = sqlite3_column_count(stmt);
    if (col_count == 0) {
        sqlite3_finalize(stmt);
        return NULL;
    }

    GType types[col_count];
    for (int i = 0; i < col_count; i++) {
        types[i] = G_TYPE_STRING; // For simplicity, treat all data as strings
    }
    store = gtk_list_store_newv(col_count, types);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        GtkTreeIter iter;
        gtk_list_store_append(store, &iter);
        for (int i = 0; i < col_count; i++) {
            const gchar *value = (const gchar*)sqlite3_column_text(stmt, i);
            gtk_list_store_set(store, &iter, i, value ? value : "NULL", -1);
        }
    }

    sqlite3_finalize(stmt);
    return store;
}
