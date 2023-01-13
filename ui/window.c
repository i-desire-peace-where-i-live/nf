/* Author:			Sergey Simonenko <gforgx@protonmail.com>
 * SPDX-License-Identifier:	0BSD */

#include "../config.h"

#ifdef HAVE_GTK_3
#include <gtk/gtk.h>

#include "../common.h"
#include "../entry.h"
#include "../resource.h"
#include "../search.h"
#include "../slice.h"
#include "../util.h"
#include "editor.h"
#include "listbox.h"

#define RES_LOGO "/resources/images/notefinder.png"
#define ICON_BACK "go-previous-symbolic"
#define ICON_FWD "go-next-symbolic"
#define ICON_ABOUT "help-about-symbolic"
#define ICON_ENTRYACTION_DELETE "list-remove-symbolic"

#define ICON_ENTRY_NOTE "emblem-documents-symbolic"
#define ICON_ENTRY_BOOKMARK "user-bookmarks-symbolic"
#define ICON_ENTRY_FILE "application-x-addon-symbolic"
#define ICON_ENTRY_UNKNOWN "action-unavailable-symbolic"

extern Slice* sources;
extern char* last_query;
GtkWidget* win;
GtkWidget* listbox;
NfEditor* editor;

Slice* get_search_results(char* query);
static void display_entries(Slice* results);

gboolean refresh(void) {
  Slice* results = get_search_results(last_query);
  display_entries(results);
  slice_free(results, NULL);

  return TRUE;
}

static gboolean on_key_pressed_cb(GtkWidget* searchbar, GdkEvent* event) {
  if (GDK_CONTROL_MASK !=
      (event->key.state & gtk_accelerator_get_default_mod_mask()))
    return FALSE;

  if (GDK_KEY_q == event->key.keyval) {
    gtk_main_quit();
    return TRUE;
  } else if (GDK_KEY_f == event->key.keyval) {
    gtk_window_set_focus(GTK_WINDOW(win), searchbar);
    return TRUE;
  } else
    return FALSE;
}

static void add_cb(GtkButton* self, NfEditor* editor) {
  editor_set_entry(editor, NULL);
  gtk_widget_show_all(GTK_WIDGET(editor));

  editor_set_entry_visible(editor, TRUE);
}

static void delete_cb(gpointer unused, gpointer unused2) {
  (void)unused;
  (void)unused2;

  GtkListBoxRow* row = gtk_list_box_get_selected_row(GTK_LIST_BOX(listbox));

  if (!row) return;

  GtkWidget* child = gtk_bin_get_child(GTK_BIN(row));

  listboxrow_update_entry_flags(NF_LISTBOX_ROW(child), ENTRY_FLAGS_APP_DELETED);

  refresh();
}

