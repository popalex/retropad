#include <gtk/gtk.h>
#include <glib/gstdio.h>
#include <string.h>
#include <time.h>
#include "file_io.h"

#define APP_TITLE "retropad"
#define UNTITLED_NAME "Untitled"
#define MAX_PATH_BUFFER 1024
#define DEFAULT_WIDTH 640
#define DEFAULT_HEIGHT 480
#define MAX_UNDO_STACK 100

typedef struct UndoRedoEntry {
    char *text;
    gint cursorPos;
} UndoRedoEntry;

typedef struct AppState {
    GtkWidget *window;
    GtkWidget *textView;
    GtkWidget *statusbar;
    GtkTextBuffer *textBuffer;
    PangoFontDescription *fontDesc;
    char currentPath[MAX_PATH_BUFFER];
    gboolean wordWrap;
    gboolean statusVisible;
    gboolean modified;
    TextEncoding encoding;
    GtkWidget *findBar;
    GtkWidget *findEntry;
    GtkWidget *replaceBar;
    GtkWidget *replaceEntry;
    gboolean matchCase;
    gboolean searchDown;
    /* Undo/Redo stack */
    GQueue *undoStack;
    GQueue *redoStack;
    gboolean isUndoRedoInProgress;
} AppState;

static AppState g_app = {0};
static guint g_statusbar_context = 0;

static void PushUndoStack(void);
static void ClearRedoStack(void);
static void DoUndo(void);
static void DoRedo(void);
static void UpdateTitle(void);
static void UpdateStatusBar(void);
static gboolean PromptSaveChanges(void);
static void DoFileNew(void);
static void DoFileOpen(void);
static gboolean DoFileSave(gboolean saveAs);
static void SetWordWrap(gboolean enabled);
static void ToggleStatusBar(gboolean visible);
static void ShowFindBar(void);
static void ShowReplaceBar(void);
static gboolean DoFindNext(gboolean reverse);
static void DoSelectFont(void);
static void InsertTimeDate(void);
static gboolean LoadDocumentFromPath(const char *path);

static UndoRedoEntry* CreateUndoEntry(void) {
    UndoRedoEntry *entry = g_new(UndoRedoEntry, 1);
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(g_app.textBuffer, &start, &end);
    entry->text = gtk_text_buffer_get_text(g_app.textBuffer, &start, &end, FALSE);
    
    GtkTextIter cursor;
    gtk_text_buffer_get_iter_at_mark(g_app.textBuffer,
        &cursor, gtk_text_buffer_get_insert(g_app.textBuffer));
    entry->cursorPos = gtk_text_iter_get_offset(&cursor);
    
    return entry;
}

static void FreeUndoEntry(gpointer data) {
    UndoRedoEntry *entry = (UndoRedoEntry *)data;
    if (entry) {
        g_free(entry->text);
        g_free(entry);
    }
}

static void PushUndoStack(void) {
    if (g_app.isUndoRedoInProgress) return;
    
    /* Limit undo stack size */
    while (g_queue_get_length(g_app.undoStack) >= MAX_UNDO_STACK) {
        FreeUndoEntry(g_queue_pop_head(g_app.undoStack));
    }
    
    g_queue_push_tail(g_app.undoStack, CreateUndoEntry());
}

static void ClearRedoStack(void) {
    g_queue_foreach(g_app.redoStack, (GFunc)FreeUndoEntry, NULL);
    g_queue_clear(g_app.redoStack);
}

