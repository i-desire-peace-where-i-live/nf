/* Author:			Sergey Simonenko <gforgx@protonmail.com>
 * SPDX-License-Identifier:	0BSD */

#include <gtk/gtk.h>

#include "common.h"
#include "slice.h"
#include "entry.h"
#include "util.h"
#include "search.h"
#include "editor.h"
#include "resource.h"

#define RES_LOGO "/resources/images/notefinder.png"
#define ICON_BACK "go-previous-symbolic"
#define ICON_FWD "go-next-symbolic"
#define ICON_ABOUT "help-about-symbolic"
#define ICON_ENTRY_NOTE "emblem-documents-symbolic"
#define ICON_ENTRY_BOOKMARK "user-bookmarks-symbolic"
#define ICON_ENTRY_FILE "application-x-addon-symbolic"
#define ICON_ENTRY_UNKNOWN "action-unavailable-symbolic"
#define ICON_ENTRYACTION_DELETE "list-remove-symbolic"

GtkWidget* win;
GtkWidget* entries_list;

extern EditorState* editor_state;

enum Columns {
	LIST_ICON,
	LIST_ITEM,
	LIST_URL,
	LIST_ENTRY_PTR,
	NUM_COLUMNS
};

Slice* get_search_results(char* query);

extern VarsStruct* vars;

static void display_entries(GtkWidget* list, Slice* results);

gboolean refresh(gpointer entries_list) {
        Slice* results = get_search_results(vars->last_query);
	display_entries(entries_list, results);
	slice_free(results, NULL);

	return TRUE;
}

static gboolean on_key_pressed_cb(GtkWidget* searchbar, GdkEvent* event) {
	if (GDK_CONTROL_MASK != (event->key.state & gtk_accelerator_get_default_mod_mask()))
		return FALSE;

	if (GDK_KEY_q == event->key.keyval) {
		gtk_main_quit();
		return TRUE;
	} else if (GDK_KEY_f == event->key.keyval) {
		printf("Wanna search!\n");
		gtk_window_set_focus(GTK_WINDOW(win), searchbar);
		return TRUE;
	} else
		return FALSE;
}

static void add_cb(GtkButton* self, gpointer editor) {
	set_editor_state(NULL);
	gtk_widget_show_all(editor);

	gtk_widget_set_visible(editor_state->entry, TRUE);
}

static void about_cb(GSimpleAction* self, GVariant* parameter, gpointer user_data) {
	GdkPixbuf* logo = gdk_pixbuf_new_from_resource(RES_LOGO, NULL);
	GtkWidget* about = gtk_about_dialog_new();
	const char* program_authors[] = {"Sergey Simonenko", NULL};
	const char* program_artists[] = {"Amédée de Caranza", NULL};

	gtk_about_dialog_set_program_name(GTK_ABOUT_DIALOG(about), program_name);
	gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(about), program_ver);
	gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(about), program_desc);

	gtk_about_dialog_set_logo(GTK_ABOUT_DIALOG(about), logo);
	g_object_unref(G_OBJECT(logo));

	gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(about), program_authors);
	gtk_about_dialog_set_artists(GTK_ABOUT_DIALOG(about), program_artists);
	gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(about), program_copyright);
	gtk_about_dialog_set_license(GTK_ABOUT_DIALOG(about), program_license);
	gtk_about_dialog_set_wrap_license(GTK_ABOUT_DIALOG(about), TRUE);
	gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(about), program_website);

	gtk_widget_show(about);
}

static char* get_short_content_string(Entry* e) {
	if (e->url)
		return e->url;

	if (!e->content)
		return NULL;

	char* tmp = get_first_n_chars(e->content, 100, "...");

	char* p = strchr(tmp, '\n');
	while (p) {
		*p = '\t';
		p = strchr(p, '\n');
	}

	return tmp;
}

static const char* get_entry_icon_name(Entry* e) {
	if (e->url)
		return ICON_ENTRY_BOOKMARK;

	if (e->content)
		return ICON_ENTRY_NOTE;

	if (e->filepath)
		return ICON_ENTRY_FILE;

	return ICON_ENTRY_UNKNOWN;
}

