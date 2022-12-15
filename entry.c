/* Author:			Sergey Simonenko <gforgx@protonmail.com>
 * SPDX-License-Identifier:	0BSD */

#include <stdlib.h>
#include <string.h>

#include "entry.h"
#include "util.h"

Entry* entry_new(void) {
	Entry* entry = malloc_or_die(sizeof(Entry));
	memset(entry, 0, sizeof(Entry));

	entry->name = NULL;
	entry->url = NULL;
	entry->filepath = NULL;
	entry->content = NULL;

	return entry;
}

void entry_free(void* v) {
	Entry* entry = v;

	free(entry->name);
	free(entry->url);
	free(entry->filepath);
	free(entry->content);

	free(entry);
}

