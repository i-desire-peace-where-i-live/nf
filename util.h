/* Author:			Sergey Simonenko <gforgx@protonmail.com>
 * SPDX-License-Identifier:	0BSD */

#ifndef NOTEFINDER_UTIL_H
#define NOTEFINDER_UTIL_H

#define MINIMUM(a, b) (((a) < (b)) ? (a) : (b))
#define MAXIMUM(a, b) (((a) > (b)) ? (a) : (b))

#define CMP_STRCMPWRAP(name, type, memb) \
static int name (const void* a, const void* b) { \
	const type* cmp = *(type**)a; \
	const type* arr_memb = *(type**)b; \
	return strcmp(cmp->memb, arr_memb->memb); \
}

#define MKSEARCHKEY(type, memb, val, keypname) \
	type key; \
	key.memb = val; \
	type* keypname = &key;

typedef struct {
	int argc;
	char** argv;

	int debug_yes;

	Slice* sources;
	
	char* last_query;
} VarsStruct;

void* malloc_or_die(size_t);
void* realloc_or_die(void*, size_t);

char* strdup_or_die(char*);
size_t strlcpy(char*, const char*, size_t);
size_t strlcat(char*, const char*, size_t);
char* get_first_n_chars(char*, int, char*);

void debugf(const char*, ...);
void debug(const char*);
void panicf(const char*, ...);
void panic(const char*);

char* get_home_dir(void);

#endif

