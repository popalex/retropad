// File loading and saving helpers for retropad
#pragma once

#include <glib.h>
#include <stdbool.h>
#include <stddef.h>

typedef enum TextEncoding {
    ENC_UTF8 = 1,
    ENC_UTF16LE = 2,
    ENC_UTF16BE = 3,
    ENC_ANSI = 4
} TextEncoding;

typedef struct FileResult {
    char path[4096];
    TextEncoding encoding;
} FileResult;

gboolean LoadTextFile(void *owner, const char *path, char **textOut, size_t *lengthOut, TextEncoding *encodingOut);
gboolean SaveTextFile(void *owner, const char *path, const char *text, size_t length, TextEncoding encoding);
