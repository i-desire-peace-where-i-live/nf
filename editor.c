/* Author:			Sergey Simonenko <gforgx@protonmail.com>
 * SPDX-License-Identifier:	0BSD */

#include <gtk/gtk.h>

#include "entry.h"
#include "source.h"
#include "util.h"
#include "editor.h"

#define ICON_RENAME "error-correct-symbolic"

extern VarsStruct* vars;

EditorState* editor_state = NULL;

extern GtkWidget* entries_list;
gboolean refresh(gpointer);

void set_editor_state(Entry* entry) {
	editor_state->current_entry = entry;

	const char* name;
	char* content;
	
	GtkTextBuffer* buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(editor_state->textview));

	gtk_widget_set_sensitive(editor_state->entry, (NULL == entry));
	gtk_widget_set_sensitive(editor_state->rename_button, (NULL != entry));

	if (entry) {
		name = (const char*)entry->name;
		slice_append(editor_state->history, entry);
		content = entry->content;
	} else {
		name = "Untitled";
		content = "";
	}

	editor_state->current_title = (const char*)name;

	gtk_text_buffer_set_text(buf, content, -1);
	gtk_widget_set_sensitive(editor_state->save_button, FALSE);
	gtk_widget_set_sensitive(editor_state->save_and_close_button, FALSE);

	gtk_header_bar_set_title(GTK_HEADER_BAR(editor_state->header), editor_state->current_title);
	gtk_entry_set_text(GTK_ENTRY(editor_state->entry), editor_state->current_title);
}

gboolean hide_editor_cb(GtkWidget* editor, gpointer unused) {
	(void)unused;

	gtk_widget_hide(editor);
	set_editor_state(NULL);

	return TRUE;
}

void edited_cb(gpointer unused, gpointer unused2) {
	(void)unused;
	(void)unused2;

	gtk_widget_set_sensitive(editor_state->save_button, TRUE);
	gtk_widget_set_sensitive(editor_state->save_and_close_button, TRUE);

	char* tmp_title = g_strdup_printf("*%s", editor_state->current_title);
	gtk_header_bar_set_title(GTK_HEADER_BAR(editor_state->header), tmp_title);
	g_free(tmp_title);
}

static void rename_cb(GtkWidget* rename_button, GtkWidget* entry) {
	gtk_widget_set_visible(entry, TRUE);
	gtk_widget_set_sensitive(entry, TRUE);
	gtk_widget_set_sensitive(rename_button, FALSE);
}

void save_entry_cb(gpointer unused, gpointer unused2) {
	(void)unused;
	(void)unused2;

	if (!editor_state->current_entry) {
		if (vars->sources->len < 1)
			return;

		Source* default_source = vars->sources->data[0];

		Entry* entry = entry_new();
		entry->name = strdup_or_die((char*)gtk_entry_get_text(GTK_ENTRY(editor_state->entry)));

		GtkTextBuffer* buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(editor_state->textview));

		GtkTextIter start;
		GtkTextIter end;

		gtk_text_buffer_get_start_iter(buf, &start);
		gtk_text_buffer_get_end_iter(buf, &end);

		entry->content = strdup_or_die(gtk_text_buffer_get_text(buf, &start, &end, FALSE));
		entry->flags |= ENTRY_FLAGS_APP_NEW;

		slice_append(default_source->entries, entry);

	} else {
		GtkTextBuffer* buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(editor_state->textview));

		GtkTextIter start;
		GtkTextIter end;

		gtk_text_buffer_get_start_iter(buf, &start);
		gtk_text_buffer_get_end_iter(buf, &end);

		const char* name = gtk_entry_get_text(GTK_ENTRY(editor_state->entry));

		if (name && (0 != strcmp(editor_state->current_entry->name, name))) {
			char* oldname = editor_state->current_entry->name;
			editor_state->current_entry->name = strdup_or_die((char*)name);
			editor_state->current_title = editor_state->current_entry->name;
			editor_state->current_entry->flags |= ENTRY_FLAGS_APP_EDITED;

			free(oldname);
		}

		char* text = gtk_text_buffer_get_text(buf, &start, &end, FALSE);

		if (text && (0 != strcmp(editor_state->current_entry->content, text))) {
			char* oldcontent = editor_state->current_entry->content;
			editor_state->current_entry->content = strdup_or_die(text);
			editor_state->current_entry->flags |= ENTRY_FLAGS_APP_EDITED;

			free(oldcontent);
		}
	}

	gtk_widget_set_sensitive(editor_state->save_button, FALSE);
	gtk_widget_set_sensitive(editor_state->save_and_close_button, FALSE);
	gtk_header_bar_set_title(GTK_HEADER_BAR(editor_state->header), editor_state->current_title);

	printf("Refreshing!\n");
	refresh(entries_list);
}

