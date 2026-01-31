// Menu bar implementation for retropad
#include "menu.h"
#include "app_state.h"
#include "file_ops.h"
#include "undo.h"
#include "find_replace.h"
#include "dialogs.h"
#include "print.h"
#include "utils.h"
#include "editor.h"

/* ===== Menu Callbacks ===== */

static void on_menu_file_new(GtkWidget *widget, gpointer user_data) {
    (void)widget; (void)user_data;
    DoFileNew();
}

static void on_menu_file_open(GtkWidget *widget, gpointer user_data) {
    (void)widget; (void)user_data;
    DoFileOpen();
}

static void on_menu_file_save(GtkWidget *widget, gpointer user_data) {
    (void)widget; (void)user_data;
    DoFileSave(FALSE);
}

static void on_menu_file_save_as(GtkWidget *widget, gpointer user_data) {
    (void)widget; (void)user_data;
    DoFileSave(TRUE);
}

static void on_menu_file_quit(GtkWidget *widget, gpointer user_data) {
    (void)widget; (void)user_data;
    gtk_window_close(GTK_WINDOW(g_app.window));
}

static void on_menu_edit_undo(GtkWidget *widget, gpointer user_data) {
    (void)widget; (void)user_data;
    DoUndo();
}

static void on_menu_edit_redo(GtkWidget *widget, gpointer user_data) {
    (void)widget; (void)user_data;
    DoRedo();
}

static void on_menu_edit_cut(GtkWidget *widget, gpointer user_data) {
    (void)widget; (void)user_data;
    GtkClipboard *clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
    gtk_text_buffer_cut_clipboard(g_app.textBuffer, clipboard, TRUE);
}

static void on_menu_edit_copy(GtkWidget *widget, gpointer user_data) {
    (void)widget; (void)user_data;
    GtkClipboard *clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
    gtk_text_buffer_copy_clipboard(g_app.textBuffer, clipboard);
}

static void on_menu_edit_paste(GtkWidget *widget, gpointer user_data) {
    (void)widget; (void)user_data;
    GtkClipboard *clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
    gtk_text_buffer_paste_clipboard(g_app.textBuffer, clipboard, NULL, TRUE);
}

static void on_menu_edit_delete(GtkWidget *widget, gpointer user_data) {
    (void)widget; (void)user_data;
    gtk_text_buffer_delete_selection(g_app.textBuffer, TRUE, TRUE);
}

static void on_menu_edit_select_all(GtkWidget *widget, gpointer user_data) {
    (void)widget; (void)user_data;
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(g_app.textBuffer, &start, &end);
    gtk_text_buffer_select_range(g_app.textBuffer, &start, &end);
}

static void on_menu_edit_find(GtkWidget *widget, gpointer user_data) {
    (void)widget; (void)user_data;
    ShowFindBar();
}

static void on_menu_edit_replace(GtkWidget *widget, gpointer user_data) {
    (void)widget; (void)user_data;
    ShowReplaceBar();
}

static void on_menu_edit_time_date(GtkWidget *widget, gpointer user_data) {
    (void)widget; (void)user_data;
    InsertTimeDate();
}

static void on_menu_format_word_wrap(GtkWidget *widget, gpointer user_data) {
    (void)user_data;
    gboolean active = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget));
    SetWordWrap(active);
}

static void on_menu_format_font(GtkWidget *widget, gpointer user_data) {
    (void)widget; (void)user_data;
    DoSelectFont();
}

static void on_menu_view_status_bar(GtkWidget *widget, gpointer user_data) {
    (void)user_data;
    gboolean active = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget));
    ToggleStatusBar(active);
}

static void on_menu_view_line_numbers(GtkWidget *widget, gpointer user_data) {
    (void)user_data;
    gboolean active = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget));
    ToggleLineNumbers(active);
}

static void on_menu_edit_goto(GtkWidget *widget, gpointer user_data) {
    (void)widget; (void)user_data;
    DoGotoLine();
}

static void on_menu_file_print(GtkWidget *widget, gpointer user_data) {
    (void)widget; (void)user_data;
    DoPrint();
}

