/* Author:			Sergey Simonenko <gforgx@protonmail.com>
 * SPDX-License-Identifier:	0BSD */

#include <stdio.h>
#include <stdlib.h>

#include "slice.h"
#include "util.h"

void* slice_new(size_t cap) {
	Slice* s;

	s = malloc_or_die(sizeof(Slice));
	s->cap = cap;
	s->len = 0;
	s->data = malloc_or_die(cap * sizeof(char*));

	pthread_mutex_init(&s->lock, NULL);

	return s;
}

void* slice_append(Slice* s, void* elem) {
	if (s->len == s->cap) {
		s->data = realloc_or_die(s->data, sizeof(char*) * (s->cap + 1));
		s->cap++;
	}

	s->data[s->len] = elem;
	s->len++;

	return s;
}

/* FIXME:
void* slice_remove(Slice* s, void* elem) {
	for (int i = 0; i < s->cap; i++) {
		if (elem != s->data[i])
			continue;
	}
}
*/

void slice_print(Slice* s, FILE* f, char* (reprcb)(void*)) {
	fprintf(f, "[");
	for (int i = 0; i < s->cap; i++) {
		reprcb(s->data[i]);
		void* elem = s->data[i];
		if (!elem) {
			fprintf(f, "(null)");
			continue;
		}

		if (reprcb)
			printf("\"%s\"", reprcb(elem));
		else
			printf("smth at \"%p\"", elem);

		if (i != s->cap - 1)
			fprintf(f, ", ");
	}
	fprintf(f, "]\n");
}

void slice_free(Slice* s, void (freecb)(void*)) {
	if (freecb) {
		for (int i = 0; i < s->len; i++)
			freecb(s->data[i]);
	}

	free(s->data);
	free(s);
}

int slice_lock(Slice* s) {
	return pthread_mutex_lock(&s->lock);
}

int slice_unlock(Slice* s) {
	return pthread_mutex_unlock(&s->lock);
}

