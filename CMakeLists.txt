cmake_minimum_required(VERSION 3.12)
project(retropad C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Find GTK3
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK REQUIRED gtk+-3.0)

# Sources
set(SOURCES
  retropad.c
  file_io.c
)

set(HEADERS
  file_io.h
)

add_executable(retropad ${SOURCES} ${HEADERS})

target_include_directories(retropad PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${GTK_INCLUDE_DIRS})
target_link_libraries(retropad PRIVATE ${GTK_LIBRARIES})
target_compile_options(retropad PRIVATE ${GTK_CFLAGS_OTHER})