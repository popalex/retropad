// File operations for retropad
#pragma once

#include <gtk/gtk.h>

/* Create a new document */
void DoFileNew(void);

/* Open a file */
void DoFileOpen(void);

/* Save file (optionally as new file) */
gboolean DoFileSave(gboolean saveAs);

/* Load document from path */
gboolean LoadDocumentFromPath(const char *path);

/* Prompt user to save changes if modified */
gboolean PromptSaveChanges(void);
