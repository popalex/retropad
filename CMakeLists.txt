cmake_minimum_required(VERSION 3.12)
project(retropad C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Find GTK3
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK REQUIRED gtk+-3.0)

# Source files
set(SOURCES
    src/main.c
    src/app_state.c
    src/file_io.c
    src/file_ops.c
    src/undo.c
    src/utils.c
    src/find_replace.c
    src/dialogs.c
    src/print.c
    src/recent_files.c
    src/editor.c
    src/menu.c
    src/prefs.c
)

# Header files
set(HEADERS
    src/app_state.h
    src/file_io.h
    src/file_ops.h
    src/undo.h
    src/utils.h
    src/find_replace.h
    src/dialogs.h
    src/print.h
    src/recent_files.h
    src/editor.h
    src/menu.h
    src/prefs.h
)

add_executable(retropad ${SOURCES} ${HEADERS})

target_include_directories(retropad PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src ${GTK_INCLUDE_DIRS})
target_link_libraries(retropad PRIVATE ${GTK_LIBRARIES})
target_compile_options(retropad PRIVATE ${GTK_CFLAGS_OTHER} -Wall -Wextra)

# Install rules
include(GNUInstallDirs)
install(TARGETS retropad RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES retropad.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES res/retropad.png DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/256x256/apps
        OPTIONAL)