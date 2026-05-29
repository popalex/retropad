// File operations implementation for retropad
#include "file_ops.h"
#include "app_state.h"
#include "file_io.h"
#include "undo.h"
#include "utils.h"
#include "recent_files.h"
#include <string.h>

gboolean PromptSaveChanges(void) {
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

/* Helper: clear undo/redo state for a fresh document */
static void ClearDocumentState(void) {
    ClearRedoStack();
    while (!g_queue_is_empty(g_app.undoStack)) {
        gpointer data = g_queue_pop_head(g_app.undoStack);
        g_free(data);
    }
    g_app.lastUndoLength = 0;
    g_app.lastUndoTime = 0;
    g_app.lastChar = '\0';
}

void DoFileNew(void) {
    if (!PromptSaveChanges()) return;
    g_app.isProgrammaticChange = TRUE;
    gtk_text_buffer_set_text(g_app.textBuffer, "", -1);
    g_app.isProgrammaticChange = FALSE;
    g_app.currentPath[0] = '\0';
    g_app.encoding = ENC_UTF8;
    g_app.modified = FALSE;
    ClearDocumentState();
    UpdateTitle();
    UpdateStatusBar();
}

void DoFileOpen(void) {
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

gboolean DoFileSave(gboolean saveAs) {
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
                path[MAX_PATH_BUFFER - 1] = '\0';
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
        g_app.currentPath[MAX_PATH_BUFFER - 1] = '\0';
    } else {
        strncpy(path, g_app.currentPath, MAX_PATH_BUFFER - 1);
        path[MAX_PATH_BUFFER - 1] = '\0';
    }

    char *text = NULL;
    int len = 0;
    if (!GetEditText(&text, &len)) return FALSE;

    gboolean ok = SaveTextFile(NULL, path, text, len, g_app.encoding);
    g_free(text);

    if (ok) {
        g_app.modified = FALSE;
        UpdateTitle();
    } else {
        GtkWidget *errDialog = gtk_message_dialog_new(
            GTK_WINDOW(g_app.window),
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_OK,
            "Could not save file:\n%s", path);
        gtk_dialog_run(GTK_DIALOG(errDialog));
        gtk_widget_destroy(errDialog);
    }
    return ok;
}

gboolean LoadDocumentFromPath(const char *path) {
    char *text = NULL;
    TextEncoding enc = ENC_UTF8;
    if (!LoadTextFile(NULL, path, &text, NULL, &enc)) {
        GtkWidget *errDialog = gtk_message_dialog_new(
            GTK_WINDOW(g_app.window),
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_OK,
            "Could not open file:\n%s", path);
        gtk_dialog_run(GTK_DIALOG(errDialog));
        gtk_widget_destroy(errDialog);
        return FALSE;
    }

    g_app.isProgrammaticChange = TRUE;
    gtk_text_buffer_set_text(g_app.textBuffer, text, -1);
    g_app.isProgrammaticChange = FALSE;

    g_free(text);
    strncpy(g_app.currentPath, path, MAX_PATH_BUFFER - 1);
    g_app.currentPath[MAX_PATH_BUFFER - 1] = '\0';
    g_app.encoding = enc;
    g_app.modified = FALSE;
    ClearDocumentState();
    AddRecentFile(path);
    UpdateTitle();
    UpdateStatusBar();
    return TRUE;
}
