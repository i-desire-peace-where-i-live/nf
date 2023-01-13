/* Author:			Sergey Simonenko <gforgx@protonmail.com>
 * SPDX-License-Identifier:	0BSD */

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "../util.h"

int LLVMFuzzerTestOneInput(uint8_t* buf, size_t sz) {
	if (0 == sz)
		return 0;

	char str[sz + 1];
	memcpy(str, buf, sz);
	str[sz] = 0;

	srand(time(0));
	int lim = rand() % sz;

	debugf("Got %s from libfuzzer", str);

	free(get_first_n_chars(str, lim, NULL));

	return 0;
}

