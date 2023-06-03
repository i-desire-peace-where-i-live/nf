/* Author:			Sergey Simonenko <gforgx@protonmail.com>
 * SPDX-License-Identifier:	0BSD */

#ifndef NOTEFINDER_UTIL_H
#define NOTEFINDER_UTIL_H

#include <stdbool.h>

#include "slice.h"

#define MINIMUM(a, b) (((a) < (b)) ? (a) : (b))
#define MAXIMUM(a, b) (((a) > (b)) ? (a) : (b))

#define CMP_STRCMPWRAP(name, type, memb)          \
  static int name(const void* a, const void* b) { \
    const type* cmp = *(type**)a;                 \
    const type* arr_memb = *(type**)b;            \
    return strcmp(cmp->memb, arr_memb->memb);     \
  }

#define MKSEARCHKEY(type, memb, val, keypname) \
  type key;                                    \
  key.memb = val;                              \
  type* keypname = &key;

void* malloc_or_die(size_t);
void* realloc_or_die(void*, size_t);
void free_and_null(void*);

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
