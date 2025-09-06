#include "ui_manager.h"
#include "db_manager.h"
#include <gtksourceview/gtksource.h>
#include <vte/vte.h>
#include <dirent.h>
#include <string.h>

// Enum for the columns in the GtkTreeStore
enum {
    COLUMN_ICON,
    COLUMN_TEXT,
    COLUMN_PATH,
    NUM_COLUMNS
};


void on_schema_row_activated(GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column, DBWidgets *db_widgets) {
    GtkTreeModel *model;
    GtkTreeIter iter;

    model = gtk_tree_view_get_model(tree_view);
    if (gtk_tree_model_get_iter(model, &iter, path)) {
        // Only act on top-level rows (tables), not children (columns)
        if (gtk_tree_path_get_depth(path) == 1) {
            gchar *table_name;
            gtk_tree_model_get(model, &iter, 0, &table_name, -1);

            GtkListStore *data_store = db_get_table_data_as_list_store(db_widgets->db, table_name);

            if (data_store) {
                // Remove the old placeholder/treeview
                GtkWidget *old_child = gtk_bin_get_child(GTK_BIN(db_widgets->data_view_container));
                if (old_child) {
                    gtk_container_remove(GTK_CONTAINER(db_widgets->data_view_container), old_child);
                }

                GtkWidget *data_treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(data_store));
                g_object_unref(data_store);

                // Create columns dynamically
                char *col_name;
                sqlite3_stmt *stmt;
                char sql[256];
                snprintf(sql, sizeof(sql), "PRAGMA table_info(%s);", table_name);
                sqlite3_prepare_v2(db_widgets->db, sql, -1, &stmt, NULL);
                int current_col = 0;
                while (sqlite3_step(stmt) == SQLITE_ROW) {
                    col_name = (char *)sqlite3_column_text(stmt, 1);
                    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
                    GtkTreeViewColumn *col = gtk_tree_view_column_new_with_attributes(col_name, renderer, "text", current_col, NULL);
                    gtk_tree_view_append_column(GTK_TREE_VIEW(data_treeview), col);
                    current_col++;
                }
                sqlite3_finalize(stmt);


                gtk_container_add(GTK_CONTAINER(db_widgets->data_view_container), data_treeview);
                gtk_widget_show_all(db_widgets->data_view_container);
            }

            g_free(table_name);
        }
    }
}


// Callback for the "Get AI Explanation" button
void on_get_explanation_clicked(GtkButton *button, GtkTextView *error_textview) {
    // For the placeholder, we'll just show a message dialog with a fixed explanation
    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(button))),
                                               GTK_DIALOG_DESTROY_WITH_PARENT,
                                               GTK_MESSAGE_INFO,
                                               GTK_BUTTONS_OK,
                                               "AI Explanation:\n\nThis is a sample explanation for a common error. A real AI would analyze the error message and provide a more detailed and context-specific explanation.");
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

// Callback for the "AI Debug" button
void on_debug_clicked(GtkButton *button, GtkWindow *parent_window) {
    GtkWidget *dialog, *content_area;
    GtkWidget *textview, *scrolled_window;
    GtkWidget *explanation_button;

    dialog = gtk_dialog_new_with_buttons("AI Debugger",
                                         parent_window,
                                         GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                         "_Close",
                                         GTK_RESPONSE_CLOSE,
                                         NULL);

    content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

    // Add a text view for the error message
    scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_size_request(scrolled_window, 400, 100);
    textview = gtk_text_view_new();
    gtk_container_add(GTK_CONTAINER(scrolled_window), textview);
    gtk_box_pack_start(GTK_BOX(content_area), scrolled_window, TRUE, TRUE, 0);

    // Add the "Get Explanation" button
    explanation_button = gtk_button_new_with_label("Get AI Explanation");
    g_signal_connect(explanation_button, "clicked", G_CALLBACK(on_get_explanation_clicked), textview);
    gtk_box_pack_start(GTK_BOX(content_area), explanation_button, FALSE, FALSE, 0);

    gtk_widget_show_all(dialog);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}


// Callback to add a new column to the list store
void on_add_col_clicked(GtkButton *button, GtkListStore *liststore) {
    GtkTreeIter iter;
    gtk_list_store_append(liststore, &iter);
    gtk_list_store_set(liststore, &iter,
                       COLUMN_COL_NAME, "NewColumn",
                       COLUMN_COL_TYPE, "TEXT",
                       -1);
}

// Callback to remove the selected column
void on_remove_col_clicked(GtkButton *button, GtkTreeView *treeview) {
    GtkTreeSelection *selection = gtk_tree_view_get_selection(treeview);
    GtkTreeModel *model;
    GtkTreeIter iter;
    if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
        gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
    }
}

