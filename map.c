/* Author:			Sergey Simonenko <gforgx@protonmail.com>
 * SPDX-License-Identifier:	0BSD */

/*
 * Mostly written after BSD make hashtable implementation being reference.
 * Only `const char*` keys are supported
 */

#include "map.h"

#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

#define MAP_HASH(var, key) \
  var = 0;                 \
  const char* p;           \
  for (p = key; *p != '\0'; p++) var = 31 * var + (unsigned char)*p;

MapData* mapdata_init(const char* k, void* v) {
  MapData* md = malloc_or_die(sizeof(MapData));

  md->V = v;
  md->K = strdup(k);
  MAP_HASH(md->K_hash, k);
  md->next = NULL;

  return md;
}

// TODO: need something like optional callback for mapdata_free(), map_free()

void mapdata_free(MapData* md) {
  free_and_null(md->K);
  free_and_null(md);
}

Map* map_new(int cap) {
  Map* m;

/* http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2Float */
#define NEXT_POW_TWO(v) \
  v--;                  \
  v |= v >> 1;          \
  v |= v >> 2;          \
  v |= v >> 4;          \
  v |= v >> 8;          \
  v |= v >> 16;         \
  v++;

  NEXT_POW_TWO(cap);
#undef NEXT_POW_TWO

  m = malloc_or_die(sizeof(Map));
  m->data = malloc_or_die(sizeof(MapData) * cap);

  m->len = 0;
  m->cap = cap;

  for (int i = 0; i < cap; i++) m->data[i] = NULL;

  pthread_mutex_init(&m->lock, NULL);

  return m;
}

void map_free(Map* m) {
  for (int i = 0; i < m->cap; i++) {
    if (!m->data[i]) continue;

    mapdata_free(m->data[i]);
  }

  free_and_null(m->data);
  free_and_null(m);
}

static void map_resize(Map* m, size_t newsz) {}

void map_put(Map* m, const char* k, void* v) {
  MapData* md_found;
  md_found = map_get_md(m, k);

  if (md_found) {
    if (0 != strcmp(md_found->K, k)) {
      while (md_found->next) {
        md_found = (MapData*)md_found->next;
      }

      MapData* md = mapdata_init(k, v);
      md_found->next = md;
      m->len++;

      return;
    } else {
      md_found->V = v;
      return;
    }
  }

  MapData* md = mapdata_init(k, v);

  // FIXME: resize m->data if required

  m->data[md->K_hash & (m->cap - 1)] = md;
  m->len++;
}

MapData* map_get_md(Map* m, const char* k) {
  unsigned int h;
  MAP_HASH(h, k);

  MapData* first_hit = m->data[h & (m->cap - 1)];

  return first_hit;
}

void* map_get(Map* m, const char* k) {
  MapData* md;

  if ((md = map_get_md(m, k))) {
    if (0 == strcmp(md->K, k)) return md->V;

    while (md->next) {
      md = md->next;

      if (0 == strcmp(md->K, k)) return md->V;
    }
  }

  return NULL;
}

int map_lock(Map* m) { return pthread_mutex_lock(&m->lock); }

int map_unlock(Map* m) { return pthread_mutex_unlock(&m->lock); }

#undef MAP_HASH
