// Undo/redo functionality for retropad
#pragma once

#include <gtk/gtk.h>

/* Push current state to undo stack with smart grouping */
void PushUndoStack(void);

/* Clear redo stack */
void ClearRedoStack(void);

/* Clear undo stack */
void ClearUndoStack(void);

/* Perform undo operation */
void DoUndo(void);

/* Perform redo operation */
void DoRedo(void);

/* Cleanup undo/redo stacks */
void UndoStackCleanup(void);

/* Reset pending undo state (call after document reset) */
void ResetPendingUndoState(void);
