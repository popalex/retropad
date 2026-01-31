// Menu bar creation for retropad
#pragma once

#include <gtk/gtk.h>

/* Create the menu bar with all menus */
GtkWidget* CreateMenuBar(GtkAccelGroup *accelGroup);

/* Create find/replace bars */
void CreateFindReplaceBar(GtkWidget *vbox);
