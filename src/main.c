#include <gtk/gtk.h>
#include "db_manager.h"
#include "ui_manager.h"

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    // Initialize the database
    sqlite3 *db = db_init("codeless.db");
    if (db == NULL) {
        return 1; // Exit if DB fails to open
    }

    // Build the UI
    GtkWidget *window = ui_manager_build_main_window(db, argc, argv);

    // Show the window and start the GTK main loop
    gtk_widget_show_all(window);
    gtk_main();

    // Close the database connection
    db_close(db);

    return 0;
}
