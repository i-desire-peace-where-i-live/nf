/* Author:			Sergey Simonenko <gforgx@protonmail.com>
 * SPDX-License-Identifier:	0BSD */

#ifndef NOTEFINDER_IPC_H
#define NOTEFINDER_IPC_H

#define MAX_KEY_LEN 128

#include <stddef.h>
#include <stdint.h>
#include <unistd.h>

#include "map.h"

typedef enum {
  CLIENT_STATUS_OK,
  CLIENT_STATUS_ERR,
} ClientStatus;

typedef struct SharedClientState {
  pid_t pid;
  int fd0[2];
  int fd1[2];

  Map* config;
  ClientStatus (*sync)(struct SharedClientState*);
} SharedClientState;

typedef struct {
  uint64_t uuid;
  char key[MAX_KEY_LEN];
  size_t value_sz;
  char value[0];
} IPCEntryData;

int send_client_data(int fd, uint64_t uuid, const char* key, char* value);
#endif