static void about_cb(GSimpleAction* self, GVariant* parameter,
                     gpointer user_data) {
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

static void display_entries(Slice* results) {
  listbox_clear(NF_LISTBOX(listbox));

  for (size_t i = 0; i < results->len; i++) {
    Entry* entry = results->data[i];
    listbox_add_entry(NF_LISTBOX(listbox), entry);
  }
}

static void search_cb(GtkSearchEntry* search, gpointer user_data) {
  char* query = (char*)gtk_editable_get_chars(GTK_EDITABLE(search), 0, -1);
  Slice* results = get_search_results(query);

  free(last_query);
  last_query = query;

  display_entries(results);
  slice_free(results, NULL);
}

static void open_entry_cb(gpointer unused, gpointer unused2) {
  (void)unused;
  (void)unused2;

  GtkListBoxRow* row = gtk_list_box_get_selected_row(GTK_LIST_BOX(listbox));

  if (!row) return;

  GtkWidget* child = gtk_bin_get_child(GTK_BIN(row));
  Entry* entry = listboxrow_get_entry(NF_LISTBOX_ROW(child));

  char* content = entry_get(entry, "content");
  char* url = entry_get(entry, "url");
  char* filepath = entry_get(entry, "filepath");

  if (content) {
    editor_set_entry(editor, entry);
    gtk_widget_show_all(GTK_WIDGET(editor));
    return;
  }

  if (url) {
    gtk_show_uri_on_window(GTK_WINDOW(win), (const char*)url, GDK_CURRENT_TIME,
                           NULL);
    return;
  }

  if (filepath) {
    char fileuri[strlen(filepath) + strlen("file://") + 1];
    strlcpy(fileuri, "file://", sizeof(fileuri));
    strlcat(fileuri, filepath, sizeof(fileuri));

    gtk_show_uri_on_window(GTK_WINDOW(win), fileuri, GDK_CURRENT_TIME, NULL);
  }
}

int show_window(int argc, char* argv[]) {
  gtk_init(&argc, &argv);

  win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size(GTK_WINDOW(win), 1024, 768);

  /* This is nearly useless: all modern DEs including GNOME simply ignore
   * this; desktop entry file with Icon must be used instead */
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

  gtk_header_bar_pack_start(GTK_HEADER_BAR(header), hbox);

  GActionGroup* actions = (GActionGroup*)g_simple_action_group_new();
  gtk_widget_insert_action_group(win, "win", actions);

  const GActionEntry entries[] = {
      {"about", about_cb, NULL, NULL, NULL, {0, 0, 0}}};

  g_action_map_add_action_entries(G_ACTION_MAP(actions), entries,
                                  G_N_ELEMENTS(entries), win);

  GMenu* menu_model = g_menu_new();
  g_menu_append(menu_model, "Sync entries", "win.sync");
  g_menu_append(menu_model, "Settings", "win.settings");
  g_menu_append(menu_model, "About", "win.about");

  GtkWidget* menu_button = gtk_menu_button_new();
  gtk_menu_button_set_menu_model(GTK_MENU_BUTTON(menu_button),
                                 G_MENU_MODEL(menu_model));
  gtk_header_bar_pack_end(GTK_HEADER_BAR(header), menu_button);

  GtkWidget* delete_button = gtk_button_new_from_icon_name(
      ICON_ENTRYACTION_DELETE, GTK_ICON_SIZE_BUTTON);
  gtk_header_bar_pack_end(GTK_HEADER_BAR(header), delete_button);

  GtkWidget* add_button = gtk_button_new_with_mnemonic("_Add entry");
  gtk_header_bar_pack_end(GTK_HEADER_BAR(header), add_button);

  GtkWidget* vpaned = gtk_paned_new(GTK_ORIENTATION_VERTICAL);

  GtkWidget* search = gtk_search_entry_new();

  GtkWidget* scroller = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroller),
                                 GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

  listbox = listbox_new();

  Slice* results = get_search_results(NULL);
  display_entries(results);
  slice_free(results, NULL);

  gtk_container_add(GTK_CONTAINER(scroller), listbox);

  g_signal_connect(G_OBJECT(search), "search-changed", G_CALLBACK(search_cb),
                   NULL);

  gtk_paned_add1(GTK_PANED(vpaned), search);
  gtk_paned_add2(GTK_PANED(vpaned), scroller);

  gtk_container_add(GTK_CONTAINER(win), vpaned);

  editor = editor_new(GTK_WINDOW(win));

  g_signal_connect(G_OBJECT(add_button), "clicked", G_CALLBACK(add_cb), editor);
  g_signal_connect(G_OBJECT(delete_button), "clicked", G_CALLBACK(delete_cb),
                   NULL);
  // FIXME: rewrite below signal
  g_signal_connect(G_OBJECT(listbox), "row-activated",
                   G_CALLBACK(open_entry_cb), NULL);

  GtkAccelGroup* accel = gtk_accel_group_new();

  gtk_widget_add_accelerator(add_button, "clicked", accel, GDK_KEY_n,
                             GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  gtk_window_add_accel_group(GTK_WINDOW(win), accel);

  g_signal_connect(GTK_WIDGET(win), "key-press-event",
                   G_CALLBACK(on_key_pressed_cb), search);

  gtk_widget_show_all(win);

  //	FIXME: first implement communication between syncer and UI
  //	g_timeout_add(10000, refresh, entries_list);

  (void)gtk_main();
  return 0;
}

#endif
