/* Author:			Sergey Simonenko <gforgx@protonmail.com>
 * SPDX-License-Identifier:	0BSD */

#include "source.h"

#include <stdlib.h>
#include <string.h>

#include "entry.h"
#include "util.h"

Source* src_new(void) {
  Source* src = malloc_or_die(sizeof(Source));
  memset(src, 0, sizeof(Source));

  src->param = NULL;
  src->entries = slice_new(0);

  return src;
}

void src_free(void* p) {
  Source* src = p;
  free(src->param);
  slice_free(src->entries, entry_free);

  free(src);
}
