/* Author:			Sergey Simonenko <gforgx@protonmail.com>
 * SPDX-License-Identifier:	0BSD */

#ifndef NOTEFINDER_MAP_H
#define NOTEFINDER_MAP_H

#include <pthread.h>

typedef struct MapData {
  void* V;

  char* K;
  unsigned int K_hash;

  struct MapData* next;
} MapData;

typedef struct {
  MapData** data;

  size_t len;
  size_t cap;

  pthread_mutex_t lock;
} Map;

MapData* mapdata_init(const char*, void*);
void mapdata_free(MapData*);

Map* map_new(int);
void map_free(Map*);
void map_put(Map*, const char*, void*);
MapData* map_get_md(Map* m, const char* k);
void* map_get(Map*, const char*);
int map_lock(Map*);
int map_unlock(Map*);

#endif
