/* Author:			Sergey Simonenko <gforgx@protonmail.com>
 * SPDX-License-Identifier:	0BSD */

#ifndef NOTEFINDER_EDITOR_H
#define NOTEFINDER_EDITOR_H

#include <gtk/gtk.h>

#include "slice.h"
#include "entry.h"

typedef struct {
	const char* current_title;
	Entry* current_entry;
	Slice* history;

	GtkWidget* header;
	GtkWidget* entry;
	GtkWidget* textview;
	GtkWidget* save_button;
	GtkWidget* save_and_close_button;
	GtkWidget* rename_button;
} EditorState;

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

