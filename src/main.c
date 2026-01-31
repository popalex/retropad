// retropad - A Petzold-style notepad clone for Linux
// Main entry point

#include <gtk/gtk.h>
#include "app_state.h"
#include "menu.h"
#include "editor.h"
#include "recent_files.h"
#include "utils.h"
#include "file_ops.h"

static gboolean on_window_delete(GtkWidget *widget, GdkEvent *event, gpointer user_data) {
    (void)widget;
    (void)event;
    (void)user_data;
    if (!PromptSaveChanges()) {
        return TRUE;
    }
    gtk_main_quit();
    return FALSE;
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    /* Initialize application state */
    AppStateInit();

    /* Create main window */
    g_app.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(g_app.window), APP_TITLE);
    gtk_window_set_default_size(GTK_WINDOW(g_app.window), DEFAULT_WIDTH, DEFAULT_HEIGHT);
    g_signal_connect(g_app.window, "delete-event", G_CALLBACK(on_window_delete), NULL);

    /* Create accelerator group */
    GtkAccelGroup *accelGroup = gtk_accel_group_new();
    gtk_window_add_accel_group(GTK_WINDOW(g_app.window), accelGroup);

    /* Create VBox for layout */
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(g_app.window), vbox);

    /* Create menu bar */
    GtkWidget *menubar = CreateMenuBar(accelGroup);
    gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);

    /* Create editor widget */
    GtkWidget *editorBox = CreateEditorWidget();
    gtk_box_pack_start(GTK_BOX(vbox), editorBox, TRUE, TRUE, 0);

    /* Create find/replace bars */
    CreateFindReplaceBar(vbox);

    /* Create status bar */
    g_app.statusbar = gtk_statusbar_new();
    g_statusbar_context = gtk_statusbar_get_context_id(GTK_STATUSBAR(g_app.statusbar), "main");
    gtk_box_pack_start(GTK_BOX(vbox), g_app.statusbar, FALSE, FALSE, 0);

    /* Load recent files and update menu */
    LoadRecentFiles();
    UpdateRecentFilesMenu();

    /* Setup drag and drop */
    SetupDragAndDrop();

    UpdateTitle();
    UpdateStatusBar();

    gtk_widget_show_all(g_app.window);
    gtk_widget_hide(g_app.findBar);
    gtk_widget_hide(g_app.replaceBar);
    gtk_widget_hide(g_app.lineNumberView);

    gtk_main();

    /* Cleanup */
    AppStateCleanup();

    return 0;
}
