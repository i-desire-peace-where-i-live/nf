/* Author:			Sergey Simonenko <gforgx@protonmail.com>
 * SPDX-License-Identifier:	0BSD */

#ifndef NOTEFINDER_EDITOR_H
#define NOTEFINDER_EDITOR_H

#include <gtk/gtk.h>

#include "../entry.h"
#include "../slice.h"

G_BEGIN_DECLS

#define NF_TYPE_EDITOR (nf_editor_get_type())

G_DECLARE_FINAL_TYPE(NfEditor, nf_editor, NF, EDITOR, GtkWindow)

NfEditor* editor_new(GtkWindow*);
void editor_set_entry(NfEditor*, Entry*);
void editor_set_entry_visible(NfEditor*, gboolean);

G_END_DECLS

#if 0
void set_editor_state(Entry*);
gboolean hide_editor_cb(GtkWidget*, gpointer);
void edited_cb(gpointer, gpointer);
void save_entry_cb(gpointer, gpointer);
void save_entry_and_close_cb(gpointer, gpointer);
EditorState* init_editor_state(GtkWidget* header, GtkWidget* entry,
		GtkWidget* textview, GtkWidget* save_button,
		GtkWidget* save_and_close_button, GtkWidget* rename_button);
GtkWidget* make_editor_window(void);
#endif
#endif