static void on_menu_file_page_setup(GtkWidget *widget, gpointer user_data) {
    (void)widget; (void)user_data;
    DoPageSetup();
}

static void on_menu_help_about(GtkWidget *widget, gpointer user_data) {
    (void)widget; (void)user_data;
    ShowAboutDialog();
}

GtkWidget* CreateMenuBar(GtkAccelGroup *accelGroup) {
    GtkWidget *menubar = gtk_menu_bar_new();

    // File menu
    GtkWidget *fileMenu = gtk_menu_new();
    GtkWidget *fileItem = gtk_menu_item_new_with_mnemonic("_File");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(fileItem), fileMenu);

    GtkWidget *newItem = gtk_menu_item_new_with_mnemonic("_New");
    g_signal_connect(newItem, "activate", G_CALLBACK(on_menu_file_new), NULL);
    gtk_widget_add_accelerator(newItem, "activate", accelGroup, GDK_KEY_n, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), newItem);

    GtkWidget *openItem = gtk_menu_item_new_with_mnemonic("_Open");
    g_signal_connect(openItem, "activate", G_CALLBACK(on_menu_file_open), NULL);
    gtk_widget_add_accelerator(openItem, "activate", accelGroup, GDK_KEY_o, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), openItem);

    GtkWidget *saveItem = gtk_menu_item_new_with_mnemonic("_Save");
    g_signal_connect(saveItem, "activate", G_CALLBACK(on_menu_file_save), NULL);
    gtk_widget_add_accelerator(saveItem, "activate", accelGroup, GDK_KEY_s, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), saveItem);

    GtkWidget *saveAsItem = gtk_menu_item_new_with_mnemonic("Save _As");
    g_signal_connect(saveAsItem, "activate", G_CALLBACK(on_menu_file_save_as), NULL);
    gtk_widget_add_accelerator(saveAsItem, "activate", accelGroup, GDK_KEY_s, GDK_CONTROL_MASK | GDK_SHIFT_MASK, GTK_ACCEL_VISIBLE);
    gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), saveAsItem);

    gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), gtk_separator_menu_item_new());

    GtkWidget *pageSetupItem = gtk_menu_item_new_with_mnemonic("Page Set_up...");
    g_signal_connect(pageSetupItem, "activate", G_CALLBACK(on_menu_file_page_setup), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), pageSetupItem);

    GtkWidget *printItem = gtk_menu_item_new_with_mnemonic("_Print...");
    g_signal_connect(printItem, "activate", G_CALLBACK(on_menu_file_print), NULL);
    gtk_widget_add_accelerator(printItem, "activate", accelGroup, GDK_KEY_p, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), printItem);

    gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), gtk_separator_menu_item_new());

    /* Recent Files submenu */
    GtkWidget *recentItem = gtk_menu_item_new_with_mnemonic("_Recent Files");
    g_app.recentFilesMenu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(recentItem), g_app.recentFilesMenu);
    gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), recentItem);

    gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), gtk_separator_menu_item_new());

    GtkWidget *exitItem = gtk_menu_item_new_with_mnemonic("E_xit");
    g_signal_connect(exitItem, "activate", G_CALLBACK(on_menu_file_quit), NULL);
    gtk_widget_add_accelerator(exitItem, "activate", accelGroup, GDK_KEY_q, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), exitItem);

    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), fileItem);

    // Edit menu
    GtkWidget *editMenu = gtk_menu_new();
    GtkWidget *editItem = gtk_menu_item_new_with_mnemonic("_Edit");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(editItem), editMenu);

    GtkWidget *undoItem = gtk_menu_item_new_with_mnemonic("_Undo");
    g_signal_connect(undoItem, "activate", G_CALLBACK(on_menu_edit_undo), NULL);
    gtk_widget_add_accelerator(undoItem, "activate", accelGroup, GDK_KEY_z, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_menu_shell_append(GTK_MENU_SHELL(editMenu), undoItem);

    GtkWidget *redoItem = gtk_menu_item_new_with_mnemonic("_Redo");
    g_signal_connect(redoItem, "activate", G_CALLBACK(on_menu_edit_redo), NULL);
    gtk_widget_add_accelerator(redoItem, "activate", accelGroup, GDK_KEY_y, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_menu_shell_append(GTK_MENU_SHELL(editMenu), redoItem);

    gtk_menu_shell_append(GTK_MENU_SHELL(editMenu), gtk_separator_menu_item_new());

    GtkWidget *cutItem = gtk_menu_item_new_with_mnemonic("Cu_t");
    g_signal_connect(cutItem, "activate", G_CALLBACK(on_menu_edit_cut), NULL);
    gtk_widget_add_accelerator(cutItem, "activate", accelGroup, GDK_KEY_x, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_menu_shell_append(GTK_MENU_SHELL(editMenu), cutItem);

    GtkWidget *copyItem = gtk_menu_item_new_with_mnemonic("_Copy");
    g_signal_connect(copyItem, "activate", G_CALLBACK(on_menu_edit_copy), NULL);
    gtk_widget_add_accelerator(copyItem, "activate", accelGroup, GDK_KEY_c, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_menu_shell_append(GTK_MENU_SHELL(editMenu), copyItem);

    GtkWidget *pasteItem = gtk_menu_item_new_with_mnemonic("_Paste");
    g_signal_connect(pasteItem, "activate", G_CALLBACK(on_menu_edit_paste), NULL);
    gtk_widget_add_accelerator(pasteItem, "activate", accelGroup, GDK_KEY_v, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_menu_shell_append(GTK_MENU_SHELL(editMenu), pasteItem);

    GtkWidget *deleteItem = gtk_menu_item_new_with_mnemonic("_Delete");
    g_signal_connect(deleteItem, "activate", G_CALLBACK(on_menu_edit_delete), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(editMenu), deleteItem);

    gtk_menu_shell_append(GTK_MENU_SHELL(editMenu), gtk_separator_menu_item_new());

    GtkWidget *selectAllItem = gtk_menu_item_new_with_mnemonic("Select _All");
    g_signal_connect(selectAllItem, "activate", G_CALLBACK(on_menu_edit_select_all), NULL);
    gtk_widget_add_accelerator(selectAllItem, "activate", accelGroup, GDK_KEY_a, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_menu_shell_append(GTK_MENU_SHELL(editMenu), selectAllItem);

    gtk_menu_shell_append(GTK_MENU_SHELL(editMenu), gtk_separator_menu_item_new());

    GtkWidget *findItem = gtk_menu_item_new_with_mnemonic("_Find");
    g_signal_connect(findItem, "activate", G_CALLBACK(on_menu_edit_find), NULL);
    gtk_widget_add_accelerator(findItem, "activate", accelGroup, GDK_KEY_f, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_menu_shell_append(GTK_MENU_SHELL(editMenu), findItem);

    GtkWidget *replaceItem = gtk_menu_item_new_with_mnemonic("_Replace");
    g_signal_connect(replaceItem, "activate", G_CALLBACK(on_menu_edit_replace), NULL);
    gtk_widget_add_accelerator(replaceItem, "activate", accelGroup, GDK_KEY_h, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_menu_shell_append(GTK_MENU_SHELL(editMenu), replaceItem);

    GtkWidget *gotoItem = gtk_menu_item_new_with_mnemonic("_Go To...");
    g_signal_connect(gotoItem, "activate", G_CALLBACK(on_menu_edit_goto), NULL);
    gtk_widget_add_accelerator(gotoItem, "activate", accelGroup, GDK_KEY_g, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_menu_shell_append(GTK_MENU_SHELL(editMenu), gotoItem);

    gtk_menu_shell_append(GTK_MENU_SHELL(editMenu), gtk_separator_menu_item_new());

    GtkWidget *timeDateItem = gtk_menu_item_new_with_mnemonic("Time/Date");
    g_signal_connect(timeDateItem, "activate", G_CALLBACK(on_menu_edit_time_date), NULL);
    gtk_widget_add_accelerator(timeDateItem, "activate", accelGroup, GDK_KEY_F5, 0, GTK_ACCEL_VISIBLE);
    gtk_menu_shell_append(GTK_MENU_SHELL(editMenu), timeDateItem);

    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), editItem);

    // Format menu
    GtkWidget *formatMenu = gtk_menu_new();
    GtkWidget *formatItem = gtk_menu_item_new_with_mnemonic("F_ormat");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(formatItem), formatMenu);

    GtkWidget *wordWrapItem = gtk_check_menu_item_new_with_mnemonic("_Word Wrap");
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(wordWrapItem), TRUE);
    g_app.wordWrapMenuItem = wordWrapItem;
    g_signal_connect(wordWrapItem, "toggled", G_CALLBACK(on_menu_format_word_wrap), NULL);
    gtk_widget_add_accelerator(wordWrapItem, "activate", accelGroup, GDK_KEY_w, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_menu_shell_append(GTK_MENU_SHELL(formatMenu), wordWrapItem);

    GtkWidget *fontItem = gtk_menu_item_new_with_mnemonic("_Font");
    g_signal_connect(fontItem, "activate", G_CALLBACK(on_menu_format_font), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(formatMenu), fontItem);

    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), formatItem);

    // View menu
    GtkWidget *viewMenu = gtk_menu_new();
    GtkWidget *viewItem = gtk_menu_item_new_with_mnemonic("_View");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(viewItem), viewMenu);

    GtkWidget *statusBarItem = gtk_check_menu_item_new_with_mnemonic("_Status Bar");
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(statusBarItem), TRUE);
    g_app.statusBarMenuItem = statusBarItem;
    g_signal_connect(statusBarItem, "toggled", G_CALLBACK(on_menu_view_status_bar), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(viewMenu), statusBarItem);

    GtkWidget *lineNumbersItem = gtk_check_menu_item_new_with_mnemonic("_Line Numbers");
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(lineNumbersItem), FALSE);
    g_app.lineNumbersMenuItem = lineNumbersItem;
    g_signal_connect(lineNumbersItem, "toggled", G_CALLBACK(on_menu_view_line_numbers), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(viewMenu), lineNumbersItem);

    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), viewItem);

    // Help menu
    GtkWidget *helpMenu = gtk_menu_new();
    GtkWidget *helpItem = gtk_menu_item_new_with_mnemonic("_Help");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(helpItem), helpMenu);

    GtkWidget *aboutItem = gtk_menu_item_new_with_mnemonic("_About");
    g_signal_connect(aboutItem, "activate", G_CALLBACK(on_menu_help_about), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(helpMenu), aboutItem);

    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), helpItem);

    gtk_widget_show_all(menubar);
    return menubar;
}

