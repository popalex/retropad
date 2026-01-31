# Copilot Instructions for retropad

## Project Overview
retropad is a Petzold-style Notepad clone written in **plain C11** for **Linux** using **GTK3**. It's a single-window text editor with classic menus, keyboard shortcuts, find/replace, font picker, and BOM-aware file encoding.

## Architecture

### File Structure
- `retropad.c` — Main application (~980 lines): window, menus, all UI callbacks, undo/redo logic
- `file_io.c/.h` — Text file I/O with BOM detection (UTF-8/UTF-16LE/UTF-16BE/ANSI)
- `CMakeLists.txt` — CMake build with GTK3 via pkg-config

### Global State Pattern
All application state lives in a single global `AppState` struct (`g_app`):
```c
static AppState g_app = {0};
```
Access UI elements via `g_app.window`, `g_app.textBuffer`, `g_app.textView`, etc. New features should add fields to this struct rather than creating additional globals.

### Naming Conventions
- **Functions**: PascalCase with prefix indicating purpose: `DoFileNew()`, `DoFileSave()`, `UpdateTitle()`, `ToggleStatusBar()`
- **Callbacks**: Named for their action, e.g., `on_new_activate`, `on_save_activate`
- **Constants**: ALL_CAPS defines: `APP_TITLE`, `MAX_PATH_BUFFER`, `DEFAULT_WIDTH`

## Build Commands
```bash
# Clean build (from project root)
rm -rf build && mkdir build && cd build && cmake .. && make

# Incremental build
cd build && make

# If CMake outputs to wrong directory, remove stale cache:
rm -rf CMakeCache.txt CMakeFiles cmake_install.cmake Makefile
```

## Key Patterns

### GTK3 Text Buffer Operations
Use `GtkTextIter` for all cursor/selection work:
```c
GtkTextIter start, end;
gtk_text_buffer_get_bounds(g_app.textBuffer, &start, &end);
char *text = gtk_text_buffer_get_text(g_app.textBuffer, &start, &end, FALSE);
```

### Undo/Redo Implementation
Custom undo stack using `GQueue` with smart grouping (500ms timeout, break on punctuation). When modifying buffer programmatically, set `g_app.isUndoRedoInProgress = TRUE` to prevent spurious undo entries.

### File Encoding
- `TextEncoding` enum: `ENC_UTF8`, `ENC_UTF16LE`, `ENC_UTF16BE`, `ENC_ANSI`
- Detection via BOM in `DetectEncoding()` (file_io.c)
- Default save format: UTF-8 with BOM

### Modified State
Always update `g_app.modified` and call `UpdateTitle()` after text changes. The title shows `*filename - retropad` when modified.

## Adding New Features
1. Add any new state to `AppState` struct
2. Create menu item in `CreateMenuBar()` with accelerator
3. Implement `Do<Feature>()` function following existing patterns
4. Connect via `g_signal_connect()`

## Dependencies
- GTK3 (`gtk+-3.0` via pkg-config)
- GLib (included with GTK3)
- Standard C library

## Testing
No automated tests. Manual testing: run `./build/retropad`, verify new features with keyboard shortcuts and menu actions.