static void DoUndo(void) {
    if (g_queue_is_empty(g_app.undoStack)) return;
    
    g_app.isUndoRedoInProgress = TRUE;
    
    /* Save current state to redo stack */
    g_queue_push_tail(g_app.redoStack, CreateUndoEntry());
    
    /* Pop and restore from undo stack */
    UndoRedoEntry *entry = (UndoRedoEntry *)g_queue_pop_tail(g_app.undoStack);
    if (entry) {
        gtk_text_buffer_set_text(g_app.textBuffer, entry->text, -1);
        
        GtkTextIter cursor;
        gtk_text_buffer_get_iter_at_offset(g_app.textBuffer, &cursor, entry->cursorPos);
        gtk_text_buffer_place_cursor(g_app.textBuffer, &cursor);
        gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(g_app.textView), &cursor, 0, FALSE, 0, 0);
        
        FreeUndoEntry(entry);
    }
    
    g_app.isUndoRedoInProgress = FALSE;
    UpdateStatusBar();
}

static void DoRedo(void) {
    if (g_queue_is_empty(g_app.redoStack)) return;
    
    g_app.isUndoRedoInProgress = TRUE;
    
    /* Save current state to undo stack */
    g_queue_push_tail(g_app.undoStack, CreateUndoEntry());
    
    /* Pop and restore from redo stack */
    UndoRedoEntry *entry = (UndoRedoEntry *)g_queue_pop_tail(g_app.redoStack);
    if (entry) {
        gtk_text_buffer_set_text(g_app.textBuffer, entry->text, -1);
        
        GtkTextIter cursor;
        gtk_text_buffer_get_iter_at_offset(g_app.textBuffer, &cursor, entry->cursorPos);
        gtk_text_buffer_place_cursor(g_app.textBuffer, &cursor);
        gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(g_app.textView), &cursor, 0, FALSE, 0, 0);
        
        FreeUndoEntry(entry);
    }
    
    g_app.isUndoRedoInProgress = FALSE;
    UpdateStatusBar();
}

static void UpdateTitle(void) {
    char name[MAX_PATH_BUFFER];
    if (g_app.currentPath[0]) {
        const char *fileName = g_app.currentPath;
        const char *slash = strrchr(g_app.currentPath, '/');
        if (slash) fileName = slash + 1;
        strncpy(name, fileName, MAX_PATH_BUFFER - 1);
    } else {
        strncpy(name, UNTITLED_NAME, MAX_PATH_BUFFER - 1);
    }
    name[MAX_PATH_BUFFER - 1] = '\0';

    char title[MAX_PATH_BUFFER + 32];
    snprintf(title, sizeof(title), "%s%s - %s",
             (g_app.modified ? "*" : ""), name, APP_TITLE);
    gtk_window_set_title(GTK_WINDOW(g_app.window), title);
}

static void UpdateStatusBar(void) {
    if (!g_app.statusVisible) return;

    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(g_app.textBuffer, &start, &end);
    gint totalLines = gtk_text_iter_get_line(&end) + 1;

    GtkTextIter cursor;
    gtk_text_buffer_get_iter_at_mark(g_app.textBuffer,
        &cursor, gtk_text_buffer_get_insert(g_app.textBuffer));
    gint line = gtk_text_iter_get_line(&cursor) + 1;
    gint col = gtk_text_iter_get_line_offset(&cursor) + 1;

    char status[128];
    snprintf(status, sizeof(status), "Ln %d, Col %d    Lines: %d",
             line, col, totalLines);

    gtk_statusbar_pop(GTK_STATUSBAR(g_app.statusbar), g_statusbar_context);
    gtk_statusbar_push(GTK_STATUSBAR(g_app.statusbar), g_statusbar_context, status);
}

static gboolean GetEditText(char **bufferOut, int *lengthOut) {
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(g_app.textBuffer, &start, &end);
    char *text = gtk_text_buffer_get_text(g_app.textBuffer, &start, &end, FALSE);
    if (!text) return FALSE;

    int len = strlen(text);
    if (lengthOut) *lengthOut = len;
    *bufferOut = text;
    return TRUE;
}

