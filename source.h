/* Author:			Sergey Simonenko <gforgx@protonmail.com>
 * SPDX-License-Identifier:	0BSD */

#ifndef NOTEFINDER_SOURCE_H
#define NOTEFINDER_SOURCE_H

#include <stdint.h>

#include "slice.h"

enum SourceStatus {
	STATUS_ERR,
	STATUS_OFFLINE,
	STATUS_ONLINE,
};

typedef enum {
	SOURCE_ERR,
	SOURCE_DIR,
	SOURCE_MOZILLA,
} SourceType;

typedef struct {
	uint64_t uuid;

	SourceType type;
	char* param;

	Slice* entries;
} Source;

Source* src_new(void);
void src_free(void*);

#endif

