/* Author:			Sergey Simonenko <gforgx@protonmail.com>
 * SPDX-License-Identifier:	0BSD */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

#include "slice.h"
#include "util.h"

extern VarsStruct* vars;

/* strlcpy and strlcat are taken from OpenBSD libc to not rely on libbsd existence
 * and (possible) platform-specific behaviour */
size_t strlcpy(char* dst, const char* src, size_t dsize) {
	const char* osrc = src;
	size_t nleft = dsize;

	/* Copy as many bytes as will fit. */
	if (nleft != 0) {
		while (--nleft != 0) {
			if ((*dst++ = *src++) == '\0')
				break;
		}
	}

	/* Not enough room in dst, add NUL and traverse rest of src. */
	if (nleft == 0) {
		if (dsize != 0)
			*dst = '\0';		/* NUL-terminate dst */
		while (*src++)
			;
	}

	return(src - osrc - 1);	/* count does not include NUL */
}

size_t strlcat(char* dst, const char* src, size_t dsize) {
	const char* odst = dst;
	const char* osrc = src;
	size_t n = dsize;
	size_t dlen;

	/* Find the end of dst and adjust bytes left but don't go past end. */
	while (n-- != 0 && *dst != '\0')
		dst++;
	dlen = dst - odst;
	n = dsize - dlen;

	if (n-- == 0)
		return(dlen + strlen(src));
	while (*src != '\0') {
		if (n != 0) {
			*dst++ = *src;
			n--;
		}
		src++;
	}
	*dst = '\0';

	return(dlen + (src - osrc));	/* count does not include NUL */
}

void debugf(const char* msg, ...) {
	if (!vars->debug_yes)
		return;

	va_list args;
	
	fprintf(stderr, "Debug: ");
	va_start(args, msg);
	vfprintf(stderr, msg, args);
	va_end(args);
	fprintf(stderr, "\n");
}

void debug(const char* msg) {
	debugf(msg);
}

void panicf(const char* msg, ...) {
	va_list args;

	fprintf(stderr, "Panic: ");
	va_start(args, msg);
	vfprintf(stderr, msg, args);
	va_end(args);
	fprintf(stderr, "\n");

	exit(EXIT_FAILURE);
}

void panic(const char* msg) {
	panicf(msg);
}

void* malloc_or_die(size_t sz) {
	void* p;

	if (!(p = malloc(sz)))
		panicf("out of memory, requested %zu bytes", sz);

	return p;
}

void* realloc_or_die(void* p, size_t sz) {
	if (!(p = realloc(p, sz)))
		panicf("out of memory, requested %zu bytes", sz);

	return p;
}

char* strdup_or_die(char* src) {
	char* str;

	if (!(str = strdup(src)))
		panic("strdup() failed; probably out of memory");

	return str;
}

char* get_first_n_chars(char* src, int limit, char* suffix) {
#define UNICODE_START_MASK 0xc0
#define IS_UNICODE_START(x) (UNICODE_START_MASK & x) == UNICODE_START_MASK
#define IS_ASCII(x) 0 <= x && x <= 127
	int c = 0;
	size_t offset;

	for (offset = 0; offset < strlen(src); offset++) {
		if (IS_UNICODE_START(src[offset]) || src[offset])
			c++;
		if (c > limit)
			break;
	}
#undef UNICODE_START_MASK
#undef IS_UNICODE_START
#undef IS_ASCII

	size_t sz = offset + (suffix ? strlen(suffix) : 0);
	char* tmp = malloc_or_die(sz + 1);
	strlcpy(tmp, src, sz + 1);
	if (suffix)
		strlcat(tmp, suffix, sz + 1);

	return tmp;
}

char* get_home_dir(void) {
	uid_t uid = getuid();
	struct passwd* pw = getpwuid(uid);

	if (!pw || !pw->pw_dir)
		panic("getpwuid() returned NULL");

	return pw->pw_dir;
}

