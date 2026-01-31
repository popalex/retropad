// Undo/redo implementation for retropad
#include "undo.h"
#include "app_state.h"
#include "utils.h"
#include <string.h>

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

static void FreeUndoEntry(gpointer data, gpointer user_data) {
    (void)user_data;
    UndoRedoEntry *entry = (UndoRedoEntry *)data;
    if (entry) {
        g_free(entry->text);
        g_free(entry);
    }
}

static gboolean IsSignificantChar(gchar c) {
    /* Check if character is a break point in undo/redo */
    return c == '\n' || c == ' ' || c == '\t' || 
           c == ',' || c == '.' || c == ';' || c == ':' || 
           c == '!' || c == '?' || c == '-' || c == '(' || c == ')' ||
           c == '\0';
}

void PushUndoStack(void) {
    if (g_app.isUndoRedoInProgress) return;
    
    /* Get current text length */
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(g_app.textBuffer, &start, &end);
    gint currentLength = gtk_text_iter_get_offset(&end);
    
    /* Get current time */
    gint64 currentTime = g_get_monotonic_time();
    
    /* Get last character in buffer for efficient break detection */
    gchar lastChar = '\0';
    if (currentLength > 0) {
        GtkTextIter lastIter;
        gtk_text_buffer_get_iter_at_offset(g_app.textBuffer, &lastIter, currentLength - 1);
        lastChar = gtk_text_iter_get_char(&lastIter);
    }
    
    /* Smart grouping: Only push a new undo state if:
     * 1. More than 500ms has passed since last undo
     * 2. Text was deleted (length decreased)
     * 3. A significant character was just typed
     */
    gboolean shouldPush = TRUE;
    
    if (!g_queue_is_empty(g_app.undoStack)) {
        gint timeDiff = (currentTime - g_app.lastUndoTime) / 1000; /* Convert to ms */
        gint lengthDiff = currentLength - g_app.lastUndoLength;
        gboolean isSignificant = IsSignificantChar(lastChar);
        
        /* If less than 500ms, text grew (adding chars), and last char not significant, don't push */
        if (timeDiff < 500 && lengthDiff > 0 && !isSignificant) {
            shouldPush = FALSE;
        }
    }
    
    if (shouldPush) {
        /* Limit undo stack size */
        while (g_queue_get_length(g_app.undoStack) >= MAX_UNDO_STACK) {
            FreeUndoEntry(g_queue_pop_head(g_app.undoStack), NULL);
        }
        
        g_queue_push_tail(g_app.undoStack, CreateUndoEntry());
    }
    
    /* Update tracking variables */
    g_app.lastUndoLength = currentLength;
    g_app.lastUndoTime = currentTime;
    g_app.lastChar = lastChar;
}

void ClearRedoStack(void) {
    g_queue_foreach(g_app.redoStack, FreeUndoEntry, NULL);
    g_queue_clear(g_app.redoStack);
}

void DoUndo(void) {
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
        
        FreeUndoEntry(entry, NULL);
    }
    
    g_app.isUndoRedoInProgress = FALSE;
    UpdateStatusBar();
}

void DoRedo(void) {
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
        
        FreeUndoEntry(entry, NULL);
    }
    
    g_app.isUndoRedoInProgress = FALSE;
    UpdateStatusBar();
}

void UndoStackCleanup(void) {
    g_queue_foreach(g_app.undoStack, FreeUndoEntry, NULL);
    g_queue_free(g_app.undoStack);
    g_queue_foreach(g_app.redoStack, FreeUndoEntry, NULL);
    g_queue_free(g_app.redoStack);
}