static gboolean FindInEdit(const char *needle, gboolean matchCase, gboolean searchDown,
                          GtkTextIter *outStart, GtkTextIter *outEnd) {
    if (!needle || needle[0] == '\0') return FALSE;

    char *text = NULL;
    int len = 0;
    if (!GetEditText(&text, &len)) return FALSE;

    char *haystack = text;
    char *needleBuf = g_strdup(needle);

    if (!matchCase) {
        char *p = haystack;
        while (*p) {
            *p = g_ascii_tolower(*p);
            p++;
        }
        p = needleBuf;
        while (*p) {
            *p = g_ascii_tolower(*p);
            p++;
        }
    }

    GtkTextIter cursor;
    gtk_text_buffer_get_iter_at_mark(g_app.textBuffer,
        &cursor, gtk_text_buffer_get_insert(g_app.textBuffer));
    gint searchPos = gtk_text_iter_get_offset(&cursor);
    if (!searchDown) searchPos = 0;

    char *found = strstr(haystack + searchPos, needleBuf);
    if (!found && searchDown) {
        found = strstr(haystack, needleBuf);
    }

    gboolean result = FALSE;
    if (found) {
        gint pos = found - haystack;
        gtk_text_buffer_get_iter_at_offset(g_app.textBuffer, outStart, pos);
        gtk_text_buffer_get_iter_at_offset(g_app.textBuffer, outEnd,
                                          pos + strlen(needle));
        result = TRUE;
    }

    g_free(text);
    g_free(needleBuf);
    return result;
}

static int ReplaceAllOccurrences(const char *needle, const char *replacement,
                                gboolean matchCase) {
    if (!needle || needle[0] == '\0') return 0;

    char *text = NULL;
    int len = 0;
    if (!GetEditText(&text, &len)) return 0;

    char *searchBuf = g_strdup(text);
    char *needleBuf = g_strdup(needle);

    if (!matchCase) {
        char *p = searchBuf;
        while (*p) {
            *p = g_ascii_tolower(*p);
            p++;
        }
        p = needleBuf;
        while (*p) {
            *p = g_ascii_tolower(*p);
            p++;
        }
    }

    int count = 0;
    char *p = searchBuf;
    while ((p = strstr(p, needleBuf)) != NULL) {
        count++;
        p += strlen(needle);
    }

    if (count == 0) {
        g_free(text);
        g_free(searchBuf);
        g_free(needleBuf);
        return 0;
    }

    GString *result = g_string_new("");
    p = searchBuf;
    const char *orig = text;
    
    while ((p = strstr(p, needleBuf)) != NULL) {
        int delta = p - searchBuf;
        g_string_append_len(result, orig, delta);
        if (replacement) {
            g_string_append(result, replacement);
        }
        orig += delta + strlen(needle);
        searchBuf += delta + strlen(needleBuf);
    }
    g_string_append(result, orig);

    gtk_text_buffer_set_text(g_app.textBuffer, result->str, -1);
    g_string_free(result, TRUE);
    g_free(text);
    g_free(searchBuf);
    g_free(needleBuf);
    
    g_app.modified = TRUE;
    UpdateTitle();
    return count;
}

static void DoFileNew(void) {
    if (!PromptSaveChanges()) return;
    gtk_text_buffer_set_text(g_app.textBuffer, "", -1);
    g_app.currentPath[0] = '\0';
    g_app.encoding = ENC_UTF8;
    g_app.modified = FALSE;
    ClearRedoStack();
    g_queue_foreach(g_app.undoStack, (GFunc)FreeUndoEntry, NULL);
    g_queue_clear(g_app.undoStack);
    UpdateTitle();
    UpdateStatusBar();
}

static void DoFileOpen(void) {
    if (!PromptSaveChanges()) return;

    GtkWidget *dialog = gtk_file_chooser_dialog_new(
        "Open File", GTK_WINDOW(g_app.window),
        GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Open", GTK_RESPONSE_ACCEPT,
        NULL);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        if (path) {
            LoadDocumentFromPath(path);
            g_free(path);
        }
    }
    gtk_widget_destroy(dialog);
}

