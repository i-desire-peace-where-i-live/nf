/* Author:			Sergey Simonenko <gforgx@protonmail.com>
 * SPDX-License-Identifier:	0BSD */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE  // for strcasestr()
#endif

#include <limits.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <unistd.h>

#ifdef HAVE_PERL
#include <EXTERN.h>
#include <perl.h>
#endif

#include "common.h"
#include "entry.h"
#include "search.h"
#include "server.h"
#include "slice.h"
#include "source.h"
#include "util.h"

extern Slice* sources;

static char* get_name(void* p) {
  Entry* entry = p;
  return entry_get(entry, "name");
}

static void do_cmd_version(void) {
  printf("%s %s\n", program_name, program_ver);

  exit(EXIT_SUCCESS);
}

static void do_cmd_stats(void) {
  printf("Data sources configured: %zu\n", sources->len);

  int count = 0;
  for (size_t i = 0; i < sources->len; i++)
    count += ((Source*)sources->data[i])->entries->len;

  printf("Total entries count: %d\n", count);

  exit(EXIT_SUCCESS);
}

int exec_cmds(int argc, char** argv) {
  if (argc < 2 || !argv[1]) panic("not enough arguments!");

  debugf("Command is: %s\n", argv[1]);

  if (0 == strcmp("stats", argv[1]))
    do_cmd_stats();
  else if (0 == strcmp("version", argv[1]))
    do_cmd_version();

  char* query = argv[1];

  Slice* results = get_search_results(query);
  slice_print(results, stdout, get_name);

  slice_free(results, NULL);

  return 0;
}
