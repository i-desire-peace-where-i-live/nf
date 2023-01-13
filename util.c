/* Author:			Sergey Simonenko <gforgx@protonmail.com>
 * SPDX-License-Identifier:	0BSD */

#include "util.h"

#include <limits.h>
#include <pwd.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "slice.h"

int enable_debug = 0;

#ifndef __APPLE__
/* strlcpy and strlcat are taken from OpenBSD libc to not rely on libbsd
 * existence and (possible) platform-specific behaviour */
size_t strlcpy(char* dst, const char* src, size_t dsize) {
  const char* osrc = src;
  size_t nleft = dsize;

  /* Copy as many bytes as will fit. */
  if (nleft != 0) {
    while (--nleft != 0) {
      if ((*dst++ = *src++) == '\0') break;
    }
  }

  /* Not enough room in dst, add NUL and traverse rest of src. */
  if (nleft == 0) {
    if (dsize != 0) *dst = '\0'; /* NUL-terminate dst */
    while (*src++)
      ;
  }

  return (src - osrc - 1); /* count does not include NUL */
}

size_t strlcat(char* dst, const char* src, size_t dsize) {
  const char* odst = dst;
  const char* osrc = src;
  size_t n = dsize;
  size_t dlen;

  /* Find the end of dst and adjust bytes left but don't go past end. */
  while (n-- != 0 && *dst != '\0') dst++;
  dlen = dst - odst;
  n = dsize - dlen;

  if (n-- == 0) return (dlen + strlen(src));
  while (*src != '\0') {
    if (n != 0) {
      *dst++ = *src;
      n--;
    }
    src++;
  }
  *dst = '\0';

  return (dlen + (src - osrc)); /* count does not include NUL */
}
#endif  // !__APPLE__

void debugf(const char* msg, ...) {
  if (!enable_debug) return;

  va_list args;

  fprintf(stderr, "Debug: ");
  va_start(args, msg);
  vfprintf(stderr, msg, args);
  va_end(args);
  fprintf(stderr, "\n");
}

void debug(const char* msg) { debugf(msg); }

void panicf(const char* msg, ...) {
  va_list args;

  fprintf(stderr, "Panic: ");
  va_start(args, msg);
  vfprintf(stderr, msg, args);
  va_end(args);
  fprintf(stderr, "\n");

  exit(EXIT_FAILURE);
}

void panic(const char* msg) { panicf(msg); }

void* malloc_or_die(size_t sz) {
  void* p;

  if (!(p = malloc(sz))) panicf("out of memory, requested %zu bytes", sz);

  return p;
}

void* realloc_or_die(void* p, size_t sz) {
  if (!(p = realloc(p, sz))) panicf("out of memory, requested %zu bytes", sz);

  return p;
}

char* strdup_or_die(char* src) {
  char* str;

  if (!(str = strdup(src))) panic("strdup() failed; probably out of memory");

  return str;
}

char* get_first_n_chars(char* src, int limit, char* suffix) {
#define UNICODE_START_MASK 0xc0
#define IS_UNICODE_START(x) ((UNICODE_START_MASK & x) == UNICODE_START_MASK)
#define IS_ASCII(x) (0 <= x && x <= 127)
  int c = 0;
  size_t offset;

  for (offset = 0; offset <= strlen(src) && c <= limit; offset++) {
    if (IS_ASCII(src[offset]) || IS_UNICODE_START(src[offset])) c++;
  }

#undef UNICODE_START_MASK
#undef IS_UNICODE_START
#undef IS_ASCII

  size_t sz = offset + 1 + (suffix ? strlen(suffix) : 0);
  char* tmp = malloc_or_die(sz);
  strlcpy(tmp, src, offset);
  if (suffix && offset < strlen(src) + 1) {
    strlcat(tmp, suffix, sz);
  }

  return tmp;
}

char* get_home_dir(void) {
  uid_t uid = getuid();
  struct passwd* pw = getpwuid(uid);

  if (!pw || !pw->pw_dir) panic("getpwuid() returned NULL");

  return pw->pw_dir;
}

char* read_file(char* path, bool skip_binary, bool* is_binary) {
  char* content = NULL;
  FILE* fp = fopen(path, "rb");
  long sz;

  if (!fp) {
    debugf("fopen() failed for \"%s\"", path);
    return NULL;
  }

  fseek(fp, 0, SEEK_END);
  sz = ftell(fp);
  rewind(fp);

  content = malloc_or_die(sz + 1);

  // TODO: add sz limit
  size_t read_sz = fread(content, 1, sz, fp);
  if (read_sz != sz) {
    debugf("fread() failed, size mismatch for \"%s\"", path);
    return NULL;
  }

  content[sz] = 0;
  fclose(fp);

  // To reach NUL terminator we have to read sz + 1 bytes, so this is
  // fine.
  if (memchr(content, 0, sz) && skip_binary) {
    //    debugf("%s appears binary", path);
    free(content);
    *is_binary = true;
    return NULL;
  }

  return content;
}

/* Last variadic argument must be sentinel NULL-value */
char* join_path(char* part, ...) {
#define ENDSWITH(str, suffixc) (suffixc != str[strlen(str) - 1])
  va_list argp;

  if (!part) return NULL;

  char* dst_path = malloc_or_die(PATH_MAX);
  strlcpy(dst_path, part, PATH_MAX);
  if (!ENDSWITH(part, '/')) strlcat(dst_path, "/", PATH_MAX);

  va_start(argp, part);

  char* oldp = NULL;
  for (;;) {
    char* p = va_arg(argp, char*);

    if (!p) break;

    if (oldp && !ENDSWITH(oldp, '/')) strlcat(dst_path, "/", PATH_MAX);

    strlcat(dst_path, p, PATH_MAX);
    oldp = p;
  }

  va_end(argp);

  return dst_path;
#undef ENDSWITH
}
