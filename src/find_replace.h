// Find and replace functionality for retropad
#pragma once

#include <gtk/gtk.h>

/* Show find bar */
void ShowFindBar(void);

/* Show replace bar */
void ShowReplaceBar(void);

/* Find next occurrence */
gboolean DoFindNext(gboolean reverse);

/* Find text in buffer */
gboolean FindInEdit(const char *needle, gboolean matchCase, gboolean searchDown,
                    GtkTextIter *outStart, GtkTextIter *outEnd);

/* Replace all occurrences */
int ReplaceAllOccurrences(const char *needle, const char *replacement,
                          gboolean matchCase);

/* Key press handlers for find/replace entries */
gboolean on_find_entry_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data);
gboolean on_replace_entry_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data);

/* Button click handlers */
void on_find_next(GtkWidget *widget, gpointer user_data);
void on_find_previous(GtkWidget *widget, gpointer user_data);
void on_replace(GtkWidget *widget, gpointer user_data);
void on_replace_all(GtkWidget *widget, gpointer user_data);
