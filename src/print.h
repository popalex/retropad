// Print support for retropad
#pragma once

#include <gtk/gtk.h>

/* Print document */
void DoPrint(void);

/* Page setup dialog */
void DoPageSetup(void);

/* Cleanup print resources */
void PrintCleanup(void);
