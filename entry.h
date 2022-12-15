/* Author:			Sergey Simonenko <gforgx@protonmail.com>
 * SPDX-License-Identifier:	0BSD */

#ifndef NOTEFINDER_ENTRY_H
#define NOTEFINDER_ENTRY_H

#include <stdint.h>
#include <stdbool.h>

#include "slice.h"

typedef enum {
	ENTRY_NOTE,
	ENTRY_BOOKMARK,
} EntryType;

typedef struct {
	uint64_t uuid;

	char* name;

	char *url;
	char *filepath;
	char *content;

#define ENTRY_FLAGS_APP_NEW	0x10000000
#define ENTRY_FLAGS_APP_EDITED	0x01000000
#define ENTRY_FLAGS_APP_DELETED	0x00100000
// 0x00010000 is reserved
#define ENTRY_FLAGS_SRC_NEW	0x00001000
// 0x00000100
// 0x00000010
// 0x00000001 are reserved
	unsigned int flags;
} Entry;

Entry* entry_new(void);
void entry_free(void*);

#endif