void CreateFindReplaceBar(GtkWidget *vbox) {
    // Create find bar
    g_app.findBar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(g_app.findBar), 5);
    g_app.findEntry = gtk_entry_new();
    g_signal_connect(g_app.findEntry, "key-press-event", G_CALLBACK(on_find_entry_key_press), NULL);
    GtkWidget *findBtn = gtk_button_new_with_label("Find Next");
    GtkWidget *prevBtn = gtk_button_new_with_label("Find Previous");
    g_signal_connect(findBtn, "clicked", G_CALLBACK(on_find_next), NULL);
    g_signal_connect(prevBtn, "clicked", G_CALLBACK(on_find_previous), NULL);

    gtk_box_pack_start(GTK_BOX(g_app.findBar), gtk_label_new("Find:"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(g_app.findBar), g_app.findEntry, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(g_app.findBar), findBtn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(g_app.findBar), prevBtn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), g_app.findBar, FALSE, FALSE, 0);
    gtk_widget_hide(g_app.findBar);

    // Create replace bar
    g_app.replaceBar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(g_app.replaceBar), 5);
    g_app.replaceEntry = gtk_entry_new();
    g_signal_connect(g_app.replaceEntry, "key-press-event", G_CALLBACK(on_replace_entry_key_press), NULL);
    GtkWidget *replaceAllBtn = gtk_button_new_with_label("Replace All");
    g_signal_connect(replaceAllBtn, "clicked", G_CALLBACK(on_replace_all), NULL);

    gtk_box_pack_start(GTK_BOX(g_app.replaceBar), gtk_label_new("Replace:"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(g_app.replaceBar), g_app.replaceEntry, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(g_app.replaceBar), replaceAllBtn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), g_app.replaceBar, FALSE, FALSE, 0);
    gtk_widget_hide(g_app.replaceBar);
}
