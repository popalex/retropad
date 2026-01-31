// Print support implementation for retropad
#include "print.h"
#include "app_state.h"
#include "utils.h"

static GtkPrintSettings *g_printSettings = NULL;
static GtkPageSetup *g_pageSetup = NULL;

static void DrawPage(GtkPrintOperation *operation, GtkPrintContext *context, gint page_nr, gpointer user_data) {
    (void)operation;
    (void)page_nr;
    (void)user_data;
    
    cairo_t *cr = gtk_print_context_get_cairo_context(context);
    gdouble width = gtk_print_context_get_width(context);
    
    PangoLayout *layout = gtk_print_context_create_pango_layout(context);
    
    if (g_app.fontDesc) {
        pango_layout_set_font_description(layout, g_app.fontDesc);
    } else {
        PangoFontDescription *desc = pango_font_description_from_string("Monospace 10");
        pango_layout_set_font_description(layout, desc);
        pango_font_description_free(desc);
    }
    
    pango_layout_set_width(layout, width * PANGO_SCALE);
    pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);
    
    char *text = NULL;
    GetEditText(&text, NULL);
    if (text) {
        pango_layout_set_text(layout, text, -1);
        g_free(text);
    }
    
    cairo_move_to(cr, 0, 0);
    pango_cairo_show_layout(cr, layout);
    
    g_object_unref(layout);
}

void DoPrint(void) {
    GtkPrintOperation *print = gtk_print_operation_new();
    
    if (g_printSettings) {
        gtk_print_operation_set_print_settings(print, g_printSettings);
    }
    if (g_pageSetup) {
        gtk_print_operation_set_default_page_setup(print, g_pageSetup);
    }
    
    gtk_print_operation_set_n_pages(print, 1);  /* Simplified: single page */
    gtk_print_operation_set_unit(print, GTK_UNIT_POINTS);
    
    g_signal_connect(print, "draw-page", G_CALLBACK(DrawPage), NULL);
    
    GtkPrintOperationResult res = gtk_print_operation_run(print,
        GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG,
        GTK_WINDOW(g_app.window), NULL);
    
    if (res == GTK_PRINT_OPERATION_RESULT_APPLY) {
        if (g_printSettings) g_object_unref(g_printSettings);
        g_printSettings = g_object_ref(gtk_print_operation_get_print_settings(print));
    }
    
    g_object_unref(print);
}

void DoPageSetup(void) {
    GtkPageSetup *newSetup = gtk_print_run_page_setup_dialog(
        GTK_WINDOW(g_app.window), g_pageSetup, g_printSettings);
    
    if (g_pageSetup) g_object_unref(g_pageSetup);
    g_pageSetup = newSetup;
}

void PrintCleanup(void) {
    if (g_printSettings) g_object_unref(g_printSettings);
    if (g_pageSetup) g_object_unref(g_pageSetup);
}
