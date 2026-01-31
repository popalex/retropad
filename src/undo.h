// Undo/redo functionality for retropad
#pragma once

#include <gtk/gtk.h>

/* Push current state to undo stack with smart grouping */
void PushUndoStack(void);

/* Clear redo stack */
void ClearRedoStack(void);

/* Perform undo operation */
void DoUndo(void);

/* Perform redo operation */
void DoRedo(void);

/* Cleanup undo/redo stacks */
void UndoStackCleanup(void);