// Callback for when a column name is edited
void on_col_name_edited(GtkCellRendererText *renderer,
                        const gchar *path_string,
                        const gchar *new_text,
                        gpointer user_data) {
    GtkTreeModel *model = (GtkTreeModel*)user_data;
    GtkTreePath *path = gtk_tree_path_new_from_string(path_string);
    GtkTreeIter iter;
    gtk_tree_model_get_iter(model, &iter, path);
    gtk_list_store_set(GTK_LIST_STORE(model), &iter, COLUMN_COL_NAME, new_text, -1);
    gtk_tree_path_free(path);
}

// Callback for when a column type is edited
void on_col_type_edited(GtkCellRendererText *renderer,
                        const gchar *path_string,
                        const gchar *new_text,
                        gpointer user_data) {
    GtkTreeModel *model = (GtkTreeModel*)user_data;
    GtkTreePath *path = gtk_tree_path_new_from_string(path_string);
    GtkTreeIter iter;
    gtk_tree_model_get_iter(model, &iter, path);
    gtk_list_store_set(GTK_LIST_STORE(model), &iter, COLUMN_COL_TYPE, new_text, -1);
    gtk_tree_path_free(path);
}


// Callback for the "New Table" button
void on_new_table_clicked(GtkButton *button, NewTableDialogData *data) {
    GtkWidget *dialog, *content_area;
    GtkWidget *table_name_entry;
    GtkWidget *label;
    GtkWidget *scrolled_window_cols;
    GtkWidget *cols_treeview;
    GtkListStore *cols_liststore;
    GtkWidget *hbox;
    GtkWidget *add_col_button, *remove_col_button;


    dialog = gtk_dialog_new_with_buttons("Create New Table",
                                         data->parent_window,
                                         GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                         "_Cancel",
                                         GTK_RESPONSE_CANCEL,
                                         "_Create",
                                         GTK_RESPONSE_ACCEPT,
                                         NULL);

    content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_box_set_spacing(GTK_BOX(content_area), 10);


    // --- Table Name ---
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    label = gtk_label_new("Table Name:");
    table_name_entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), table_name_entry, TRUE, TRUE, 0);
    gtk_container_add(GTK_CONTAINER(content_area), hbox);


    // --- Columns ---
    label = gtk_label_new("Columns:");
    gtk_container_add(GTK_CONTAINER(content_area), label);

    scrolled_window_cols = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window_cols),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(scrolled_window_cols, -1, 150);
    gtk_container_add(GTK_CONTAINER(content_area), scrolled_window_cols);

    cols_liststore = gtk_list_store_new(NUM_COL_COLS, G_TYPE_STRING, G_TYPE_STRING);
    cols_treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(cols_liststore));
    g_object_unref(cols_liststore);
    gtk_container_add(GTK_CONTAINER(scrolled_window_cols), cols_treeview);

    // Column Name
    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    g_object_set(renderer, "editable", TRUE, NULL);
    g_signal_connect(renderer, "edited", G_CALLBACK(on_col_name_edited), cols_liststore);
    GtkTreeViewColumn *col = gtk_tree_view_column_new_with_attributes("Column Name",
                                                                      renderer,
                                                                      "text", COLUMN_COL_NAME,
                                                                      NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(cols_treeview), col);

    // Column Type
    renderer = gtk_cell_renderer_text_new();
    g_object_set(renderer, "editable", TRUE, NULL);
    g_signal_connect(renderer, "edited", G_CALLBACK(on_col_type_edited), cols_liststore);
    col = gtk_tree_view_column_new_with_attributes("Data Type",
                                                   renderer,
                                                   "text", COLUMN_COL_TYPE,
                                                   NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(cols_treeview), col);


    // --- Add/Remove Buttons ---
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    add_col_button = gtk_button_new_with_label("Add Column");
    g_signal_connect(add_col_button, "clicked", G_CALLBACK(on_add_col_clicked), cols_liststore);
    remove_col_button = gtk_button_new_with_label("Remove Column");
    g_signal_connect(remove_col_button, "clicked", G_CALLBACK(on_remove_col_clicked), cols_treeview);
    gtk_box_pack_start(GTK_BOX(hbox), add_col_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), remove_col_button, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(content_area), hbox);


    gtk_widget_show_all(dialog);

    gint result = gtk_dialog_run(GTK_DIALOG(dialog));
    if (result == GTK_RESPONSE_ACCEPT) {
        const gchar *table_name = gtk_entry_get_text(GTK_ENTRY(table_name_entry));
        if (strlen(table_name) > 0) {
            if (db_create_table(data->db, table_name, cols_liststore) == 0) {
                // Success, so refresh the schema view
                gtk_tree_store_clear(data->schema_store);
                db_populate_schema_store(data->db, data->schema_store);
            }
        }
    }

    gtk_widget_destroy(dialog);
}