static void display_entries(GtkWidget* list, Slice* results) {
	GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(list));
        GtkTreeIter iter;

	gtk_list_store_clear(GTK_LIST_STORE(model));

	for (int i = 0; i < results->len; i++) {
		Entry* entry = results->data[i];
		gtk_list_store_append(GTK_LIST_STORE(model), &iter);

		char* short_content = get_short_content_string(entry);

		gtk_list_store_set(GTK_LIST_STORE(model), &iter,
				LIST_ICON, get_entry_icon_name(entry),
				LIST_ITEM, entry->name,
				LIST_URL, (short_content ? short_content : ""),
				LIST_ENTRY_PTR, (gpointer)entry, -1);
	}
}

static void search_cb(GtkSearchEntry* search, gpointer user_data) {
	char* query = (char*)gtk_editable_get_chars(GTK_EDITABLE(search), 0, -1);
        Slice* results = get_search_results(query);

	free(vars->last_query);
	vars->last_query = query;

	display_entries(GTK_WIDGET(user_data), results);
	slice_free(results, NULL);
}

static void open_entry_cb(GtkTreeView* entries_list,
		GtkTreePath* path,
		GtkTreeViewColumn* column,
		gpointer user_data) {
        GtkTreeModel* model = gtk_tree_view_get_model(entries_list);
	GtkTreeIter iter;

	gtk_tree_model_get_iter(GTK_TREE_MODEL(model), &iter, path);

	GValue value = {0,};
	gtk_tree_model_get_value(model, &iter, LIST_ENTRY_PTR, &value);

	gpointer p = g_value_get_pointer(&value);
	if (!p)
		return;

	Entry* entry = p;

	if (entry->content) {
		set_editor_state(entry);
		GtkWidget* editor = user_data;
		gtk_widget_show_all(editor);
		gtk_widget_set_visible(editor_state->entry, FALSE);
		return;
	}

	if (entry->url) {
		gtk_show_uri_on_window(GTK_WINDOW(win), (const char*)entry->url, GDK_CURRENT_TIME, NULL);
		return;
	}

	if (entry->filepath) {
		char fileuri[strlen(entry->filepath) + strlen("file://") + 1];
		strlcpy(fileuri, "file://", sizeof(fileuri));
		strlcat(fileuri, entry->filepath, sizeof(fileuri));

		gtk_show_uri_on_window(GTK_WINDOW(win), fileuri, GDK_CURRENT_TIME, NULL);
	}
}

int show_window(int argc, char* argv[]) {
	gtk_init(&argc, &argv);

	win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size(GTK_WINDOW(win), 1024, 768);

	/* This is nearly useless: all modern DEs including GNOME simply ignore this;
	 * desktop entry file with Icon must be used instead */
	gtk_window_set_icon(GTK_WINDOW(win),
			gdk_pixbuf_new_from_resource(RES_LOGO, NULL));

	g_signal_connect(G_OBJECT(win), "destroy", G_CALLBACK(gtk_main_quit), NULL);

	GtkWidget* header = gtk_header_bar_new();
	gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(header), true);
	gtk_header_bar_set_title(GTK_HEADER_BAR(header),
			"Notes and other personal information");
	gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(header), true);

	gtk_window_set_titlebar(GTK_WINDOW(win), header);

	GtkWidget* hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	GtkStyleContext* sc = gtk_widget_get_style_context(hbox);
	gtk_style_context_add_class(sc, "linked");

#if 0
	GtkWidget* back_button = gtk_button_new();
	GIcon* back_icon = g_themed_icon_new(ICON_BACK);
	GtkWidget* back_img = gtk_image_new_from_gicon(back_icon, GTK_ICON_SIZE_BUTTON);
	g_object_unref(G_OBJECT(back_icon));
	gtk_container_add(GTK_CONTAINER(back_button), back_img);
	gtk_container_add(GTK_CONTAINER(hbox), back_button);


	GtkWidget* fwd_button = gtk_button_new();
	GIcon* fwd_icon = g_themed_icon_new(ICON_FWD);
	GtkWidget* fwd_img = gtk_image_new_from_gicon(fwd_icon, GTK_ICON_SIZE_BUTTON);
	g_object_unref(G_OBJECT(fwd_icon));
	gtk_container_add(GTK_CONTAINER(fwd_button), fwd_img);
	gtk_container_add(GTK_CONTAINER(hbox), fwd_button);
