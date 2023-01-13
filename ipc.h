/* Author:			Sergey Simonenko <gforgx@protonmail.com>
 * SPDX-License-Identifier:	0BSD */

#ifndef NOTEFINDER_IPC_H
#define NOTEFINDER_IPC_H

#define MAX_KEY_LEN 128

#include <stdint.h>

typedef struct {
  uint64_t uuid;
  char key[MAX_KEY_LEN];
  size_t value_sz;
  char value[0];
} IPCEntryData;

int ipc_entry_data_send(int fd, uint64_t uuid, const char* key, char* value);
#endif