// Callback for the AI suggest button
void on_ai_suggest_clicked(GtkButton *button, AppWidgets *widgets) {
    GtkTextIter iter;
    gtk_text_buffer_get_iter_at_mark(widgets->buffer,
                                     &iter,
                                     gtk_text_buffer_get_insert(widgets->buffer));

    const char *suggestion = "\nfor (int i = 0; i < 10; i++) {\n    // Your code here\n}\n";

    gtk_text_buffer_insert(widgets->buffer, &iter, suggestion, -1);
}


// Function to load file content into the editor
void load_file_into_editor(const char *filepath, AppWidgets *widgets) {
    gchar *contents;
    gsize length;

    if (g_file_get_contents(filepath, &contents, &length, NULL)) {
        gtk_text_buffer_set_text(widgets->buffer, contents, length);
        g_free(contents);
    } else {
        gtk_text_buffer_set_text(widgets->buffer, "Could not load file.", -1);
    }
}


// Callback for when a row is activated in the tree view
void on_row_activated(GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column, AppWidgets *widgets) {
    GtkTreeModel *model;
    GtkTreeIter iter;

    model = gtk_tree_view_get_model(tree_view);
    if (gtk_tree_model_get_iter(model, &iter, path)) {
        gchar *filepath;
        gtk_tree_model_get(model, &iter, COLUMN_PATH, &filepath, -1);
        if (filepath && g_file_test(filepath, G_FILE_TEST_IS_REGULAR)) {
            load_file_into_editor(filepath, widgets);
        }
        g_free(filepath);
    }
}


// Function to populate the GtkTreeStore with files and directories
void populate_file_tree(GtkTreeStore *treestore, GtkTreeIter *parent, const char *path) {
    DIR *dir;
    struct dirent *entry;
    GtkTreeIter iter;

    if (!(dir = opendir(path))) {
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

        GdkPixbuf *pixbuf;
        if (entry->d_type == DT_DIR) {
            pixbuf = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(), "folder", 16, 0, NULL);
        } else {
            pixbuf = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(), "text-x-generic", 16, 0, NULL);
        }


        gtk_tree_store_append(treestore, &iter, parent);
        gtk_tree_store_set(treestore, &iter,
                           COLUMN_ICON, pixbuf,
                           COLUMN_TEXT, entry->d_name,
                           COLUMN_PATH, full_path,
                           -1);

        if (entry->d_type == DT_DIR) {
            populate_file_tree(treestore, &iter, full_path);
        }

        if (pixbuf) {
            g_object_unref(pixbuf);
        }
    }

    closedir(dir);
}


