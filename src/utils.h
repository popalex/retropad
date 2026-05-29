// Utility functions for retropad
#pragma once

#include <gtk/gtk.h>

/* Update window title based on current state */
void UpdateTitle(void);

/* Update status bar with cursor position */
void UpdateStatusBar(void);

/* Get text content from editor buffer */
gboolean GetEditText(char **bufferOut, size_t *lengthOut);

/* Insert current time/date at cursor */
void InsertTimeDate(void);

/* Set word wrap mode */
void SetWordWrap(gboolean enabled);

/* Toggle status bar visibility */
void ToggleStatusBar(gboolean visible);