void save_entry_and_close_cb(gpointer unused, gpointer editor) {
	(void)unused;

	save_entry_cb(NULL, NULL);
	gtk_widget_hide(GTK_WIDGET(editor));
}

EditorState* init_editor_state(GtkWidget* header, GtkWidget* entry, GtkWidget* textview,
		GtkWidget* save_button, GtkWidget* save_and_close_button,
		GtkWidget* rename_button) {
	EditorState* state = malloc_or_die(sizeof(EditorState));

	state->current_title = NULL;
	state->current_entry = NULL;
	state->history = slice_new(1);

	state->header = header;
	state->entry = entry;
	state->textview = textview;
	state->save_button = save_button;
	state->save_and_close_button = save_and_close_button;
	state->rename_button = rename_button;

	return state;
}

GtkWidget* make_editor_window(void) {
	GtkWidget* editor = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size(GTK_WINDOW(editor), 512, 384);

	GtkWidget* header = gtk_header_bar_new();
	gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(header), true);
	gtk_header_bar_set_title(GTK_HEADER_BAR(header), "Untitled");
	gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(header), true);
	
	GtkWidget* save_and_close_button = gtk_button_new_with_mnemonic("Save and _close");
	gtk_header_bar_pack_end(GTK_HEADER_BAR(header), save_and_close_button);

	GtkWidget* save_button = gtk_button_new_with_mnemonic("_Save");
	gtk_header_bar_pack_end(GTK_HEADER_BAR(header), save_button);

	GtkWidget* rename_button = gtk_button_new_from_icon_name(ICON_RENAME,
			GTK_ICON_SIZE_BUTTON);
	gtk_header_bar_pack_end(GTK_HEADER_BAR(header), rename_button);

	gtk_window_set_titlebar(GTK_WINDOW(editor), header);

	GtkWidget* vpaned = gtk_paned_new(GTK_ORIENTATION_VERTICAL);

	GtkWidget* entry = gtk_entry_new();
	gtk_paned_add1(GTK_PANED(vpaned), entry);

	GtkWidget* scroller = gtk_scrolled_window_new(NULL, NULL);

	GtkWidget* textview = gtk_text_view_new();
	GtkTextBuffer* buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));

	g_signal_connect(GTK_EDITABLE(entry), "changed", G_CALLBACK(edited_cb), NULL);
	g_signal_connect(G_OBJECT(buf), "changed", G_CALLBACK(edited_cb), NULL);

	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(textview), GTK_WRAP_WORD);
#define TEXTVIEW_MARGIN 8
	gtk_text_view_set_left_margin(GTK_TEXT_VIEW(textview), TEXTVIEW_MARGIN);
	gtk_text_view_set_right_margin(GTK_TEXT_VIEW(textview), TEXTVIEW_MARGIN);
	gtk_text_view_set_top_margin(GTK_TEXT_VIEW(textview), TEXTVIEW_MARGIN);
	gtk_text_view_set_bottom_margin(GTK_TEXT_VIEW(textview), TEXTVIEW_MARGIN);
#undef TEXTVIEW_MARGIN

	gtk_container_add(GTK_CONTAINER(scroller), textview);
	gtk_paned_add2(GTK_PANED(vpaned), scroller);

	editor_state = init_editor_state(header, entry, textview, save_button,
			save_and_close_button, rename_button);
	set_editor_state(NULL);

	gtk_container_add(GTK_CONTAINER(editor), vpaned);

	g_signal_connect(G_OBJECT(save_button), "clicked",
			G_CALLBACK(save_entry_cb), editor);
	g_signal_connect(G_OBJECT(save_and_close_button), "clicked",
			G_CALLBACK(save_entry_and_close_cb), editor);
	g_signal_connect(G_OBJECT(rename_button), "clicked",
			G_CALLBACK(rename_cb), entry);
	g_signal_connect(G_OBJECT(editor), "delete-event",
			G_CALLBACK(hide_editor_cb), editor);

	GtkAccelGroup* accel = gtk_accel_group_new();

	gtk_widget_add_accelerator(save_button, "clicked", accel,
			GDK_KEY_S, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	gtk_widget_add_accelerator(save_and_close_button, "clicked", accel,
			GDK_KEY_w, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

	gtk_window_add_accel_group(GTK_WINDOW(editor), accel);

	return editor;
}

