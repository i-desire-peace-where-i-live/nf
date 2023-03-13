/* Author:			Sergey Simonenko <gforgx@protonmail.com>
 * SPDX-License-Identifier:	0BSD */

#include "ipc.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "util.h"

int ipc_entry_data_send(int fd, uint64_t uuid, const char* key, char* value) {
  size_t value_sz = strlen(value) + 1;
  size_t total_sz = sizeof(IPCEntryData) + value_sz;

  IPCEntryData* msg = malloc_or_die(total_sz);
  strlcpy(msg->key, key, MAX_KEY_LEN);
  msg->value_sz = value_sz;
  strlcpy((char*)&msg->value, value, value_sz);

  printf("write() resulted in %d bytes\n", write(fd, msg, total_sz));

  return 0;
}
