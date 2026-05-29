// Find and replace implementation for retropad
#include "find_replace.h"
#include "app_state.h"
#include "utils.h"
#include <string.h>

gboolean FindInEdit(const char *needle, gboolean matchCase, gboolean searchDown,
                    GtkTextIter *outStart, GtkTextIter *outEnd) {
    if (!needle || needle[0] == '\0') return FALSE;

    char *text = NULL;
    size_t len = 0;
    if (!GetEditText(&text, &len)) return FALSE;

    char *haystack = NULL;
    char *needleBuf = NULL;

    if (!matchCase) {
        haystack = g_utf8_casefold(text, -1);
        needleBuf = g_utf8_casefold(needle, -1);
    } else {
        haystack = g_strdup(text);
        needleBuf = g_strdup(needle);
    }

    /* Get search start position as byte offset.
     * GTK iters use character offsets, so convert to byte offset in the UTF-8 string. */
    GtkTextIter selStart, selEnd;
    gint searchBytePos;
    if (gtk_text_buffer_get_selection_bounds(g_app.textBuffer, &selStart, &selEnd)) {
        if (searchDown) {
            GtkTextIter bufStart;
            gtk_text_buffer_get_start_iter(g_app.textBuffer, &bufStart);
            gchar *tmp = gtk_text_buffer_get_text(g_app.textBuffer, &bufStart, &selEnd, FALSE);
            searchBytePos = (gint)strlen(tmp);
            g_free(tmp);
        } else {
            GtkTextIter bufStart;
            gtk_text_buffer_get_start_iter(g_app.textBuffer, &bufStart);
            gchar *tmp = gtk_text_buffer_get_text(g_app.textBuffer, &bufStart, &selStart, FALSE);
            searchBytePos = (gint)strlen(tmp);
            g_free(tmp);
        }
    } else {
        GtkTextIter cursor, bufStart;
        gtk_text_buffer_get_iter_at_mark(g_app.textBuffer,
            &cursor, gtk_text_buffer_get_insert(g_app.textBuffer));
        gtk_text_buffer_get_start_iter(g_app.textBuffer, &bufStart);
        gchar *tmp = gtk_text_buffer_get_text(g_app.textBuffer, &bufStart, &cursor, FALSE);
        searchBytePos = (gint)strlen(tmp);
        g_free(tmp);
    }

    /* Ensure searchBytePos is within bounds */
    if (searchBytePos < 0) searchBytePos = 0;
    if ((size_t)searchBytePos > len) searchBytePos = (gint)len;

    char *found = NULL;
    gboolean result = FALSE;

    if (searchDown) {
        /* Search forward from position */
        found = strstr(haystack + searchBytePos, needleBuf);
        if (!found) {
            /* Wrap around to beginning */
            found = strstr(haystack, needleBuf);
        }
    } else {
        /* Search backward - find last occurrence before searchBytePos */
        char *lastFound = NULL;
        char *p = haystack;
        while ((p = strstr(p, needleBuf)) != NULL) {
            if ((p - haystack) < searchBytePos) {
                lastFound = p;
                p += 1;
            } else {
                break;
            }
        }
        if (lastFound) {
            found = lastFound;
        } else {
            /* Wrap around - find last occurrence in entire text */
            p = haystack;
            while ((p = strstr(p, needleBuf)) != NULL) {
                lastFound = p;
                p += 1;
            }
            found = lastFound;
        }
    }

    if (found) {
        /* Convert byte offset to character offset for GTK */
        gint bytePos = (gint)(found - haystack);
        gint charPos = (gint)g_utf8_strlen(text, bytePos);
        gint needleCharLen = (gint)g_utf8_strlen(needle, -1);

        gtk_text_buffer_get_iter_at_offset(g_app.textBuffer, outStart, charPos);
        gtk_text_buffer_get_iter_at_offset(g_app.textBuffer, outEnd,
                                          charPos + needleCharLen);
        result = TRUE;
    }

    g_free(text);
    g_free(haystack);
    g_free(needleBuf);
    return result;
}

int ReplaceAllOccurrences(const char *needle, const char *replacement,
                          gboolean matchCase) {
    if (!needle || needle[0] == '\0') return 0;

    char *text = NULL;
    size_t len = 0;
    if (!GetEditText(&text, &len)) return 0;

    char *searchBuf = NULL;
    char *needleBuf = NULL;

    if (!matchCase) {
        searchBuf = g_utf8_casefold(text, -1);
        needleBuf = g_utf8_casefold(needle, -1);
    } else {
        searchBuf = g_strdup(text);
        needleBuf = g_strdup(needle);
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
    
    /* If find entry is empty, try to use selected text */
    if (!needle || needle[0] == '\0') {
        GtkTextIter selStart, selEnd;
        if (gtk_text_buffer_get_selection_bounds(g_app.textBuffer, &selStart, &selEnd)) {
            char *selected = gtk_text_buffer_get_text(g_app.textBuffer, &selStart, &selEnd, FALSE);
            if (selected && selected[0] != '\0') {
                gtk_entry_set_text(GTK_ENTRY(g_app.findEntry), selected);
                needle = gtk_entry_get_text(GTK_ENTRY(g_app.findEntry));
            }
            g_free(selected);
        }
    }
    
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

void on_replace(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    (void)user_data;
    const char *needle = gtk_entry_get_text(GTK_ENTRY(g_app.findEntry));
    const char *replacement = gtk_entry_get_text(GTK_ENTRY(g_app.replaceEntry));
    
    if (!needle || needle[0] == '\0') return;
    
    /* Check if current selection matches the search term */
    GtkTextIter selStart, selEnd;
    if (gtk_text_buffer_get_selection_bounds(g_app.textBuffer, &selStart, &selEnd)) {
        char *selected = gtk_text_buffer_get_text(g_app.textBuffer, &selStart, &selEnd, FALSE);
        gboolean matches = FALSE;
        
        if (g_app.matchCase) {
            matches = (strcmp(selected, needle) == 0);
        } else {
            matches = (g_ascii_strcasecmp(selected, needle) == 0);
        }
        
        if (matches) {
            /* Replace the selection */
            gtk_text_buffer_delete(g_app.textBuffer, &selStart, &selEnd);
            gtk_text_buffer_insert(g_app.textBuffer, &selStart, replacement, -1);
        }
        g_free(selected);
    }
    
    /* Find next occurrence */
    DoFindNext(FALSE);
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
