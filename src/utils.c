// Utility functions implementation for retropad
#include "utils.h"
#include "app_state.h"
#include "prefs.h"
#include <string.h>
#include <time.h>

void UpdateTitle(void) {
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

void UpdateStatusBar(void) {
    if (!g_app.statusVisible) return;

    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(g_app.textBuffer, &start, &end);
    gint totalLines = gtk_text_iter_get_line(&end) + 1;

    GtkTextIter cursor;
    gtk_text_buffer_get_iter_at_mark(g_app.textBuffer,
        &cursor, gtk_text_buffer_get_insert(g_app.textBuffer));
    gint line = gtk_text_iter_get_line(&cursor) + 1;
    gint col = gtk_text_iter_get_line_offset(&cursor) + 1;

    /* Get encoding name */
    const char *encName = "UTF-8";
    switch (g_app.encoding) {
        case ENC_UTF16LE: encName = "UTF-16 LE"; break;
        case ENC_UTF16BE: encName = "UTF-16 BE"; break;
        case ENC_ANSI: encName = "ANSI"; break;
        case ENC_UTF8: default: encName = "UTF-8"; break;
    }

    char status[128];
    snprintf(status, sizeof(status), "Ln %d, Col %d    Lines: %d    %s",
             line, col, totalLines, encName);

    gtk_statusbar_pop(GTK_STATUSBAR(g_app.statusbar), g_statusbar_context);
    gtk_statusbar_push(GTK_STATUSBAR(g_app.statusbar), g_statusbar_context, status);
}

gboolean GetEditText(char **bufferOut, size_t *lengthOut) {
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(g_app.textBuffer, &start, &end);
    char *text = gtk_text_buffer_get_text(g_app.textBuffer, &start, &end, FALSE);
    if (!text) return FALSE;

    size_t len = strlen(text);
    if (lengthOut) *lengthOut = len;
    *bufferOut = text;
    return TRUE;
}

void InsertTimeDate(void) {
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    if (!tm_info) return;
    
    char stamp[128];
    if (strftime(stamp, sizeof(stamp), "%X %x", tm_info) == 0) return;

    GtkTextIter cursor;
    gtk_text_buffer_get_iter_at_mark(g_app.textBuffer,
        &cursor, gtk_text_buffer_get_insert(g_app.textBuffer));
    gtk_text_buffer_insert(g_app.textBuffer, &cursor, stamp, -1);
}

void SetWordWrap(gboolean enabled) {
    if (g_app.wordWrap == enabled) return;
    g_app.wordWrap = enabled;

    if (enabled) {
        gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(g_app.textView), GTK_WRAP_WORD);
    } else {
        gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(g_app.textView), GTK_WRAP_NONE);
    }
    SavePrefs();
}

void ToggleStatusBar(gboolean visible) {
    g_app.statusVisible = visible;
    if (visible) {
        gtk_widget_show(g_app.statusbar);
    } else {
        gtk_widget_hide(g_app.statusbar);
    }
    SavePrefs();
}
