// Recent files management for retropad
#pragma once

#include <gtk/gtk.h>

/* Load recent files from disk */
void LoadRecentFiles(void);

/* Save recent files to disk */
void SaveRecentFiles(void);

/* Add a file to recent files list */
void AddRecentFile(const char *path);

/* Update the recent files menu */
void UpdateRecentFilesMenu(void);