static gboolean DoFileSave(gboolean saveAs) {
    char path[MAX_PATH_BUFFER];

    if (saveAs || g_app.currentPath[0] == '\0') {
        GtkWidget *dialog = gtk_file_chooser_dialog_new(
            "Save File", GTK_WINDOW(g_app.window),
            GTK_FILE_CHOOSER_ACTION_SAVE,
            "_Cancel", GTK_RESPONSE_CANCEL,
            "_Save", GTK_RESPONSE_ACCEPT,
            NULL);

        gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);

        if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
            char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
            if (filename) {
                strncpy(path, filename, MAX_PATH_BUFFER - 1);
                g_free(filename);
            } else {
                gtk_widget_destroy(dialog);
                return FALSE;
            }
        } else {
            gtk_widget_destroy(dialog);
            return FALSE;
        }
        gtk_widget_destroy(dialog);
        strncpy(g_app.currentPath, path, MAX_PATH_BUFFER - 1);
    } else {
        strncpy(path, g_app.currentPath, MAX_PATH_BUFFER - 1);
    }

    char *text = NULL;
    int len = 0;
    if (!GetEditText(&text, &len)) return FALSE;

    gboolean ok = SaveTextFile(NULL, path, text, len, g_app.encoding);
    g_free(text);

    if (ok) {
        g_app.modified = FALSE;
        UpdateTitle();
    }
    return ok;
}

static gboolean LoadDocumentFromPath(const char *path) {
    char *text = NULL;
    TextEncoding enc = ENC_UTF8;
    if (!LoadTextFile(NULL, path, &text, NULL, &enc)) {
        return FALSE;
    }

    gtk_text_buffer_set_text(g_app.textBuffer, text, -1);
    g_free(text);
    strncpy(g_app.currentPath, path, MAX_PATH_BUFFER - 1);
    g_app.encoding = enc;
    g_app.modified = FALSE;
    ClearRedoStack();
    g_queue_foreach(g_app.undoStack, (GFunc)FreeUndoEntry, NULL);
    g_queue_clear(g_app.undoStack);
    UpdateTitle();
    UpdateStatusBar();
    return TRUE;
}

static gboolean PromptSaveChanges(void) {
    if (!g_app.modified) return TRUE;

    GtkWidget *dialog = gtk_message_dialog_new(
        GTK_WINDOW(g_app.window),
        GTK_DIALOG_MODAL,
        GTK_MESSAGE_QUESTION,
        GTK_BUTTONS_NONE,
        "Save changes to %s?",
        g_app.currentPath[0] ? g_app.currentPath : UNTITLED_NAME);

    gtk_dialog_add_buttons(GTK_DIALOG(dialog),
        "_Don't Save", GTK_RESPONSE_NO,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Save", GTK_RESPONSE_YES,
        NULL);

    gint res = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);

    if (res == GTK_RESPONSE_YES) {
        return DoFileSave(FALSE);
    }
    return res == GTK_RESPONSE_NO;
}

static void SetWordWrap(gboolean enabled) {
    if (g_app.wordWrap == enabled) return;
    g_app.wordWrap = enabled;

    if (enabled) {
        gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(g_app.textView), GTK_WRAP_WORD);
    } else {
        gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(g_app.textView), GTK_WRAP_NONE);
    }
}

static void ToggleStatusBar(gboolean visible) {
    g_app.statusVisible = visible;
    if (visible) {
        gtk_widget_show(g_app.statusbar);
    } else {
        gtk_widget_hide(g_app.statusbar);
    }
}

static gboolean DoFindNext(gboolean reverse) {
    const char *needle = gtk_entry_get_text(GTK_ENTRY(g_app.findEntry));
    if (!needle || needle[0] == '\0') {
        ShowFindBar();
        return FALSE;
    }

    GtkTextIter outStart, outEnd;
    if (FindInEdit(needle, g_app.matchCase, !reverse, &outStart, &outEnd)) {
        gtk_text_buffer_select_range(g_app.textBuffer, &outStart, &outEnd);
        gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(g_app.textView), &outStart, 0, FALSE, 0, 0);
        return TRUE;
    }

    GtkWidget *dialog = gtk_message_dialog_new(
        GTK_WINDOW(g_app.window),
        GTK_DIALOG_MODAL,
        GTK_MESSAGE_INFO,
        GTK_BUTTONS_OK,
        "Cannot find the text.");
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    return FALSE;
}

