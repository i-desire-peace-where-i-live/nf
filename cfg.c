/* Author:			Sergey Simonenko <gforgx@protonmail.com>
 * SPDX-License-Identifier:	0BSD */

#include <limits.h>
#include <pthread.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <unistd.h>

#include "slice.h"
#include "source.h"
#include "util.h"

#define CONFIG_FILE ".config/notefinder"

Slice* get_sources_from_config(char* config_path) {
  FILE* fp;
  fp = fopen(config_path, "r");
  if (!fp) panic("couldn't open the configuration file");

  char* config_entry = NULL;
  size_t sz;

  Slice* sources = slice_new(4);

  while (-1 != getline(&config_entry, &sz, fp)) {
    char* p;
    if ((p = strchr(config_entry, '\n'))) *p = '\0';

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
