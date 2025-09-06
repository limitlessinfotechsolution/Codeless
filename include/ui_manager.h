#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include <gtk/gtk.h>
#include <sqlite3.h>
#include "db_manager.h" // For the column enum

// Struct to hold pointers to the widgets
typedef struct {
    GtkWidget *editor;
    GtkTextBuffer *buffer;
} AppWidgets;

// Struct to hold pointers to DB related widgets
typedef struct {
    sqlite3 *db;
    GtkWidget *data_view_container;
} DBWidgets;

typedef struct {
    GtkWindow *parent_window;
    sqlite3 *db;
    GtkTreeStore *schema_store;
} NewTableDialogData;

GtkWidget* ui_manager_build_main_window(sqlite3 *db, int argc, char *argv[]);

#endif // UI_MANAGER_H
