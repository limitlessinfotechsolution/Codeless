#include <gtk/gtk.h>
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

// Struct to hold pointers to the widgets
typedef struct {
    GtkWidget *editor;
    GtkTextBuffer *buffer;
} AppWidgets;


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


int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

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
    populate_file_tree(treestore, NULL, ".");

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


    // --- AI SUGGEST BUTTON ---
    GtkWidget *ai_button = gtk_button_new_with_label("AI Suggest");
    g_signal_connect(ai_button, "clicked", G_CALLBACK(on_ai_suggest_clicked), widgets);

    // --- LAYOUT ---
    // A box to hold the button and the editor
    GtkWidget *editor_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_pack_start(GTK_BOX(editor_box), ai_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(editor_box), scrolled_window_editor, TRUE, TRUE, 0);

    // Vertical pane for editor and terminal
    GtkWidget *vpaned = gtk_paned_new(GTK_ORIENTATION_VERTICAL);
    gtk_paned_add1(GTK_PANED(vpaned), editor_box);
    gtk_paned_add2(GTK_PANED(vpaned), terminal);
    gtk_paned_set_position(GTK_PANED(vpaned), 600);

    // Horizontal pane for file explorer and the editor/terminal pane
    GtkWidget *hpaned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_paned_add1(GTK_PANED(hpaned), scrolled_window_tree);
    gtk_paned_add2(GTK_PANED(hpaned), vpaned);
    gtk_paned_set_position(GTK_PANED(hpaned), 250);

    gtk_container_add(GTK_CONTAINER(window), hpaned);

    gtk_widget_show_all(window);
    gtk_main();

    g_slice_free(AppWidgets, widgets);

    return 0;
}
