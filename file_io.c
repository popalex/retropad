// Text file load/save helpers with simple BOM detection for retropad.
#include "file_io.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <glib/gstdio.h>

static TextEncoding DetectEncoding(const guchar *data, gsize size) {
    if (size >= 2 && data[0] == 0xFF && data[1] == 0xFE) {
        return ENC_UTF16LE;
    }
    if (size >= 2 && data[0] == 0xFE && data[1] == 0xFF) {
        return ENC_UTF16BE;
    }
    if (size >= 3 && data[0] == 0xEF && data[1] == 0xBB && data[2] == 0xBF) {
        return ENC_UTF8;
    }
    // Assume UTF-8 for Linux
    return ENC_UTF8;
}

static gboolean DecodeToUTF8(const guchar *data, gsize size, TextEncoding encoding, char **outText, size_t *outLength) {
    char *result = NULL;
    GError *error = NULL;

    switch (encoding) {
    case ENC_UTF16LE: {
        if (size < 2) return FALSE;
        gsize byteOffset = (data[0] == 0xFF && data[1] == 0xFE) ? 2 : 0;
        result = g_convert((const gchar *)(data + byteOffset), size - byteOffset,
                          "UTF-8", "UTF-16LE", NULL, NULL, &error);
        break;
    }
    case ENC_UTF16BE: {
        if (size < 2) return FALSE;
        gsize byteOffset = (data[0] == 0xFE && data[1] == 0xFF) ? 2 : 0;
        result = g_convert((const gchar *)(data + byteOffset), size - byteOffset,
                          "UTF-8", "UTF-16BE", NULL, NULL, &error);
        break;
    }
    case ENC_UTF8: {
        gsize byteOffset = (size >= 3 && data[0] == 0xEF && data[1] == 0xBB && data[2] == 0xBF) ? 3 : 0;
        result = g_strndup((const gchar *)(data + byteOffset), size - byteOffset);
        break;
    }
    case ENC_ANSI:
    default: {
        result = g_convert((const gchar *)data, size, "UTF-8", "ISO-8859-1", NULL, NULL, &error);
        break;
    }
    }

    if (error) {
        g_error_free(error);
        g_free(result);
        return FALSE;
    }

    if (!result) return FALSE;
    *outText = result;
    if (outLength) {
        *outLength = strlen(result);
    }
    return TRUE;
}

gboolean LoadTextFile(void *owner, const char *path, char **textOut, size_t *lengthOut, TextEncoding *encodingOut) {
    *textOut = NULL;
    if (lengthOut) *lengthOut = 0;
    if (encodingOut) *encodingOut = ENC_UTF8;

    GError *error = NULL;
    gchar *buffer = NULL;
    gsize bytes = 0;

    if (!g_file_get_contents(path, &buffer, &bytes, &error)) {
        g_error_free(error);
        return FALSE;
    }

    if (bytes == 0) {
        char *empty = g_strdup("");
        *textOut = empty;
        if (lengthOut) *lengthOut = 0;
        if (encodingOut) *encodingOut = ENC_UTF8;
        g_free(buffer);
        return TRUE;
    }

    TextEncoding enc = DetectEncoding((const guchar *)buffer, bytes);
    char *text = NULL;
    size_t len = 0;
    if (!DecodeToUTF8((const guchar *)buffer, bytes, enc, &text, &len)) {
        g_free(buffer);
        return FALSE;
    }

    g_free(buffer);
    *textOut = text;
    if (lengthOut) *lengthOut = len;
    if (encodingOut) *encodingOut = enc;
    return TRUE;
}

static gboolean WriteUTF8WithBOM(FILE *file, const char *text, size_t length) {
    static const guchar bom[] = {0xEF, 0xBB, 0xBF};
    if (fwrite(bom, sizeof(bom), 1, file) != 1) {
        return FALSE;
    }
    if (fwrite(text, 1, length, file) != length) {
        return FALSE;
    }
    return TRUE;
}

static gboolean WriteUTF16LE(FILE *file, const char *text, size_t length) {
    static const guchar bom[] = {0xFF, 0xFE};
    if (fwrite(bom, sizeof(bom), 1, file) != 1) {
        return FALSE;
    }
    GError *error = NULL;
    gchar *converted = g_convert(text, length, "UTF-16LE", "UTF-8", NULL, NULL, &error);
    if (error) {
        g_error_free(error);
        return FALSE;
    }
    gsize conv_len = strlen(converted);
    gboolean ok = fwrite(converted, 1, conv_len, file) == conv_len;
    g_free(converted);
    return ok;
}

static gboolean WriteANSI(FILE *file, const char *text, size_t length) {
    GError *error = NULL;
    gchar *converted = g_convert(text, length, "ISO-8859-1", "UTF-8", NULL, NULL, &error);
    if (error) {
        g_error_free(error);
        return FALSE;
    }
    gsize conv_len = strlen(converted);
    gboolean ok = fwrite(converted, 1, conv_len, file) == conv_len;
    g_free(converted);
    return ok;
}

gboolean SaveTextFile(void *owner, const char *path, const char *text, size_t length, TextEncoding encoding) {
    FILE *file = g_fopen(path, "wb");
    if (!file) {
        return FALSE;
    }

    gboolean ok = FALSE;
    switch (encoding) {
    case ENC_UTF16LE:
        ok = WriteUTF16LE(file, text, length);
        break;
    case ENC_ANSI:
        ok = WriteANSI(file, text, length);
        break;
    case ENC_UTF16BE:
        // Saving as UTF-16BE is uncommon; fall back to UTF-8 with BOM to preserve readability
        ok = WriteUTF8WithBOM(file, text, length);
        encoding = ENC_UTF8;
        break;
    case ENC_UTF8:
    default:
        ok = WriteUTF8WithBOM(file, text, length);
        break;
    }

    fclose(file);
    return ok;
}
