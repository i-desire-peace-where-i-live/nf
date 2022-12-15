/* Author:			Sergey Simonenko <gforgx@protonmail.com>
 * SPDX-License-Identifier:	0BSD */

#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>

#include "common.h"
#include "slice.h"
#include "source.h"
#include "util.h"
#include "dir.h"
#include "mozilla.h"
#include "syncer.h"

extern VarsStruct* vars;

void* syncer_main(void* unused) {
	(void)unused;

	for (;;) {
		slice_lock(vars->sources);

		for (int i = 0; i < vars->sources->len; i++) {
			Source* src = vars->sources->data[i];
			debugf("Source type is: %d, parameter is: %s", src->type,
					src->param);

			switch (src->type) {
				case SOURCE_DIR:
					sync_dir(src->param, src->entries);
					break;
				case SOURCE_MOZILLA:
					sync_mozilla(src->param, src->entries);
					break;
				default:
					continue;
			}
		}

		slice_unlock(vars->sources);

		if (vars->argc > 1)
			return 0;

		sleep(FETCHER_SLEEP_DURATION);
	}
}