static void DoSelectFont(void) {
    GtkWidget *dialog = gtk_font_chooser_dialog_new(
        "Select Font", GTK_WINDOW(g_app.window));

    if (g_app.fontDesc) {
        gtk_font_chooser_set_font_desc(GTK_FONT_CHOOSER(dialog), g_app.fontDesc);
    }

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        PangoFontDescription *fontDesc =
            gtk_font_chooser_get_font_desc(GTK_FONT_CHOOSER(dialog));
        if (fontDesc) {
            if (g_app.fontDesc) {
                pango_font_description_free(g_app.fontDesc);
            }
            g_app.fontDesc = pango_font_description_copy(fontDesc);
            GtkCssProvider *provider = gtk_css_provider_new();
            gchar *font_name = pango_font_description_to_string(g_app.fontDesc);
            gchar *css = g_strdup_printf("textview { font: %s; }", font_name);
            gtk_css_provider_load_from_data(provider, css, -1, NULL);
            
            GtkStyleContext *context = gtk_widget_get_style_context(g_app.textView);
            gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
            
            g_free(css);
            g_free(font_name);
            g_object_unref(provider);
        }
    }
    gtk_widget_destroy(dialog);
}

static void InsertTimeDate(void) {
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char stamp[128];
    strftime(stamp, sizeof(stamp), "%X %x", tm_info);

    GtkTextIter cursor;
    gtk_text_buffer_get_iter_at_mark(g_app.textBuffer,
        &cursor, gtk_text_buffer_get_insert(g_app.textBuffer));
    gtk_text_buffer_insert(g_app.textBuffer, &cursor, stamp, -1);
}

static void ShowFindBar(void) {
    gtk_widget_show_all(g_app.findBar);
    gtk_widget_grab_focus(g_app.findEntry);
}

static void ShowReplaceBar(void) {
    gtk_widget_show_all(g_app.replaceBar);
    gtk_widget_grab_focus(g_app.replaceEntry);
}

static void on_text_changed(GtkTextBuffer *buffer, gpointer user_data) {
    if (!g_app.isUndoRedoInProgress) {
        PushUndoStack();
        ClearRedoStack();
    }
    g_app.modified = TRUE;
    UpdateTitle();
    UpdateStatusBar();
}

static void on_cursor_moved(GtkTextBuffer *buffer, GParamSpec *pspec, gpointer user_data) {
    UpdateStatusBar();
}

static gboolean on_window_delete(GtkWidget *widget, GdkEvent *event, gpointer user_data) {
    if (!PromptSaveChanges()) {
        return TRUE;
    }
    gtk_main_quit();
    return FALSE;
}

static void on_find_next(GtkWidget *widget, gpointer user_data) {
    DoFindNext(FALSE);
}

static void on_find_previous(GtkWidget *widget, gpointer user_data) {
    DoFindNext(TRUE);
}

