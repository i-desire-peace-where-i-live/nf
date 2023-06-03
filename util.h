/* Author:			Sergey Simonenko <gforgx@protonmail.com>
 * SPDX-License-Identifier:	0BSD */

#ifndef NOTEFINDER_UTIL_H
#define NOTEFINDER_UTIL_H

#include <stdbool.h>

#include "slice.h"

#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

void* malloc_or_die(size_t);
void* realloc_or_die(void*, size_t);

#define free_and_null(p) \
  do {                   \
    if (p) {             \
      free(p);           \
      p = NULL;          \
    }                    \
  } while (0)

char* strdup_or_die(char*);
#ifndef __APPLE__
size_t strlcpy(char*, const char*, size_t);
size_t strlcat(char*, const char*, size_t);
#endif
char* get_first_n_chars(char*, int, char*);

void debugf(const char*, ...);
void debug(const char*);
void panicf(const char*, ...);
void panic(const char*);

char* get_home_dir(void);
char* read_file(char*, bool, bool*);
char* join_path(char*, ...);

#endif
