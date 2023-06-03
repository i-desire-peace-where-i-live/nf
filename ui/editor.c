/* Author:			Sergey Simonenko <gforgx@protonmail.com>
 * SPDX-License-Identifier:	0BSD */

#include "../config.h"

#ifdef HAVE_GTK_3
#include <gtk/gtk.h>

#include "../entry.h"
#include "../source.h"
#include "../util.h"
#include "editor.h"

#define RENAME_ICON "error-correct-symbolic"

extern Slice* sources;
gboolean refresh(void);

struct _NfEditor {
  GtkWindow parent;

  GtkWidget* vpaned;
  GtkWidget* header;
  GtkWidget* entry;
  GtkWidget* scroller;
  GtkWidget* textview;
  GtkWidget* save_button;
  GtkWidget* save_and_close_button;
  GtkWidget* rename_button;

  const char* current_name;
  Entry* current_entry;

  Slice* history;
};

G_DEFINE_TYPE(NfEditor, nf_editor, GTK_TYPE_WINDOW);

static void nf_editor_class_init(NfEditorClass* class) {}
static void nf_editor_init(NfEditor* editor) {}

gboolean editor_hide_cb(NfEditor*, gpointer);
void edited_cb(gpointer, NfEditor*);
static void rename_cb(GtkWidget*, GtkWidget*);
void save_entry_cb(gpointer, NfEditor*);
void save_entry_and_close_cb(gpointer, gpointer);

