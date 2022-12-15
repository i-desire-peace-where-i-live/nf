/* Author:			Sergey Simonenko <gforgx@protonmail.com>
 * SPDX-License-Identifier:	0BSD */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE	// for strcasestr()
#endif

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <pwd.h>
#include <pthread.h>
#include <sys/param.h>

#include <EXTERN.h>
#include <perl.h>

#include "common.h"
#include "slice.h"
#include "source.h"
#include "entry.h"
#include "util.h"
#include "syncer.h"

#define CONFIG_FILE ".config/notefinder"

#define UI (1 == argc)

VarsStruct* vars = NULL;

Slice* get_search_results(char* query) {
	Slice* results;
	results = slice_new(0);

	slice_lock(vars->sources);

	for (int i = 0; i < vars->sources->len; i++) {
		Source* src = vars->sources->data[i];

		for (int j = 0; j < src->entries->len; j++) {
			Entry* entry = src->entries->data[j];
			if (!query || strcasestr(entry->name, query)) {
				slice_append(results, entry);
			}
		}
	}

	slice_unlock(vars->sources);

	return results;
}

int show_window(int, char**);
int exec_cmds(int, char**);

pthread_t run_syncer(void) {
	pthread_t tid;

	pthread_create(&tid, NULL, syncer_main, NULL);
	return tid;
}

Slice* get_sources_from_config(char* config_path) {
	FILE* fp;
	fp = fopen(config_path, "r");
	if (!fp)
		panic("couldn't open the configuration file");

	char* config_entry = NULL;
	size_t sz;

	Slice* sources = slice_new(4);

	while (-1 != getline(&config_entry, &sz, fp)) {
		char* p;
		if ((p = strchr(config_entry, '\n')))
			*p = '\0';

		char* type = strtok(config_entry, ":");
		char* param = strtok(NULL, ":");

		if (!type || *type == '#' || !param) {
			continue;
		}

		Source* source = src_new();

		if (0 == strcmp(type, "dir"))
			source->type = SOURCE_DIR;
		else if (0 == strcmp(type, "mozilla"))
			source->type = SOURCE_MOZILLA;
		else
			source->type = SOURCE_ERR;

		source->param = strdup_or_die(param);

		slice_append(sources, source);
	}

	free(config_entry);

	fclose(fp);

	return sources;
}

VarsStruct* init_vars(int argc, char** argv) {
	VarsStruct* ret = malloc_or_die(sizeof(VarsStruct));
	memset(ret, 0, sizeof(VarsStruct));

	ret->argc = argc;
	ret->argv = argv;

	if (getenv("DEBUG"))
		ret->debug_yes = 1;
	else
		ret->debug_yes = 0;

	/* Parse the configuration file: */
	char config_path[PATH_MAX];
	size_t n;
	n = strlcpy(config_path, get_home_dir(), sizeof(config_path));
	strlcpy(config_path + n, "/" CONFIG_FILE, sizeof(config_path) - n);

	ret->sources = get_sources_from_config(config_path);
	
	ret->last_query = NULL;

	return ret;
}

int main(int argc, char** argv) {
	/* All global variables MUST be initialized here within VarsStruct: */
	vars = init_vars(argc, argv);

	PerlInterpreter* perl;
	perl = perl_alloc();
	perl_construct(perl);

#if 0
	char* perl_args[] = {"", "/home/gforgx/test.pl"};
	printf("Parsing perl...\n======\n");
	perl_parse(perl, NULL, 2, perl_args, NULL);
	perl_run(perl);
	printf("\n======\nDone parsing perl...\n");
#endif

	pthread_t tid = run_syncer();

	if (UI)
		show_window(argc, argv);
	else {
		pthread_join(tid, NULL);
		exec_cmds(argc, argv);
	}
clean:
	/* Final cleanup */
	perl_destruct(perl);
	perl_free(perl);

	slice_free(vars->sources, src_free);
	free(vars);

	exit(EXIT_SUCCESS);
}

