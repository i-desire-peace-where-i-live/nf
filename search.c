/* Author:			Sergey Simonenko <gforgx@protonmail.com>
 * SPDX-License-Identifier:	0BSD */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE  // for strcasestr()
#endif

#include <string.h>

#include "entry.h"
#include "slice.h"
#include "source.h"
#include "util.h"

extern Slice* sources;

Slice* get_search_results(char* query) {
  slice_lock(sources);

  Slice* results;
  results = slice_new(0);

  for (size_t i = 0; i < sources->len; i++) {
    Source* src = sources->data[i];

    for (int j = 0; j < src->entries->len; j++) {
      Entry* entry = src->entries->data[j];
      if (!query || strcasestr(entry_get(entry, "name"), query)) {
        slice_append(results, entry);
      }
    }
  }

  slice_unlock(sources);
  return results;
}