NfEditor* editor_new(GtkWindow* parent) {
  NfEditor* new;

  new = g_object_new(nf_editor_get_type(), NULL);
  gtk_window_set_transient_for(GTK_WINDOW(new), parent);
  gtk_window_set_default_size(GTK_WINDOW(new), 512, 384);

  new->header = gtk_header_bar_new();
  gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(new->header), true);
  gtk_header_bar_set_title(GTK_HEADER_BAR(new->header), "Untitled");
  gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(new->header), true);

  new->save_and_close_button = gtk_button_new_with_mnemonic("Save and _close");
  gtk_header_bar_pack_end(GTK_HEADER_BAR(new->header),
                          new->save_and_close_button);

  new->save_button = gtk_button_new_with_mnemonic("_Save");
  gtk_header_bar_pack_end(GTK_HEADER_BAR(new->header), new->save_button);

  new->rename_button =
      gtk_button_new_from_icon_name(RENAME_ICON, GTK_ICON_SIZE_BUTTON);
  gtk_header_bar_pack_end(GTK_HEADER_BAR(new->header), new->rename_button);

  gtk_window_set_titlebar(GTK_WINDOW(new), new->header);

  new->vpaned = gtk_paned_new(GTK_ORIENTATION_VERTICAL);

  new->entry = gtk_entry_new();
  gtk_paned_add1(GTK_PANED(new->vpaned), new->entry);

  new->scroller = gtk_scrolled_window_new(NULL, NULL);

  new->textview = gtk_text_view_new();
  GtkTextBuffer* buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(new->textview));

  g_signal_connect(GTK_EDITABLE(new->entry), "changed", G_CALLBACK(edited_cb),
                   new);
  g_signal_connect(G_OBJECT(buf), "changed", G_CALLBACK(edited_cb), new);

  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(new->textview), GTK_WRAP_WORD);
  gtk_text_view_set_left_margin(GTK_TEXT_VIEW(new->textview), 8);
  gtk_text_view_set_right_margin(GTK_TEXT_VIEW(new->textview), 8);
  gtk_text_view_set_top_margin(GTK_TEXT_VIEW(new->textview), 8);
  gtk_text_view_set_bottom_margin(GTK_TEXT_VIEW(new->textview), 8);

  gtk_container_add(GTK_CONTAINER(new->scroller), new->textview);
  gtk_paned_add2(GTK_PANED(new->vpaned), new->scroller);

  gtk_container_add(GTK_CONTAINER(new), new->vpaned);

  g_signal_connect(G_OBJECT(new->save_button), "clicked",
                   G_CALLBACK(save_entry_cb), new);
  g_signal_connect(G_OBJECT(new->save_and_close_button), "clicked",
                   G_CALLBACK(save_entry_and_close_cb), new);
  g_signal_connect(G_OBJECT(new->rename_button), "clicked",
                   G_CALLBACK(rename_cb), new->entry);
  g_signal_connect(G_OBJECT(new), "delete-event", G_CALLBACK(editor_hide_cb),
                   new);

  GtkAccelGroup* accel = gtk_accel_group_new();

  gtk_widget_add_accelerator(new->save_button, "clicked", accel, GDK_KEY_S,
                             GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
  gtk_widget_add_accelerator(new->save_and_close_button, "clicked", accel,
                             GDK_KEY_w, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  gtk_window_add_accel_group(GTK_WINDOW(new), accel);

  new->history = slice_new(64);
  new->current_entry = NULL;

  return new;
}

void editor_set_entry_visible(NfEditor* editor, gboolean make_visible) {
  gtk_widget_set_visible(editor->entry, TRUE);
}

void editor_set_entry(NfEditor* editor, Entry* entry) {
  const char* name;
  char* content;
  GtkTextBuffer* buf =
      gtk_text_view_get_buffer(GTK_TEXT_VIEW(editor->textview));

  editor->current_entry = entry;

  gtk_widget_set_sensitive(editor->entry, (NULL == entry));
  gtk_widget_set_sensitive(editor->rename_button, (NULL != entry));

  if (entry) {
    name = (const char*)entry_get(entry, "name");
    slice_append(editor->history, entry);
    content = entry_get(entry, "content");
  } else {
    name = "Untitled";
    content = "";
  }

  editor->current_name = (const char*)name;

  gtk_text_buffer_set_text(buf, content, -1);
  gtk_widget_set_sensitive(editor->save_button, FALSE);
  gtk_widget_set_sensitive(editor->save_and_close_button, FALSE);

  gtk_header_bar_set_title(GTK_HEADER_BAR(editor->header),
                           editor->current_name);
  gtk_entry_set_text(GTK_ENTRY(editor->entry), editor->current_name);
}

gboolean editor_hide_cb(NfEditor* editor, gpointer unused) {
  (void)unused;

  gtk_widget_hide(GTK_WIDGET(editor));
  editor_set_entry(editor, NULL);

  return TRUE;
}

void edited_cb(gpointer unused, NfEditor* editor) {
  (void)unused;

  gtk_widget_set_sensitive(editor->save_button, TRUE);
  gtk_widget_set_sensitive(editor->save_and_close_button, TRUE);

  char* tmp_title = g_strdup_printf("*%s", editor->current_name);
  gtk_header_bar_set_title(GTK_HEADER_BAR(editor->header), tmp_title);
  g_free(tmp_title);
}

static void rename_cb(GtkWidget* rename_button, GtkWidget* entry) {
  gtk_widget_set_visible(entry, TRUE);
  gtk_widget_set_sensitive(entry, TRUE);
  gtk_widget_set_sensitive(rename_button, FALSE);
}

void save_entry_cb(gpointer unused, NfEditor* editor) {
  (void)unused;

  const char* current_name;
  char* current_text;
  GtkTextBuffer* buf;
  GtkTextIter start, end;

  buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(editor->textview));
  gtk_text_buffer_get_start_iter(buf, &start);
  gtk_text_buffer_get_end_iter(buf, &end);

  current_text = gtk_text_buffer_get_text(buf, &start, &end, FALSE);
  current_name = gtk_entry_get_text(GTK_ENTRY(editor->entry));

  if (NULL == editor->current_entry) {
    if (sources->len < 1) return;

    Source* default_source = sources->data[0];

    editor->current_entry = entry_new();
    entry_set(editor->current_entry, "name", strdup_or_die((char*)current_name),
              true);
    entry_set(editor->current_entry, "content", strdup_or_die(current_text),
              true);
    editor->current_entry->flags |= ENTRY_FLAGS_APP_NEW;

    slice_append(default_source->entries, editor->current_entry);
  } else {
    char* oldname = entry_get(editor->current_entry, "name");
    char* oldtext = entry_get(editor->current_entry, "content");

    if (0 != strcmp(oldname, current_name)) {
      char* newname = strdup_or_die((char*)current_name);
      entry_set(editor->current_entry, "name", newname, true);
      editor->current_name = newname;
      editor->current_entry->flags |= ENTRY_FLAGS_APP_UPDATED;

      free_and_null(oldname);
    }

    if (0 != strcmp(oldtext, current_text)) {
      char* newtext = strdup_or_die(current_text);
      entry_set(editor->current_entry, "content", newtext, true);
      editor->current_entry->flags |= ENTRY_FLAGS_APP_UPDATED;

      free_and_null(oldtext);
    }
  }

  gtk_widget_set_sensitive(editor->save_button, FALSE);
  gtk_widget_set_sensitive(editor->save_and_close_button, FALSE);
  gtk_header_bar_set_title(GTK_HEADER_BAR(editor->header),
                           editor->current_name);

  refresh();
}

void save_entry_and_close_cb(gpointer unused, gpointer editor) {
  (void)unused;

  save_entry_cb(NULL, NULL);
  gtk_widget_hide(GTK_WIDGET(editor));
}

#endif
