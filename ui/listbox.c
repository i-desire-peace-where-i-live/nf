/* Author:			Sergey Simonenko <gforgx@protonmail.com>
 * SPDX-License-Identifier:	0BSD */

#include "listbox.h"

#include "../config.h"

#ifdef HAVE_GTK_3
#include <gtk/gtk.h>

#include "../entry.h"
#include "../util.h"

#define ENTRY_ICON_NOTE "emblem-documents"
#define ENTRY_ICON_BOOKMARK "user-bookmarks"
#define ENTRY_ICON_FILE "application-x-addon-symbolic"
#define ENTRY_ICON_UNKNOWN "action-unavailable-symbolic"

struct _NfListBox {
  GtkListBox parent;

  GtkWidget* image;
};

struct _NfListBoxRow {
  GtkBox parent;

  NfListBox* listbox;

  GtkWidget* hbox_above;
  GtkWidget* hbox_below;
  GtkWidget* name_label;
  GtkWidget* content_label;

  GtkWidget* date_label;
  GtkWidget* icon_widget;

  Entry* entry;
};

G_DEFINE_TYPE(NfListBox, nf_listbox, GTK_TYPE_LIST_BOX);
G_DEFINE_TYPE(NfListBoxRow, nf_listboxrow, GTK_TYPE_BOX);

static void nf_listbox_class_init(NfListBoxClass* class) {}
static void nf_listboxrow_class_init(NfListBoxRowClass* class) {}
static void nf_listbox_init(NfListBox* listbox) {}
static void nf_listboxrow_init(NfListBoxRow* row) {}

GtkWidget* listbox_new(void) {
  NfListBox* new;

  new = g_object_new(nf_listbox_get_type(), NULL);
  gtk_list_box_set_activate_on_single_click(GTK_LIST_BOX(new), FALSE);
  new->image = gtk_image_new();
  gtk_image_set_from_icon_name(GTK_IMAGE(new->image), ENTRY_ICON_NOTE,
                               GTK_ICON_SIZE_BUTTON);

  return GTK_WIDGET(new);
}

GtkWidget* listboxrow_new(NfListBox* listbox) {
  NfListBoxRow* new;

  new = g_object_new(nf_listboxrow_get_type(), "orientation",
                     GTK_ORIENTATION_VERTICAL, "spacing", 0, NULL);

  new->listbox = listbox;

  gtk_container_set_border_width(GTK_CONTAINER(new), 2);

  new->hbox_above = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_set_spacing(GTK_BOX(new->hbox_above), 2);

  new->hbox_below = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

  new->name_label = gtk_label_new(NULL);
  new->content_label = gtk_label_new(NULL);
  new->date_label = gtk_label_new("Feb 3 2023 18:40");
  new->icon_widget = listbox->image;  // gtk_image_new();

  //  gtk_box_pack_start(GTK_BOX(new->hbox_above), new->icon_widget, FALSE,
  //  FALSE,
  //                     0);
  gtk_box_pack_start(GTK_BOX(new->hbox_above), new->name_label, FALSE, FALSE,
                     0);
  gtk_box_pack_end(GTK_BOX(new->hbox_above), new->date_label, FALSE, FALSE, 0);

  gtk_container_add(GTK_CONTAINER(new->hbox_below), new->content_label);

  gtk_container_add(GTK_CONTAINER(new), new->hbox_above);
  gtk_container_add(GTK_CONTAINER(new), new->hbox_below);

  return GTK_WIDGET(new);
}

static void gtk_label_set_italic(GtkLabel* label, char* text) {
  char* markup =
      g_markup_printf_escaped("<span style=\"italic\">\%s</span>", text);

  gtk_label_set_markup(label, markup);
  g_free(markup);
}

void listboxrow_set_entry(NfListBoxRow* row, Entry* entry) {
  char* short_name;
  char* short_content = "...";

  row->entry = entry;

  short_name = get_first_n_chars(entry_get(entry, "name"), 100, "...");
  gtk_label_set_text(GTK_LABEL(row->name_label), short_name);
  /*
    const char* icon = ENTRY_ICON_UNKNOWN;
  */
  if (ENTRY_NOTE == entry->type) {
    //    icon = ENTRY_ICON_NOTE;
    short_content = get_first_n_chars(entry_get(entry, "content"), 50, "...");

    char* p = strchr(short_content, '\n');
    while (p) {
      *p = '\t';
      p = strchr(p, '\n');
    }
  } else if (ENTRY_BOOKMARK == entry->type) {
    //    icon = ENTRY_ICON_BOOKMARK;
    short_content = get_first_n_chars(entry_get(entry, "url"), 50, "...");
  } else if (ENTRY_FILE == entry->type) {
  }
  //    icon = ENTRY_ICON_FILE;

  /*
    gtk_image_set_from_icon_name(GTK_IMAGE(row->icon_widget), icon,
                                 GTK_ICON_SIZE_BUTTON);
  */
  gtk_label_set_italic(GTK_LABEL(row->content_label), short_content);
}

void listboxrow_update_entry_flags(NfListBoxRow* row, int new_flag) {
  row->entry->flags |= new_flag;
}

Entry* listboxrow_get_entry(NfListBoxRow* row) { return row->entry; }

static void listboxrow_remove_cb(GtkWidget* child, gpointer unused) {
  (void)unused;

  gtk_widget_destroy(child);
}

void listbox_clear(NfListBox* listbox) {
  gtk_container_foreach(GTK_CONTAINER(listbox), listboxrow_remove_cb, NULL);
}

void listbox_add_entry(NfListBox* listbox, Entry* entry) {
  GtkWidget* listboxrow;

  if (ENTRY_FLAGS_APP_DELETED == (entry->flags & ENTRY_FLAGS_APP_DELETED))
    return;

  listboxrow = listboxrow_new(listbox);
  listboxrow_set_entry(NF_LISTBOX_ROW(listboxrow), entry);

  gtk_list_box_insert(GTK_LIST_BOX(listbox), listboxrow, -1);
  gtk_widget_show_all(listboxrow);
}
#endif
