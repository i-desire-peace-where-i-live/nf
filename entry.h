/* Author:			Sergey Simonenko <gforgx@protonmail.com>
 * SPDX-License-Identifier:	0BSD */

#ifndef NOTEFINDER_ENTRY_H
#define NOTEFINDER_ENTRY_H

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#include "map.h"
#include "slice.h"

typedef enum {
  ENTRY_NOTE,
  ENTRY_BOOKMARK,
  ENTRY_FILE,
} EntryType;

typedef struct {
  uint64_t uuid;

  EntryType type;

  struct tm* created_time;
  struct tm* modified_time;

#define ENTRY_FLAGS_APP_NEW 0x10000000
#define ENTRY_FLAGS_APP_UPDATED 0x01000000
#define ENTRY_FLAGS_APP_DELETED 0x00100000
// 0x00010000 is reserved
#define ENTRY_FLAGS_SRC_NEW 0x00001000
#define ENTRY_FLAGS_SRC_UPDATED 0x00000100
  // 0x00000010
  // 0x00000001 are reserved
  unsigned int flags;

  Map* props;
  Slice* resources;
} Entry;

void* entry_get(Entry*, const char*);
void entry_set(Entry*, const char*, void*, bool);
void entry_set_default(Entry*, const char*, void*, bool);

Entry* entry_new(void);
void entry_free(void*);

#endif
