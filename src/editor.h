// Editor widget setup for retropad
#pragma once

#include <gtk/gtk.h>

/* Create the editor widget (text view with line numbers) */
GtkWidget* CreateEditorWidget(void);

/* Toggle line numbers visibility */
void ToggleLineNumbers(gboolean visible);

/* Setup drag and drop for the text view */
void SetupDragAndDrop(void);

/* Text buffer change callback */
void on_text_changed(GtkTextBuffer *buffer, gpointer user_data);

/* Cursor position change callback */
void on_cursor_moved(GtkTextBuffer *buffer, GParamSpec *pspec, gpointer user_data);
