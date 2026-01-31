// Find and replace implementation for retropad
#include "find_replace.h"
#include "app_state.h"
#include "utils.h"
#include <string.h>

gboolean FindInEdit(const char *needle, gboolean matchCase, gboolean searchDown,
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

int ReplaceAllOccurrences(const char *needle, const char *replacement,
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
    char *searchPtr = searchBuf;
    const char *origPtr = text;
    
    while ((p = strstr(searchPtr, needleBuf)) != NULL) {
        int delta = p - searchPtr;
        g_string_append_len(result, origPtr, delta);
        if (replacement) {
            g_string_append(result, replacement);
        }
        origPtr += delta + strlen(needle);
        searchPtr = p + strlen(needleBuf);
    }
    g_string_append(result, origPtr);

    gtk_text_buffer_set_text(g_app.textBuffer, result->str, -1);
    g_string_free(result, TRUE);
    g_free(text);
    g_free(searchBuf);
    g_free(needleBuf);
    
    g_app.modified = TRUE;
    UpdateTitle();
    return count;
}

void ShowFindBar(void) {
    gtk_widget_show_all(g_app.findBar);
    gtk_widget_grab_focus(g_app.findEntry);
}

void ShowReplaceBar(void) {
    gtk_widget_show_all(g_app.replaceBar);
    gtk_widget_grab_focus(g_app.replaceEntry);
}

gboolean DoFindNext(gboolean reverse) {
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

gboolean on_find_entry_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data) {
    (void)widget;
    (void)user_data;
    if (event->keyval == GDK_KEY_Escape) {
        gtk_widget_hide(g_app.findBar);
        gtk_widget_hide(g_app.replaceBar);
        gtk_widget_grab_focus(g_app.textView);
        return TRUE;
    }
    if (event->keyval == GDK_KEY_Return || event->keyval == GDK_KEY_KP_Enter) {
        DoFindNext(FALSE);
        return TRUE;
    }
    return FALSE;
}

gboolean on_replace_entry_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data) {
    (void)widget;
    (void)user_data;
    if (event->keyval == GDK_KEY_Escape) {
        gtk_widget_hide(g_app.findBar);
        gtk_widget_hide(g_app.replaceBar);
        gtk_widget_grab_focus(g_app.textView);
        return TRUE;
    }
    return FALSE;
}

void on_find_next(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    (void)user_data;
    DoFindNext(FALSE);
}

void on_find_previous(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    (void)user_data;
    DoFindNext(TRUE);
}

void on_replace_all(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    (void)user_data;
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
