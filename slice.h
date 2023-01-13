/* Author:			Sergey Simonenko <gforgx@protonmail.com>
 * SPDX-License-Identifier:	0BSD */

#ifndef NOTEFINDER_SLICE_H
#define NOTEFINDER_SLICE_H

#include <pthread.h>
#include <stdio.h>

typedef struct {
  void** data;
  size_t len;
  size_t cap;

  pthread_mutex_t lock;
} Slice;

void* slice_new(size_t);
void* slice_append(Slice*, void*);
void slice_remove(Slice*, void*, void(freecb)(void*));
void slice_remove_by_iter(Slice*, size_t, void(freecb)(void*));
void slice_print(Slice*, FILE*, char*(reprcb)(void*));
void slice_free(Slice*, void(freecb)(void*));
int slice_lock(Slice*);
int slice_unlock(Slice*);

#endif
