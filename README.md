# retropad

A Petzold-style Notepad clone written in plain C11 for Linux using GTK3. Keeps the classic menus, accelerators, word wrap toggle, status bar, find/replace, font picker, time/date insertion, and BOM-aware load/save. Designed to be clean, simple, and focused — not an IDE.

## Prerequisites (Linux)

- Git
- GCC or Clang compiler
- CMake 3.12 or later
- GTK3 development libraries: `libgtk-3-dev`
- GLib development libraries: `libglib2.0-dev`

### Ubuntu/Debian
```bash
sudo apt-get install build-essential cmake libgtk-3-dev libglib2.0-dev
```

### Fedora/RHEL
```bash
sudo dnf install gcc cmake gtk3-devel glib2-devel
```

### Arch
```bash
sudo pacman -S base-devel cmake gtk3 glib2
```

## Get the code
```bash
git clone https://github.com/popalex/retropad.git retropad
cd retropad
```

## Build with CMake
```bash
mkdir build
cd build
cmake ..
make
```

This produces the `retropad` executable in the `build/` directory. Clean with:
```bash
make clean
# or to remove build directory entirely:
cd .. && rm -rf build
```

## Install (system-wide)
```bash
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make
sudo make install
```

Installs the binary to `/usr/local/bin`, the `.desktop` entry to `/usr/local/share/applications`, and the icon (if present) to the hicolor icon theme.

## Run
```bash
./build/retropad
# or with a file argument:
./build/retropad path/to/file.txt
```

## Features & notes
- Menus: File, Edit, Format, View, Help with standard keyboard shortcuts (Ctrl+N/O/S, Ctrl+F, Ctrl+H, etc.).
- Word wrap toggles text wrapping; status bar displays line/column numbers and encoding.
- Find/Replace bars with find next/previous and replace all functionality.
- Font picker for custom fonts and sizes.
- Time/date insertion (F5).
- Drag-and-drop file opening.
- Go To Line (Ctrl+G).
- Printing via GTK print dialog.
- File I/O: detects UTF-8/UTF-16LE/UTF-16BE/ANSI encodings via BOM; BOM-less files are validated as UTF-8 and fall back to ISO-8859-1 (ANSI) if invalid. Saves as **UTF-8 without BOM** by default (Linux-friendly); UTF-16 and ANSI files are round-tripped in their original encoding.
- Atomic save: file is written to a temporary path then renamed, preventing data loss on write failure.
- Error dialogs on load/save failure.
- Persistent preferences stored in `~/.config/retropad/config.ini` (window size, word wrap, status bar, line numbers, font). Loaded at startup, saved on exit and when changed.
- Recent files list (up to 5 entries) persisted in `~/.retropad_recent`.
- Custom undo/redo with smart grouping (groups consecutive typing, breaks on punctuation/500ms timeout).
- Line numbers can be toggled from View menu; rendering only iterates visible lines for efficiency.

## Project layout

```
src/
  main.c          — Entry point, window creation, main loop
  app_state.h/c   — Global AppState struct and initialization
  menu.h/c        — Menu bar, accelerators, all menu callbacks
  editor.h/c      — Text view, line numbers, drag-drop, buffer callbacks
  undo.h/c        — Smart undo/redo stack with grouping
  file_ops.h/c    — High-level file operations (New/Open/Save/Load)
  file_io.h/c     — Low-level BOM-aware file read/write with atomic save
  find_replace.h/c — Find/replace bar and search logic
  dialogs.h/c     — Go To Line, Font selection, About dialogs
  print.h/c       — GTK print support
  recent_files.h/c — MRU file list persistence and menu
  utils.h/c       — Shared helpers (UpdateTitle, UpdateStatusBar, etc.)
  prefs.h/c       — Preferences load/save via GKeyFile
CMakeLists.txt    — CMake build + install configuration
retropad.desktop  — freedesktop.org desktop entry for Linux integration
res/              — Application resources (icon)
```

## Design goals

- **Simple**: one window, one file, plain text only.
- **Fast**: minimal dependencies, GTK3, no heavy frameworks.
- **Linux-native**: UTF-8 without BOM, XDG config dir, `.desktop` integration.
- **Reliable**: atomic saves, error dialogs, persistent preferences.

## Non-goals

The following are intentionally out of scope:

- Syntax highlighting
- Tabs / multi-document interface
- Plugin system
- Split panes or file tree/sidebar
- Terminal integration
- Project/workspace concepts
- LSP or autocomplete

## Common build issues
- If GTK3 headers are not found, install `libgtk-3-dev` (Ubuntu/Debian) or equivalent for your distro.
- If CMake fails, ensure `cmake` is in your PATH: `cmake --version`.
- If compilation fails with undefined references, ensure all GTK3 libraries are linked: check CMakeLists.txt has the correct `target_link_libraries`.
- If `make` says "No targets specified and no makefile found" after running `cmake ..` from the build directory, there may be stale CMake cache files in the source directory. Clean them with:

  ```bash
  rm -rf CMakeCache.txt CMakeFiles cmake_install.cmake Makefile
  ```

  Then retry the build steps.
