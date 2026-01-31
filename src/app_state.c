// Application state implementation for retropad
#include "app_state.h"
#include "undo.h"
#include "print.h"

/* Global application state */
AppState g_app = {0};
guint g_statusbar_context = 0;

void AppStateInit(void) {
    /* Initialize undo/redo stacks */
    g_app.undoStack = g_queue_new();
    g_app.redoStack = g_queue_new();
    g_app.isUndoRedoInProgress = FALSE;
    g_app.lastUndoLength = 0;
    g_app.lastUndoTime = 0;
    g_app.lastChar = '\0';

    g_app.wordWrap = TRUE;
    g_app.statusVisible = TRUE;
    g_app.encoding = ENC_UTF8;
    g_app.modified = FALSE;
    g_app.lineNumbersVisible = FALSE;
}

void AppStateCleanup(void) {
    /* Cleanup undo/redo stacks */
    UndoStackCleanup();
    
    /* Cleanup recent files */
    g_list_free_full(g_app.recentFiles, g_free);

    /* Cleanup print settings */
    PrintCleanup();

    if (g_app.fontDesc) {
        pango_font_description_free(g_app.fontDesc);
    }
}