#endif

	gtk_header_bar_pack_start(GTK_HEADER_BAR(header), hbox);

	GActionGroup* actions = (GActionGroup*)g_simple_action_group_new();
	gtk_widget_insert_action_group(win, "win", actions);

	const GActionEntry entries[] = {
		{"about", about_cb, NULL, NULL, NULL, {0, 0, 0}}
	};

	g_action_map_add_action_entries(G_ACTION_MAP(actions), entries,
			G_N_ELEMENTS(entries), win);

	GMenu* menu_model = g_menu_new();
	g_menu_append(menu_model, "Sync entries", "win.sync");
	g_menu_append(menu_model, "Settings", "win.settings");
	g_menu_append(menu_model, "About", "win.about");

	GtkWidget* menu_button = gtk_menu_button_new();
	gtk_menu_button_set_menu_model(GTK_MENU_BUTTON(menu_button), G_MENU_MODEL(menu_model));
	gtk_header_bar_pack_end(GTK_HEADER_BAR(header), menu_button);

	GtkWidget* delete_button = gtk_button_new_from_icon_name(
			ICON_ENTRYACTION_DELETE, GTK_ICON_SIZE_BUTTON);
	gtk_header_bar_pack_end(GTK_HEADER_BAR(header), delete_button);


	GtkWidget* add_button = gtk_button_new_with_mnemonic("_Add entry");
	gtk_header_bar_pack_end(GTK_HEADER_BAR(header), add_button);

	GtkWidget* vpaned = gtk_paned_new(GTK_ORIENTATION_VERTICAL);
	
	GtkWidget* search = gtk_search_entry_new();

	GtkWidget* scroller = gtk_scrolled_window_new(NULL, NULL);

	entries_list = gtk_tree_view_new();
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(entries_list), TRUE);

	GtkListStore* entries_store = gtk_list_store_new(NUM_COLUMNS,
			G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER);
	gtk_tree_view_set_model(GTK_TREE_VIEW(entries_list), GTK_TREE_MODEL(entries_store));
	g_object_unref(G_OBJECT(entries_store));

	GtkCellRenderer* icon_renderer = gtk_cell_renderer_pixbuf_new();
	GtkTreeViewColumn* icon_column = gtk_tree_view_column_new_with_attributes(NULL,
			icon_renderer, "icon-name", LIST_ICON, NULL);

	GtkCellRenderer* name_renderer = gtk_cell_renderer_text_new();
	GtkTreeViewColumn* name_column = gtk_tree_view_column_new_with_attributes("Name",
			name_renderer, "text", LIST_ITEM, NULL);
	gtk_tree_view_column_set_max_width(name_column, 700);
	gtk_tree_view_column_set_resizable(name_column, TRUE);

	GtkCellRenderer* short_content_renderer = gtk_cell_renderer_text_new();
	GtkTreeViewColumn* short_content_column = gtk_tree_view_column_new_with_attributes(
			"Content", short_content_renderer, "text", LIST_URL, NULL);

	gtk_tree_view_append_column(GTK_TREE_VIEW(entries_list), icon_column);
	gtk_tree_view_append_column(GTK_TREE_VIEW(entries_list), name_column);
	gtk_tree_view_append_column(GTK_TREE_VIEW(entries_list), short_content_column);

        Slice* results = get_search_results(NULL);
	display_entries(entries_list, results);
	slice_free(results, NULL);

	gtk_container_add(GTK_CONTAINER(scroller), entries_list);

	g_signal_connect(G_OBJECT(search), "search-changed", G_CALLBACK(search_cb), G_OBJECT(entries_list));

	gtk_paned_add1(GTK_PANED(vpaned), search);
	gtk_paned_add2(GTK_PANED(vpaned), scroller);

	gtk_container_add(GTK_CONTAINER(win), vpaned);

	GtkWidget* editor = make_editor_window();

	g_signal_connect(G_OBJECT(add_button), "clicked", G_CALLBACK(add_cb), G_OBJECT(editor));
	g_signal_connect(entries_list, "row-activated", G_CALLBACK(open_entry_cb), editor);

	GtkAccelGroup* accel = gtk_accel_group_new();

	gtk_widget_add_accelerator(add_button, "clicked", accel,
			GDK_KEY_n, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

	gtk_window_add_accel_group(GTK_WINDOW(win), accel);

	g_signal_connect(GTK_WIDGET(win), "key-press-event", G_CALLBACK(on_key_pressed_cb), search);

	gtk_widget_show_all(win);

	g_timeout_add(10000, refresh, entries_list);

	(void)gtk_main();
	return 0;
}

