// Editor widget implementation for retropad
#include "editor.h"
#include "app_state.h"
#include "undo.h"
#include "utils.h"
#include "file_ops.h"
#include "prefs.h"

static gboolean OnLineNumbersDraw(GtkWidget *widget, cairo_t *cr, gpointer data) {
    (void)data;
    
    GdkRGBA bg = {0.95, 0.95, 0.95, 1.0};
    GdkRGBA fg = {0.5, 0.5, 0.5, 1.0};
    
    gdk_cairo_set_source_rgba(cr, &bg);
    cairo_paint(cr);
    
    gdk_cairo_set_source_rgba(cr, &fg);
    
    PangoLayout *layout = gtk_widget_create_pango_layout(widget, NULL);
    if (g_app.fontDesc) {
        pango_layout_set_font_description(layout, g_app.fontDesc);
    }
    
    GdkRectangle visible;
    gtk_text_view_get_visible_rect(GTK_TEXT_VIEW(g_app.textView), &visible);
    
    /* Start iteration from the first visible line instead of buffer start */
    GtkTextIter iter;
    gtk_text_view_get_iter_at_location(GTK_TEXT_VIEW(g_app.textView), &iter,
                                       visible.x, visible.y);
    gtk_text_iter_set_line_offset(&iter, 0);
    
    int lineNum = gtk_text_iter_get_line(&iter) + 1;
    
    while (!gtk_text_iter_is_end(&iter)) {
        GdkRectangle loc;
        gtk_text_view_get_iter_location(GTK_TEXT_VIEW(g_app.textView), &iter, &loc);
        
        int winY;
        gtk_text_view_buffer_to_window_coords(GTK_TEXT_VIEW(g_app.textView),
            GTK_TEXT_WINDOW_WIDGET, 0, loc.y, NULL, &winY);
        
        /* Stop once we are past the visible area */
        if (winY > visible.height + loc.height) break;
        
        char numStr[16];
        snprintf(numStr, sizeof(numStr), "%d", lineNum);
        pango_layout_set_text(layout, numStr, -1);
        
        int tw, th;
        pango_layout_get_pixel_size(layout, &tw, &th);
        
        cairo_move_to(cr, LINE_NUMBER_MARGIN_WIDTH - tw - 5, winY);
        pango_cairo_show_layout(cr, layout);
        
        if (!gtk_text_iter_forward_line(&iter)) break;
        lineNum++;
    }
    
    g_object_unref(layout);
    return FALSE;
}

static void OnTextViewScrolled(GtkAdjustment *adj, gpointer data) {
    (void)adj;
    (void)data;
    if (g_app.lineNumbersVisible && g_app.lineNumberView) {
        gtk_widget_queue_draw(g_app.lineNumberView);
    }
}

void ToggleLineNumbers(gboolean visible) {
    g_app.lineNumbersVisible = visible;
    if (g_app.lineNumberView) {
        if (visible) {
            gtk_widget_show(g_app.lineNumberView);
        } else {
            gtk_widget_hide(g_app.lineNumberView);
        }
    }
    if (g_app.lineNumbersMenuItem) {
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(g_app.lineNumbersMenuItem), visible);
    }
    SavePrefs();
}

static void OnDragDataReceived(GtkWidget *widget, GdkDragContext *context,
                               gint x, gint y, GtkSelectionData *data,
                               guint info, guint time, gpointer user_data) {
    (void)widget;
    (void)x;
    (void)y;
    (void)info;
    (void)user_data;
    
    gchar **uris = gtk_selection_data_get_uris(data);
    if (uris && uris[0]) {
        gchar *path = g_filename_from_uri(uris[0], NULL, NULL);
        if (path) {
            if (PromptSaveChanges()) {
                LoadDocumentFromPath(path);
            }
            g_free(path);
        }
        g_strfreev(uris);
    }
    gtk_drag_finish(context, TRUE, FALSE, time);
}

void SetupDragAndDrop(void) {
    GtkTargetEntry targets[] = {
        {"text/uri-list", 0, 0}
    };
    gtk_drag_dest_set(g_app.textView, GTK_DEST_DEFAULT_ALL, targets, 1, GDK_ACTION_COPY);
    g_signal_connect(g_app.textView, "drag-data-received", G_CALLBACK(OnDragDataReceived), NULL);
}

void on_text_changed(GtkTextBuffer *buffer, gpointer user_data) {
    (void)buffer;
    (void)user_data;
    if (g_app.isProgrammaticChange) return;
    if (!g_app.isUndoRedoInProgress) {
        PushUndoStack();
        ClearRedoStack();
    }
    g_app.modified = TRUE;
    UpdateTitle();
    UpdateStatusBar();
}

void on_cursor_moved(GtkTextBuffer *buffer, GParamSpec *pspec, gpointer user_data) {
    (void)buffer;
    (void)pspec;
    (void)user_data;
    UpdateStatusBar();
}

GtkWidget* CreateEditorWidget(void) {
    /* Create text view with buffer */
    g_app.textBuffer = gtk_text_buffer_new(NULL);
    g_signal_connect(g_app.textBuffer, "changed", G_CALLBACK(on_text_changed), NULL);
    g_signal_connect(g_app.textBuffer, "notify::cursor-position",
        G_CALLBACK(on_cursor_moved), NULL);

    g_app.textView = gtk_text_view_new_with_buffer(g_app.textBuffer);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(g_app.textView), GTK_WRAP_WORD);

    /* Create HBox for line numbers + text view */
    GtkWidget *editorBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

    /* Line number margin */
    g_app.lineNumberView = gtk_drawing_area_new();
    gtk_widget_set_size_request(g_app.lineNumberView, LINE_NUMBER_MARGIN_WIDTH, -1);
    g_signal_connect(g_app.lineNumberView, "draw", G_CALLBACK(OnLineNumbersDraw), NULL);
    gtk_box_pack_start(GTK_BOX(editorBox), g_app.lineNumberView, FALSE, FALSE, 0);

    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
        GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scrolled), g_app.textView);
    gtk_box_pack_start(GTK_BOX(editorBox), scrolled, TRUE, TRUE, 0);

    /* Connect scroll adjustment to redraw line numbers */
    GtkAdjustment *vadj = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(scrolled));
    g_signal_connect(vadj, "value-changed", G_CALLBACK(OnTextViewScrolled), NULL);
    g_signal_connect(g_app.textBuffer, "changed", G_CALLBACK(OnTextViewScrolled), NULL);

    return editorBox;
}
