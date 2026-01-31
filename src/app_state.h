// Application state and global definitions for retropad
#pragma once

#include <gtk/gtk.h>
#include "file_io.h"

#define APP_TITLE "retropad"
#define UNTITLED_NAME "Untitled"
#define MAX_PATH_BUFFER 1024
#define DEFAULT_WIDTH 640
#define DEFAULT_HEIGHT 480
#define MAX_UNDO_STACK 100
#define MAX_RECENT_FILES 5
#define LINE_NUMBER_MARGIN_WIDTH 50
#define RECENT_FILES_PATH ".retropad_recent"

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
    /* Smart undo/redo */
    gint lastUndoLength;
    gint64 lastUndoTime;
    gchar lastChar;
    /* Line numbers */
    GtkWidget *lineNumberView;
    gboolean lineNumbersVisible;
    /* Recent files */
    GList *recentFiles;
    GtkWidget *recentFilesMenu;
    /* Toggle menu items (for checkmarks) */
    GtkWidget *wordWrapMenuItem;
    GtkWidget *statusBarMenuItem;
    GtkWidget *lineNumbersMenuItem;
} AppState;

/* Global application state */
extern AppState g_app;
extern guint g_statusbar_context;

/* Initialization and cleanup */
void AppStateInit(void);
void AppStateCleanup(void);