static void on_replace_all(GtkWidget *widget, gpointer user_data) {
    const char *needle = gtk_entry_get_text(GTK_ENTRY(g_app.findEntry));
    const char *replacement = gtk_entry_get_text(GTK_ENTRY(g_app.replaceEntry));
    int replaced = ReplaceAllOccurrences(needle, replacement, g_app.matchCase);

    GtkWidget *dialog = gtk_message_dialog_new(
        GTK_WINDOW(g_app.window),
        GTK_DIALOG_MODAL,
        GTK_MESSAGE_INFO,
        GTK_BUTTONS_OK,
        "Replaced %d occurrence%s.",
        replaced, replaced == 1 ? "" : "s");
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

static void on_menu_file_new(GtkWidget *widget, gpointer user_data) {
    DoFileNew();
}

static void on_menu_file_open(GtkWidget *widget, gpointer user_data) {
    DoFileOpen();
}

static void on_menu_file_save(GtkWidget *widget, gpointer user_data) {
    DoFileSave(FALSE);
}

static void on_menu_file_save_as(GtkWidget *widget, gpointer user_data) {
    DoFileSave(TRUE);
}

static void on_menu_file_quit(GtkWidget *widget, gpointer user_data) {
    gtk_window_close(GTK_WINDOW(g_app.window));
}

static void on_menu_edit_undo(GtkWidget *widget, gpointer user_data) {
    // GTK3 GtkTextBuffer doesn't have undo/redo built-in
    // This would require GtkSourceView for undo support
    DoUndo();
}

static void on_menu_edit_redo(GtkWidget *widget, gpointer user_data) {
    DoRedo();
}

static void on_menu_edit_cut(GtkWidget *widget, gpointer user_data) {
    GtkClipboard *clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
    gtk_text_buffer_cut_clipboard(g_app.textBuffer, clipboard, TRUE);
}

static void on_menu_edit_copy(GtkWidget *widget, gpointer user_data) {
    GtkClipboard *clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
    gtk_text_buffer_copy_clipboard(g_app.textBuffer, clipboard);
}

static void on_menu_edit_paste(GtkWidget *widget, gpointer user_data) {
    GtkClipboard *clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
    gtk_text_buffer_paste_clipboard(g_app.textBuffer, clipboard, NULL, TRUE);
}

static void on_menu_edit_delete(GtkWidget *widget, gpointer user_data) {
    gtk_text_buffer_delete_selection(g_app.textBuffer, TRUE, TRUE);
}

static void on_menu_edit_select_all(GtkWidget *widget, gpointer user_data) {
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(g_app.textBuffer, &start, &end);
    gtk_text_buffer_select_range(g_app.textBuffer, &start, &end);
}

static void on_menu_edit_find(GtkWidget *widget, gpointer user_data) {
    ShowFindBar();
}

static void on_menu_edit_replace(GtkWidget *widget, gpointer user_data) {
    ShowReplaceBar();
}

static void on_menu_edit_time_date(GtkWidget *widget, gpointer user_data) {
    InsertTimeDate();
}

static void on_menu_format_word_wrap(GtkWidget *widget, gpointer user_data) {
    SetWordWrap(!g_app.wordWrap);
}

static void on_menu_format_font(GtkWidget *widget, gpointer user_data) {
    DoSelectFont();
}

static void on_menu_view_status_bar(GtkWidget *widget, gpointer user_data) {
    ToggleStatusBar(!g_app.statusVisible);
}

static void on_menu_help_about(GtkWidget *widget, gpointer user_data) {
    GtkWidget *dialog = gtk_message_dialog_new(
        GTK_WINDOW(g_app.window),
        GTK_DIALOG_MODAL,
        GTK_MESSAGE_INFO,
        GTK_BUTTONS_OK,
        "retropad\n\nA Petzold-style notepad clone for Linux");
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

static GtkWidget *CreateMenuBar(void) {
    GtkWidget *menubar = gtk_menu_bar_new();

    // File menu
    GtkWidget *fileMenu = gtk_menu_new();
    GtkWidget *fileItem = gtk_menu_item_new_with_mnemonic("_File");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(fileItem), fileMenu);

    GtkWidget *newItem = gtk_menu_item_new_with_mnemonic("_New");
    g_signal_connect(newItem, "activate", G_CALLBACK(on_menu_file_new), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), newItem);

    GtkWidget *openItem = gtk_menu_item_new_with_mnemonic("_Open");
    g_signal_connect(openItem, "activate", G_CALLBACK(on_menu_file_open), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), openItem);

    GtkWidget *saveItem = gtk_menu_item_new_with_mnemonic("_Save");
    g_signal_connect(saveItem, "activate", G_CALLBACK(on_menu_file_save), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), saveItem);

    GtkWidget *saveAsItem = gtk_menu_item_new_with_mnemonic("Save _As");
    g_signal_connect(saveAsItem, "activate", G_CALLBACK(on_menu_file_save_as), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), saveAsItem);

    gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), gtk_separator_menu_item_new());

    GtkWidget *exitItem = gtk_menu_item_new_with_mnemonic("E_xit");
    g_signal_connect(exitItem, "activate", G_CALLBACK(on_menu_file_quit), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), exitItem);

    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), fileItem);

    // Edit menu
    GtkWidget *editMenu = gtk_menu_new();
    GtkWidget *editItem = gtk_menu_item_new_with_mnemonic("_Edit");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(editItem), editMenu);

    GtkWidget *undoItem = gtk_menu_item_new_with_mnemonic("_Undo");
    g_signal_connect(undoItem, "activate", G_CALLBACK(on_menu_edit_undo), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(editMenu), undoItem);

    GtkWidget *redoItem = gtk_menu_item_new_with_mnemonic("_Redo");
    g_signal_connect(redoItem, "activate", G_CALLBACK(on_menu_edit_redo), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(editMenu), redoItem);

    gtk_menu_shell_append(GTK_MENU_SHELL(editMenu), gtk_separator_menu_item_new());

    GtkWidget *cutItem = gtk_menu_item_new_with_mnemonic("Cu_t");
    g_signal_connect(cutItem, "activate", G_CALLBACK(on_menu_edit_cut), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(editMenu), cutItem);

    GtkWidget *copyItem = gtk_menu_item_new_with_mnemonic("_Copy");
    g_signal_connect(copyItem, "activate", G_CALLBACK(on_menu_edit_copy), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(editMenu), copyItem);

    GtkWidget *pasteItem = gtk_menu_item_new_with_mnemonic("_Paste");
    g_signal_connect(pasteItem, "activate", G_CALLBACK(on_menu_edit_paste), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(editMenu), pasteItem);

    GtkWidget *deleteItem = gtk_menu_item_new_with_mnemonic("_Delete");
    g_signal_connect(deleteItem, "activate", G_CALLBACK(on_menu_edit_delete), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(editMenu), deleteItem);

    gtk_menu_shell_append(GTK_MENU_SHELL(editMenu), gtk_separator_menu_item_new());

    GtkWidget *selectAllItem = gtk_menu_item_new_with_mnemonic("Select _All");
    g_signal_connect(selectAllItem, "activate", G_CALLBACK(on_menu_edit_select_all), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(editMenu), selectAllItem);

    gtk_menu_shell_append(GTK_MENU_SHELL(editMenu), gtk_separator_menu_item_new());

    GtkWidget *findItem = gtk_menu_item_new_with_mnemonic("_Find");
    g_signal_connect(findItem, "activate", G_CALLBACK(on_menu_edit_find), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(editMenu), findItem);

    GtkWidget *replaceItem = gtk_menu_item_new_with_mnemonic("_Replace");
    g_signal_connect(replaceItem, "activate", G_CALLBACK(on_menu_edit_replace), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(editMenu), replaceItem);

    gtk_menu_shell_append(GTK_MENU_SHELL(editMenu), gtk_separator_menu_item_new());

    GtkWidget *timeDateItem = gtk_menu_item_new_with_mnemonic("Time/Date");
    g_signal_connect(timeDateItem, "activate", G_CALLBACK(on_menu_edit_time_date), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(editMenu), timeDateItem);

    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), editItem);

    // Format menu
    GtkWidget *formatMenu = gtk_menu_new();
    GtkWidget *formatItem = gtk_menu_item_new_with_mnemonic("F_ormat");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(formatItem), formatMenu);

    GtkWidget *wordWrapItem = gtk_menu_item_new_with_mnemonic("_Word Wrap");
    g_signal_connect(wordWrapItem, "activate", G_CALLBACK(on_menu_format_word_wrap), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(formatMenu), wordWrapItem);

    GtkWidget *fontItem = gtk_menu_item_new_with_mnemonic("_Font");
    g_signal_connect(fontItem, "activate", G_CALLBACK(on_menu_format_font), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(formatMenu), fontItem);

    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), formatItem);

    // View menu
    GtkWidget *viewMenu = gtk_menu_new();
    GtkWidget *viewItem = gtk_menu_item_new_with_mnemonic("_View");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(viewItem), viewMenu);

    GtkWidget *statusBarItem = gtk_menu_item_new_with_mnemonic("_Status Bar");
    g_signal_connect(statusBarItem, "activate", G_CALLBACK(on_menu_view_status_bar), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(viewMenu), statusBarItem);

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

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    // Create main window
    g_app.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(g_app.window), APP_TITLE);
    gtk_window_set_default_size(GTK_WINDOW(g_app.window), DEFAULT_WIDTH, DEFAULT_HEIGHT);
    g_signal_connect(g_app.window, "delete-event", G_CALLBACK(on_window_delete), NULL);

    // Create VBox for layout
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(g_app.window), vbox);

    // Create menu bar
    GtkWidget *menubar = CreateMenuBar();
    gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);

    // Create text view with buffer
    g_app.textBuffer = gtk_text_buffer_new(NULL);
    g_signal_connect(g_app.textBuffer, "changed", G_CALLBACK(on_text_changed), NULL);
    g_signal_connect(g_app.textBuffer, "notify::cursor-position",
        G_CALLBACK(on_cursor_moved), NULL);

    g_app.textView = gtk_text_view_new_with_buffer(g_app.textBuffer);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(g_app.textView), GTK_WRAP_WORD);

    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
        GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scrolled), g_app.textView);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled, TRUE, TRUE, 0);

    // Create find bar
    g_app.findBar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(g_app.findBar), 5);
    g_app.findEntry = gtk_entry_new();
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
    GtkWidget *replaceAllBtn = gtk_button_new_with_label("Replace All");
    g_signal_connect(replaceAllBtn, "clicked", G_CALLBACK(on_replace_all), NULL);

    gtk_box_pack_start(GTK_BOX(g_app.replaceBar), gtk_label_new("Replace:"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(g_app.replaceBar), g_app.replaceEntry, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(g_app.replaceBar), replaceAllBtn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), g_app.replaceBar, FALSE, FALSE, 0);
    gtk_widget_hide(g_app.replaceBar);

    // Create status bar
    g_app.statusbar = gtk_statusbar_new();
    g_statusbar_context = gtk_statusbar_get_context_id(GTK_STATUSBAR(g_app.statusbar), "main");
    gtk_box_pack_start(GTK_BOX(vbox), g_app.statusbar, FALSE, FALSE, 0);

    /* Initialize undo/redo stacks */
    g_app.undoStack = g_queue_new();
    g_app.redoStack = g_queue_new();
    g_app.isUndoRedoInProgress = FALSE;

    g_app.wordWrap = TRUE;
    g_app.statusVisible = TRUE;
    g_app.encoding = ENC_UTF8;
    g_app.modified = FALSE;

    UpdateTitle();
    UpdateStatusBar();

    gtk_widget_show_all(g_app.window);
    gtk_widget_hide(g_app.findBar);
    gtk_widget_hide(g_app.replaceBar);

    gtk_main();

    /* Cleanup undo/redo stacks */
    g_queue_foreach(g_app.undoStack, (GFunc)FreeUndoEntry, NULL);
    g_queue_free(g_app.undoStack);
    g_queue_foreach(g_app.redoStack, (GFunc)FreeUndoEntry, NULL);
    g_queue_free(g_app.redoStack);

    if (g_app.fontDesc) {
        pango_font_description_free(g_app.fontDesc);
    }

    return 0;
}