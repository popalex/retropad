# retropad

A Petzold-style Notepad clone written in mostly plain C for Linux using GTK3. It keeps the classic menus, accelerators, word wrap toggle, status bar, find/replace, font picker, time/date insertion, and BOM-aware load/save. Printing is intentionally omitted.

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
git clone https://github.com/your/repo.git retropad
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

## Run
```bash
./build/retropad
# or after building, from the build directory:
./retropad
```

## Features & notes
- Menus: File, Edit, Format, View, Help with standard keyboard shortcuts (Ctrl+N/O/S, Ctrl+F, Ctrl+H, etc.).
- Word Wrap toggles text wrapping; status bar displays line and column numbers.
- Find/Replace bars with find next/previous and replace all functionality.
- Font picker for custom fonts and sizes.
- Time/date insertion.
- File I/O: detects UTF-8/UTF-16/ANSI encodings via BOM detection; saves with UTF-8 BOM by default.
- Status bar shows current line/column and total line count.
- Cut, copy, paste, select all with clipboard integration.

## Project layout
- `retropad.c` — main application, GTK3 UI, window setup, menus, callbacks.
- `file_io.c/.h` — GTK3 file dialogs and encoding-aware load/save helpers.
- `CMakeLists.txt` — CMake build configuration with GTK3 dependencies.
- `build/` — generated build artifacts and executable (after building).

## Common build issues
- If GTK3 headers are not found, install `libgtk-3-dev` (Ubuntu/Debian) or equivalent for your distro.
- If CMake fails, ensure `cmake` is in your PATH: `cmake --version`.
- If compilation fails with undefined references, ensure all GTK3 libraries are linked: check CMakeLists.txt has the correct `target_link_libraries`.
- If `make` says "No targets specified and no makefile found" after running `cmake ..` from the build directory, there may be stale CMake cache files in the source directory. Clean them with:

  ```bash
  rm -rf CMakeCache.txt CMakeFiles cmake_install.cmake Makefile
  ```

  Then retry the build steps.

## Notes
- Custom undo/redo implementation with smart grouping (groups consecutive typing, breaks on punctuation/500ms timeout).
- Drag-and-drop file opening is supported.
- Line numbers can be toggled from View menu.