GtkWidget* ui_manager_build_main_window(sqlite3 *db, int argc, char *argv[]) {
    // Create the main window
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Codeless IDE");
    gtk_window_set_default_size(GTK_WINDOW(window), 1200, 800);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    AppWidgets *widgets = g_slice_new(AppWidgets);


    // --- FILE EXPLORER ---
    GtkWidget *scrolled_window_tree = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window_tree),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    GtkTreeStore *treestore = gtk_tree_store_new(NUM_COLUMNS, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING);
    const char *path = ".";
    if (argc > 1) {
        path = argv[1];
    }
    populate_file_tree(treestore, NULL, path);

    GtkWidget *treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(treestore));
    g_object_unref(treestore);

    GtkCellRenderer *renderer_pixbuf = gtk_cell_renderer_pixbuf_new();
    GtkTreeViewColumn *column_pixbuf = gtk_tree_view_column_new_with_attributes("Icon", renderer_pixbuf, "pixbuf", COLUMN_ICON, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column_pixbuf);

    GtkCellRenderer *renderer_text = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *column_text = gtk_tree_view_column_new_with_attributes("Name", renderer_text, "text", COLUMN_TEXT, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column_text);

    gtk_container_add(GTK_CONTAINER(scrolled_window_tree), treeview);


    // --- CODE EDITOR ---
    GtkWidget *scrolled_window_editor = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window_editor),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    widgets->editor = gtk_source_view_new();
    widgets->buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widgets->editor));
    gtk_container_add(GTK_CONTAINER(scrolled_window_editor), widgets->editor);

    g_signal_connect(treeview, "row-activated", G_CALLBACK(on_row_activated), widgets);


    // --- TERMINAL ---
    GtkWidget *terminal = vte_terminal_new();
    gchar *shell = g_strdup(g_environ_getenv(g_get_environ(), "SHELL"));
    gchar **envp = g_get_environ();

    vte_terminal_spawn_async(VTE_TERMINAL(terminal),
                            VTE_PTY_DEFAULT,
                            NULL,  // working_directory
                            (gchar *[]){ shell, NULL },
                            envp, // envv
                            G_SPAWN_DEFAULT,
                            NULL,  // child_setup
                            NULL,  // child_setup_data
                            NULL,  // child_setup_data_destroy
                            -1,    // timeout
                            NULL,  // cancellable
                            NULL,  // callback
                            NULL); // user_data

    g_free(shell);
    g_strfreev(envp);


    // --- AI BUTTONS ---
    GtkWidget *ai_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget *ai_button = gtk_button_new_with_label("AI Suggest");
    g_signal_connect(ai_button, "clicked", G_CALLBACK(on_ai_suggest_clicked), widgets);
    GtkWidget *debug_button = gtk_button_new_with_label("AI Debug");
    g_signal_connect(debug_button, "clicked", G_CALLBACK(on_debug_clicked), window);
    gtk_box_pack_start(GTK_BOX(ai_box), ai_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(ai_box), debug_button, FALSE, FALSE, 0);


    // --- LAYOUT ---
    // A box to hold the button and the editor
    GtkWidget *editor_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_pack_start(GTK_BOX(editor_box), ai_box, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(editor_box), scrolled_window_editor, TRUE, TRUE, 0);

    // Vertical pane for editor and terminal
    GtkWidget *vpaned = gtk_paned_new(GTK_ORIENTATION_VERTICAL);
    gtk_paned_add1(GTK_PANED(vpaned), editor_box);
    gtk_paned_add2(GTK_PANED(vpaned), terminal);
    gtk_paned_set_position(GTK_PANED(vpaned), 600);

    // --- LEFT SIDEBAR NOTEBOOK ---
    GtkWidget *notebook = gtk_notebook_new();
    GtkWidget *explorer_label = gtk_label_new("Explorer");
    GtkWidget *db_label = gtk_label_new("Database");

    // --- DATABASE VIEW ---
    GtkWidget *db_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

    GtkWidget *new_table_button = gtk_button_new_with_label("New Table");
    gtk_box_pack_start(GTK_BOX(db_box), new_table_button, FALSE, FALSE, 0);

    GtkWidget *scrolled_window_db = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window_db),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(db_box), scrolled_window_db, TRUE, TRUE, 0);

    GtkTreeStore *db_treestore = gtk_tree_store_new(1, G_TYPE_STRING); // Only one column for the name
    db_populate_schema_store(db, db_treestore);

    GtkWidget *db_treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(db_treestore));

    GtkCellRenderer *db_renderer = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *db_column = gtk_tree_view_column_new_with_attributes("Schema",
                                                                            db_renderer,
                                                                            "text", 0,
                                                                            NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(db_treeview), db_column);
    gtk_container_add(GTK_CONTAINER(scrolled_window_db), db_treeview);

    NewTableDialogData *dialog_data = g_slice_new(NewTableDialogData);
    dialog_data->parent_window = GTK_WINDOW(window);
    dialog_data->db = db;
    dialog_data->schema_store = db_treestore;
    g_signal_connect(new_table_button, "clicked", G_CALLBACK(on_new_table_clicked), dialog_data);

    DBWidgets *db_widgets = g_slice_new(DBWidgets);
    db_widgets->db = db;

    g_signal_connect(db_treeview, "row-activated", G_CALLBACK(on_schema_row_activated), db_widgets);


    // Add explorer to notebook
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), scrolled_window_tree, explorer_label);

    // --- DATA VIEW (placeholder) ---
    GtkWidget *scrolled_window_data = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window_data),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    GtkWidget *data_view_label = gtk_label_new("Select a table to view its data.");
    gtk_container_add(GTK_CONTAINER(scrolled_window_data), data_view_label);
    db_widgets->data_view_container = scrolled_window_data;

    // Vertical pane for schema and data views
    GtkWidget *db_vpaned = gtk_paned_new(GTK_ORIENTATION_VERTICAL);
    gtk_paned_add1(GTK_PANED(db_vpaned), db_box);
    gtk_paned_add2(GTK_PANED(db_vpaned), scrolled_window_data);
    gtk_paned_set_position(GTK_PANED(db_vpaned), 300);

    // Add database view to notebook
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), db_vpaned, db_label);

    // Horizontal pane for the notebook and the editor/terminal pane
    GtkWidget *hpaned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_paned_add1(GTK_PANED(hpaned), notebook);
    gtk_paned_add2(GTK_PANED(hpaned), vpaned);
    gtk_paned_set_position(GTK_PANED(hpaned), 250);

    gtk_container_add(GTK_CONTAINER(window), hpaned);

    return window;
}
