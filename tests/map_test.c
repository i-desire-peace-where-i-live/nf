/* Author:			Sergey Simonenko <gforgx@protonmail.com>
 * SPDX-License-Identifier:	0BSD */

#include <stdint.h>
#include <string.h>

#include "../map.h"
#include "../util.h"

static Map* map = NULL;

int LLVMFuzzerTestOneInput(uint8_t* buf, size_t sz) {
	char str[sz + 1];
	memcpy(str, buf, sz);
	str[sz] = 0;

	debugf("Got %s from libfuzzer, current len is: %d", str, map->len);

	map_put(map, str, str);

	if (str != map_get(map, str))
		return -1;

	return 0;
}

int LLVMFuzzerInitialize(void) {
	map = map_init(1024);

	return 0;
}

