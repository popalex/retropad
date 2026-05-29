// Recent files management implementation for retropad
#include "recent_files.h"
#include "app_state.h"
#include "file_ops.h"
#include <stdio.h>
#include <string.h>

static void OnRecentFileActivate(GtkWidget *widget, gpointer data) {
    (void)widget;
    const char *path = (const char *)data;
    if (PromptSaveChanges()) {
        LoadDocumentFromPath(path);
    }
}

void LoadRecentFiles(void) {
    g_app.recentFiles = NULL;
    
    const char *home = g_get_home_dir();
    char path[MAX_PATH_BUFFER];
    snprintf(path, sizeof(path), "%s/%s", home, RECENT_FILES_PATH);
    
    FILE *f = fopen(path, "r");
    if (!f) return;
    
    char line[MAX_PATH_BUFFER];
    int count = 0;
    while (fgets(line, sizeof(line), f) && count < MAX_RECENT_FILES) {
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') line[len-1] = '\0';
        if (line[0] && g_file_test(line, G_FILE_TEST_EXISTS)) {
            g_app.recentFiles = g_list_append(g_app.recentFiles, g_strdup(line));
            count++;
        }
    }
    fclose(f);
}

void SaveRecentFiles(void) {
    const char *home = g_get_home_dir();
    char path[MAX_PATH_BUFFER];
    snprintf(path, sizeof(path), "%s/%s", home, RECENT_FILES_PATH);
    
    FILE *f = fopen(path, "w");
    if (!f) return;
    
    for (GList *l = g_app.recentFiles; l; l = l->next) {
        fprintf(f, "%s\n", (char *)l->data);
    }
    fclose(f);
}

void AddRecentFile(const char *path) {
    if (!path || path[0] == '\0') return;
    
    /* Remove if already exists */
    for (GList *l = g_app.recentFiles; l; l = l->next) {
        if (strcmp((char *)l->data, path) == 0) {
            g_free(l->data);
            g_app.recentFiles = g_list_delete_link(g_app.recentFiles, l);
            break;
        }
    }
    
    /* Add to front */
    g_app.recentFiles = g_list_prepend(g_app.recentFiles, g_strdup(path));
    
    /* Trim to max */
    while (g_list_length(g_app.recentFiles) > MAX_RECENT_FILES) {
        GList *last = g_list_last(g_app.recentFiles);
        g_free(last->data);
        g_app.recentFiles = g_list_delete_link(g_app.recentFiles, last);
    }
    
    SaveRecentFiles();
    UpdateRecentFilesMenu();
}

void UpdateRecentFilesMenu(void) {
    if (!g_app.recentFilesMenu) return;
    
    /* Clear existing items */
    GList *children = gtk_container_get_children(GTK_CONTAINER(g_app.recentFilesMenu));
    for (GList *l = children; l; l = l->next) {
        gtk_widget_destroy(GTK_WIDGET(l->data));
    }
    g_list_free(children);
    
    /* Add new items */
    if (!g_app.recentFiles) {
        GtkWidget *item = gtk_menu_item_new_with_label("(No recent files)");
        gtk_widget_set_sensitive(item, FALSE);
        gtk_menu_shell_append(GTK_MENU_SHELL(g_app.recentFilesMenu), item);
    } else {
        for (GList *l = g_app.recentFiles; l; l = l->next) {
            const char *path = (const char *)l->data;
            const char *name = strrchr(path, '/');
            name = name ? name + 1 : path;
            
            GtkWidget *item = gtk_menu_item_new_with_label(name);
            gtk_widget_set_tooltip_text(item, path);
            g_signal_connect(item, "activate", G_CALLBACK(OnRecentFileActivate), l->data);
            gtk_menu_shell_append(GTK_MENU_SHELL(g_app.recentFilesMenu), item);
        }
    }
    
    gtk_widget_show_all(g_app.recentFilesMenu);
}
