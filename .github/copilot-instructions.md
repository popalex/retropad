# Copilot Instructions for retropad

## Project Overview
retropad is a Petzold-style Notepad clone written in **plain C11** for **Linux** using **GTK3**. It's a single-window text editor with classic menus, keyboard shortcuts, find/replace, font picker, and BOM-aware file encoding.

## Architecture

### File Structure (src/ directory)

| File | Purpose | ~Lines |
|------|---------|--------|
| `main.c` | Entry point, window creation, main loop | ~70 |
| `app_state.h/c` | Global `AppState` struct definition and initialization | ~60 |
| `menu.h/c` | Menu bar creation, all `on_menu_*` callbacks | ~300 |
| `editor.h/c` | Text view widget, line numbers, drag-drop, buffer callbacks | ~150 |
| `undo.h/c` | Smart undo/redo stack with grouping | ~140 |
| `file_ops.h/c` | High-level file operations (New/Open/Save) | ~130 |
| `file_io.h/c` | Low-level BOM-aware file read/write | ~190 |
| `find_replace.h/c` | Find bar, replace bar, search logic | ~200 |
| `dialogs.h/c` | Go To Line, Font selection, About dialogs | ~140 |
| `print.h/c` | GTK print support | ~80 |
| `recent_files.h/c` | MRU file list persistence and menu | ~100 |
| `utils.h/c` | Shared helpers (UpdateTitle, UpdateStatusBar, etc.) | ~90 |
| `prefs.h/c` | Preferences load/save (`~/.config/retropad/config.ini`) via GKeyFile | ~130 |

### Global State Pattern
All application state lives in a single global `AppState` struct (`g_app`) defined in `app_state.c`:
```c
AppState g_app = {0};  // Definition in app_state.c
extern AppState g_app; // Declaration in app_state.h
```
Access UI elements via `g_app.window`, `g_app.textBuffer`, `g_app.textView`, etc. New features should add fields to this struct rather than creating additional globals.

### Naming Conventions
- **Functions**: PascalCase with prefix indicating purpose: `DoFileNew()`, `DoFileSave()`, `UpdateTitle()`, `ToggleStatusBar()`
- **Callbacks**: Named for their action, e.g., `on_menu_file_new`, `on_text_changed`
- **Constants**: ALL_CAPS defines in `app_state.h`: `APP_TITLE`, `MAX_PATH_BUFFER`, `DEFAULT_WIDTH`
- **Static functions**: Module-local helpers use `static` keyword

### Module Dependencies
```
main.c
  ├── app_state.h (includes file_io.h)
  ├── menu.h
  ├── editor.h
  ├── recent_files.h
  ├── utils.h
  └── file_ops.h

menu.c → file_ops, undo, find_replace, dialogs, print, utils, editor
editor.c → app_state, undo, utils, file_ops
file_ops.c → app_state, file_io, undo, utils, recent_files
```

## Build Commands
```bash
# Clean build (from project root)
rm -rf build && mkdir build && cd build && cmake .. && make

# Incremental build
cd build && make

# Run
./build/retropad
```

## Key Patterns

### GTK3 Text Buffer Operations
Use `GtkTextIter` for all cursor/selection work:
```c
GtkTextIter start, end;
gtk_text_buffer_get_bounds(g_app.textBuffer, &start, &end);
char *text = gtk_text_buffer_get_text(g_app.textBuffer, &start, &end, FALSE);
```

### Undo/Redo Implementation (undo.c)
Custom undo stack using `GQueue` with smart grouping (500ms timeout, break on punctuation). When modifying buffer programmatically, set `g_app.isProgrammaticChange = TRUE` to suppress undo push and modified-state update, then set it back to `FALSE` after. `isUndoRedoInProgress` is used specifically during undo/redo operations.

### File Encoding (file_io.c)
- `TextEncoding` enum: `ENC_UTF8`, `ENC_UTF16LE`, `ENC_UTF16BE`, `ENC_ANSI`
- Detection via BOM in `DetectEncoding()`; BOM-less files are validated with `g_utf8_validate()` and fall back to `ENC_ANSI` (ISO-8859-1) if not valid UTF-8
- Default save format: **UTF-8 without BOM** (Linux-friendly); atomic save via temp file + rename

### Modified State
Always update `g_app.modified` and call `UpdateTitle()` after text changes. The title shows `*filename - retropad` when modified. Programmatic buffer changes (open/new) use `g_app.isProgrammaticChange = TRUE` to avoid spurious modified state.

### Preferences (prefs.c)
`LoadPrefs()` / `SavePrefs()` use `GKeyFile` to read/write `~/.config/retropad/config.ini`. Load is called after `gtk_widget_show_all()` at startup. Save is called on window close, font change, and each toggle of word wrap / status bar / line numbers.

## Adding New Features

### Adding a new menu action
1. Add any new state to `AppState` struct in `app_state.h`
2. Create `Do<Feature>()` function in appropriate module (or new module)
3. Add callback wrapper in `menu.c`
4. Add menu item in `CreateMenuBar()` with accelerator

### Adding a new module
1. Create `feature.h` with function prototypes
2. Create `feature.c` with implementation
3. Add both files to `CMakeLists.txt` under SOURCES/HEADERS
4. Include header where needed

## Dependencies
- GTK3 (`gtk+-3.0` via pkg-config)
- GLib (included with GTK3)
- Standard C library

## Testing
No automated tests. Manual testing: run `./build/retropad`, verify new features with keyboard shortcuts and menu actions.
