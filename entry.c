/* Author:			Sergey Simonenko <gforgx@protonmail.com>
 * SPDX-License-Identifier:	0BSD */

#include "entry.h"

#include <stdlib.h>
#include <string.h>

#include "map.h"
#include "util.h"

Entry* entry_new(void) {
  Entry* entry = malloc_or_die(sizeof(Entry));

  entry->flags = 0;
  entry->created_time = NULL;
  entry->modified_time = NULL;
  entry->props = map_new(16);
  entry->resources = slice_new(16);

  return entry;
}

void entry_free(void* p) {
  Entry* entry = p;

  map_free(entry->props);

  for (int i = 0; i < entry->resources->len; i++)
    free(entry->resources->data[i]);

  free(entry);
}

void* entry_get(Entry* entry, const char* prop) {
  return map_get(entry->props, prop);
}

void entry_set(Entry* entry, const char* prop, void* value, bool must_free) {
  map_put(entry->props, prop, value);

  if (must_free) slice_append(entry->resources, value);
}

void entry_set_default(Entry* entry, const char* prop, void* value,
                       bool must_free) {
  void* current = map_get(entry->props, prop);

  if (!current) entry_set(entry, prop, value, must_free);
}
