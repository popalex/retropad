// Dialog functions implementation for retropad
#include "dialogs.h"
#include "app_state.h"
#include "prefs.h"
#include <stdlib.h>

/* Filter input to only allow digits */
static void on_goto_entry_insert(GtkEditable *editable, const gchar *text,
                                  gint length, gint *position, gpointer data) {
    (void)position;
    (void)data;
    for (int i = 0; i < length; i++) {
        if (!g_ascii_isdigit(text[i])) {
            g_signal_stop_emission_by_name(editable, "insert-text");
            return;
        }
    }
}

/* Validate line number and update Go To button sensitivity */
static void on_goto_entry_changed(GtkEditable *editable, gpointer data) {
    GtkWidget *dialog = GTK_WIDGET(data);
    GtkWidget *goButton = gtk_dialog_get_widget_for_response(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT);
    
    const char *text = gtk_entry_get_text(GTK_ENTRY(editable));
    
    /* Get total line count */
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(g_app.textBuffer, &end);
    gint totalLines = gtk_text_iter_get_line(&end) + 1;
    
    /* Validate: must be a number between 1 and totalLines */
    gboolean valid = FALSE;
    if (text && text[0] != '\0') {
        char *endptr = NULL;
        long line = strtol(text, &endptr, 10);
        valid = (endptr != text && *endptr == '\0' && line >= 1 && line <= totalLines);
    }
    
    gtk_widget_set_sensitive(goButton, valid);
}

void DoGotoLine(void) {
    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "Go To Line", GTK_WINDOW(g_app.window),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Go To", GTK_RESPONSE_ACCEPT,
        NULL);
    
    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT);
    
    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    
    /* Get total line count for display */
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(g_app.textBuffer, &end);
    gint totalLines = gtk_text_iter_get_line(&end) + 1;
    
    char labelText[64];
    snprintf(labelText, sizeof(labelText), "Line number (1 - %d):", totalLines);
    GtkWidget *label = gtk_label_new(labelText);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    
    GtkWidget *entry = gtk_entry_new();
    gtk_entry_set_input_purpose(GTK_ENTRY(entry), GTK_INPUT_PURPOSE_DIGITS);
    gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE);
    
    /* Connect validation signals */
    g_signal_connect(entry, "insert-text", G_CALLBACK(on_goto_entry_insert), NULL);
    g_signal_connect(entry, "changed", G_CALLBACK(on_goto_entry_changed), dialog);
    
    /* Pre-fill with current line number */
    GtkTextIter cursor;
    gtk_text_buffer_get_iter_at_mark(g_app.textBuffer,
        &cursor, gtk_text_buffer_get_insert(g_app.textBuffer));
    gint currentLine = gtk_text_iter_get_line(&cursor) + 1;
    char lineStr[16];
    snprintf(lineStr, sizeof(lineStr), "%d", currentLine);
    gtk_entry_set_text(GTK_ENTRY(entry), lineStr);
    gtk_editable_select_region(GTK_EDITABLE(entry), 0, -1);
    
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), entry, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(content), vbox);
    
    gtk_widget_show_all(dialog);
    
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        const char *text = gtk_entry_get_text(GTK_ENTRY(entry));
        char *endptr = NULL;
        long line = strtol(text, &endptr, 10);
        if (endptr != text && *endptr == '\0' && line >= 1 && line <= totalLines) {
            GtkTextIter iter;
            gtk_text_buffer_get_iter_at_line(g_app.textBuffer, &iter, line - 1);
            gtk_text_buffer_place_cursor(g_app.textBuffer, &iter);
            gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(g_app.textView), &iter, 0.1, TRUE, 0, 0.5);
        }
    }
    gtk_widget_destroy(dialog);
}

void DoSelectFont(void) {
    GtkWidget *dialog = gtk_font_chooser_dialog_new(
        "Select Font", GTK_WINDOW(g_app.window));

    if (g_app.fontDesc) {
        gtk_font_chooser_set_font_desc(GTK_FONT_CHOOSER(dialog), g_app.fontDesc);
    }

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        PangoFontDescription *fontDesc =
            gtk_font_chooser_get_font_desc(GTK_FONT_CHOOSER(dialog));
        if (fontDesc) {
            if (g_app.fontDesc) {
                pango_font_description_free(g_app.fontDesc);
            }
            g_app.fontDesc = pango_font_description_copy(fontDesc);
            pango_font_description_free(fontDesc);

            gchar *font_name = pango_font_description_to_string(g_app.fontDesc);
            gchar *css = g_strdup_printf("textview { font: %s; }", font_name);

            if (!g_app.cssProvider) {
                g_app.cssProvider = gtk_css_provider_new();
                GtkStyleContext *context = gtk_widget_get_style_context(g_app.textView);
                gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(g_app.cssProvider),
                                               GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
            }
            gtk_css_provider_load_from_data(g_app.cssProvider, css, -1, NULL);
            
            g_free(css);
            g_free(font_name);
            SavePrefs();
        }
    }
    gtk_widget_destroy(dialog);
}

void ShowAboutDialog(void) {
    GtkWidget *dialog = gtk_message_dialog_new(
        GTK_WINDOW(g_app.window),
        GTK_DIALOG_MODAL,
        GTK_MESSAGE_INFO,
        GTK_BUTTONS_OK,
        "retropad\n\nA Petzold-style notepad clone for Linux");
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}
