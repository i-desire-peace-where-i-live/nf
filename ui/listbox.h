/* Author:			Sergey Simonenko <gforgx@protonmail.com>
 * SPDX-License-Identifier:	0BSD */

#ifndef NOTEFINDER_LISTBOX_H
#define NOTEFINDER_LISTBOX_H

#include <gtk/gtk.h>

#include "../entry.h"

G_BEGIN_DECLS

#define NF_TYPE_LISTBOX (nf_listbox_get_type())
#define NF_TYPE_LISTBOX_ROW (nf_listboxrow_get_type())

G_DECLARE_FINAL_TYPE(NfListBox, nf_listbox, NF, LISTBOX, GtkListBox)
G_DECLARE_FINAL_TYPE(NfListBoxRow, nf_listboxrow, NF, LISTBOX_ROW, GtkBox)

GtkWidget* listbox_new(void);
void listbox_clear(NfListBox*);
void listbox_add_entry(NfListBox*, Entry*);
void listboxrow_update_entry_flags(NfListBoxRow*, int);
Entry* listboxrow_get_entry(NfListBoxRow*);

G_END_DECLS

#endif
