/* Author:			Sergey Simonenko <gforgx@protonmail.com>
 * SPDX-License-Identifier:	0BSD */

#ifndef NOTEFINDER_SERVER_H
#define NOTEFINDER_SERVER_H

typedef struct {
  pid_t pid;
  int fd0[2];
  int fd1[2];
#if 0
  BackendFunction* func;
  char* param;
#endif
} ChildData;

void* server_init(void*);
void* daemon_loop(void*);

#endif
