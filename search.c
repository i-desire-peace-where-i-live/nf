/* Author:			Sergey Simonenko <gforgx@protonmail.com>
 * SPDX-License-Identifier:	0BSD */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE	// for strcasestr()
#endif

#include <string.h>

#include "slice.h"
#include "source.h"
#include "entry.h"
#include "util.h"

extern VarsStruct* vars;

Slice* get_search_results(char* query) {
	Slice* results;
	results = slice_new(0);

	for (int i = 0; i < vars->sources->len; i++) {
		Source* src = vars->sources->data[i];

		for (int j = 0; j < src->entries->len; j++) {
			Entry* entry = src->entries->data[j];
			if (!query || strcasestr(entry->name, query)) {
				slice_append(results, entry);
			}
		}
	}

	return results;
}

