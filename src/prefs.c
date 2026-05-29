// Preferences load/save implementation for retropad using GKeyFile.
// Persists: window size, word wrap, status bar visibility,
// line numbers visibility, and font.
#include "prefs.h"
#include "app_state.h"
#include "utils.h"
#include "editor.h"
#include <glib.h>
#include <glib/gstdio.h>

#define PREFS_GROUP "retropad"
#define PREFS_FILE  "config.ini"

/* Guard to prevent SavePrefs re-entrance during LoadPrefs */
static gboolean g_loading_prefs = FALSE;

static gchar *GetPrefsPath(void) {
    return g_build_filename(g_get_user_config_dir(), "retropad", PREFS_FILE, NULL);
}

void LoadPrefs(void) {
    gchar *path = GetPrefsPath();
    GKeyFile *kf = g_key_file_new();

    if (!g_key_file_load_from_file(kf, path, G_KEY_FILE_NONE, NULL)) {
        /* No config file yet; use compiled-in defaults */
        g_key_file_free(kf);
        g_free(path);
        return;
    }

    g_loading_prefs = TRUE;

    GError *err = NULL;

    /* Window size */
    gint w = g_key_file_get_integer(kf, PREFS_GROUP, "width", &err);
    if (!err && w > 0) {
        g_clear_error(&err);
        gint h = g_key_file_get_integer(kf, PREFS_GROUP, "height", &err);
        if (!err && h > 0 && g_app.window) {
            gtk_window_resize(GTK_WINDOW(g_app.window), w, h);
        }
    }
    g_clear_error(&err);

    /* Word wrap */
    gboolean ww = g_key_file_get_boolean(kf, PREFS_GROUP, "word_wrap", &err);
    if (!err) {
        SetWordWrap(ww);
        if (g_app.wordWrapMenuItem) {
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(g_app.wordWrapMenuItem), ww);
        }
    }
    g_clear_error(&err);

    /* Status bar */
    gboolean sb = g_key_file_get_boolean(kf, PREFS_GROUP, "status_bar", &err);
    if (!err) {
        ToggleStatusBar(sb);
        if (g_app.statusBarMenuItem) {
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(g_app.statusBarMenuItem), sb);
        }
    }
    g_clear_error(&err);

    /* Line numbers */
    gboolean ln = g_key_file_get_boolean(kf, PREFS_GROUP, "line_numbers", &err);
    if (!err) {
        ToggleLineNumbers(ln);
    }
    g_clear_error(&err);

    /* Font */
    gchar *font = g_key_file_get_string(kf, PREFS_GROUP, "font", &err);
    if (!err && font && font[0] != '\0') {
        PangoFontDescription *desc = pango_font_description_from_string(font);
        if (desc) {
            if (g_app.fontDesc) {
                pango_font_description_free(g_app.fontDesc);
            }
            g_app.fontDesc = desc;

            if (g_app.textView) {
                GtkCssProvider *provider = gtk_css_provider_new();
                gchar *css = g_strdup_printf("textview { font: %s; }", font);
                gtk_css_provider_load_from_data(provider, css, -1, NULL);
                GtkStyleContext *ctx = gtk_widget_get_style_context(g_app.textView);
                gtk_style_context_add_provider(ctx, GTK_STYLE_PROVIDER(provider),
                                               GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
                g_free(css);
                g_object_unref(provider);
            }
        }
    }
    g_clear_error(&err);
    g_free(font);

    g_loading_prefs = FALSE;

    g_key_file_free(kf);
    g_free(path);
}

void SavePrefs(void) {
    if (g_loading_prefs) return;

    gchar *dir = g_build_filename(g_get_user_config_dir(), "retropad", NULL);
    if (g_mkdir_with_parents(dir, 0700) != 0) {
        g_free(dir);
        return;
    }
    g_free(dir);

    gchar *path = GetPrefsPath();
    GKeyFile *kf = g_key_file_new();

    /* Window size */
    if (g_app.window) {
        gint w = 0, h = 0;
        gtk_window_get_size(GTK_WINDOW(g_app.window), &w, &h);
        g_key_file_set_integer(kf, PREFS_GROUP, "width", w);
        g_key_file_set_integer(kf, PREFS_GROUP, "height", h);
    }

    g_key_file_set_boolean(kf, PREFS_GROUP, "word_wrap", g_app.wordWrap);
    g_key_file_set_boolean(kf, PREFS_GROUP, "status_bar", g_app.statusVisible);
    g_key_file_set_boolean(kf, PREFS_GROUP, "line_numbers", g_app.lineNumbersVisible);

    if (g_app.fontDesc) {
        gchar *font = pango_font_description_to_string(g_app.fontDesc);
        g_key_file_set_string(kf, PREFS_GROUP, "font", font);
        g_free(font);
    }

    GError *err = NULL;
    gchar *data = g_key_file_to_data(kf, NULL, &err);
    if (!err && data) {
        g_file_set_contents(path, data, -1, NULL);
    }
    g_clear_error(&err);
    g_free(data);
    g_key_file_free(kf);
    g_free(path);
}
