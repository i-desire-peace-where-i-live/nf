/* Author:			Sergey Simonenko <gforgx@protonmail.com>
 * SPDX-License-Identifier:	0BSD */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/param.h>
#include <sys/stat.h>

#include "slice.h"
#include "source.h"
#include "entry.h"
#include "util.h"
#include "dir.h"

CMP_STRCMPWRAP(compare_entry, Entry, name)

static void update_entry_from_file(Entry* entry, char* path, char* localname) {
	if (!entry->filepath)
		entry->filepath = strdup_or_die(path);

	if (!entry->name)
		entry->name = strdup_or_die(localname);

	char* content = NULL;
	FILE* fp = fopen(path, "rb");
	long sz;

	if (!fp) {
		debugf("fopen() failed for \"%s\"", path);
		return;
	}

	fseek(fp, 0, SEEK_END);
	sz = ftell(fp);
	rewind(fp);

	// FIXME: we need to take care of the old content somehow, but not now...
//	free(entry->content);
//	entry->content = NULL;

	content = malloc_or_die(sz + 1);

	fread(content, 1, sz, fp);
	content[sz] = 0;
	fclose(fp);

	// To reach NUL terminator we have to read sz + 1 bytes, so this is fine:
	if (!(memchr(content, 0, sz))) {
		// FIXME: crash happens when I overwrite this
		if (!entry->content)
			entry->content = content;
	} else {
		free(content);
		debugf("%s appears binary", localname);
	}
}

int sync_dir(char* param, Slice* entries) {
	debugf("Importing from %s", param);
	for (int i = 0; i < entries->len; i++) {
		Entry* entry = entries->data[i];

		if (ENTRY_FLAGS_APP_NEW == (entry->flags & ENTRY_FLAGS_APP_NEW)) {
			char path[PATH_MAX];

			strlcpy(path, param, PATH_MAX);
			strlcat(path, "/", PATH_MAX);
			strlcat(path, entry->name, PATH_MAX);

			entry->filepath = strdup_or_die(path);

			entry->flags &= ~(ENTRY_FLAGS_APP_NEW);

		} else if (ENTRY_FLAGS_APP_EDITED != (entry->flags & ENTRY_FLAGS_APP_EDITED))
			continue;

		if (!entry->filepath || !entry->content)
			continue;

		FILE* fp = fopen(entry->filepath, "w");
		fputs(entry->content, fp);
		fclose(fp);

		entry->flags &= ~(ENTRY_FLAGS_APP_EDITED);
	}

	debugf("Exporting to %s", param);
	struct dirent* de;
	DIR* dir;

	if (!(dir = opendir(param))) {
		debugf("\"%s\" doesn't exist!", param);
		return STATUS_ERR;
	}

	while ((de = readdir(dir))) {
		if (0 == strcmp(de->d_name, ".") || 0 == strcmp(de->d_name, ".."))
			continue;

		char path[PATH_MAX];

		strlcpy(path, param, PATH_MAX);
		strlcat(path, "/", PATH_MAX);
		strlcat(path, de->d_name, PATH_MAX);

		debugf("found file named: \"%s\"", path);

		struct stat st;
		if (0 != stat(path, &st) || 0 == S_ISREG(st.st_mode)) {
			debugf("stat() failed for \"%s\"; most likely it's not a regular file", path);
			continue;
		}

		MKSEARCHKEY(Entry, name, de->d_name, p)
		Entry* entry;

		if (!(entry = (bsearch(&p, entries->data, entries->len, sizeof(char*), compare_entry)))) {
			entry = entry_new();
			entry->uuid = (uint64_t)st.st_ino;
			slice_append(entries, entry);
		} else {
			debugf("\"%s\" already exists in the cache, updating", path);
		}

		update_entry_from_file(entry, path, de->d_name);
		qsort(entries->data, entries->len, sizeof(char*), compare_entry);
	}

	closedir(dir);

	return STATUS_ONLINE;
}

