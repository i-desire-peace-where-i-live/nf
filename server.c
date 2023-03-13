/* Author:			Sergey Simonenko <gforgx@protonmail.com>
 * SPDX-License-Identifier:	0BSD */

#include "server.h"

#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common.h"
#include "config.h"
#include "ipc.h"
#include "slice.h"
#include "source.h"
#include "util.h"

static int child_count = 0;
static int child_num = -1;

#define MAX_CHILD 1024
static ChildData children[MAX_CHILD];

extern Slice* sources;
extern int nargs;

int sync_dir(char*, Slice*);
int sync_mozilla(char*, Slice*);

void child_init(int num) {
  child_num = num;

  // FIXME: this is necessary to not close fds before all are opened
  sleep(3);
  close(children[num].fd0[0]);
  close(children[num].fd1[1]);

  while (1) {
    char* value1 = "Note 1";
    char* value2 = "Text of note 1";

    ipc_entry_data_send(children[num].fd0[1], 0, "title", value1);
    ipc_entry_data_send(children[num].fd0[1], 0, "text", value2);

    sleep(3);
  }
}

void server_loop(void) {
  while (1) {
    for (int i = 0; i < child_count; i++) {
      IPCEntryData msg;
      size_t nbytes = read(children[i].fd0[0], &msg, sizeof(IPCEntryData));
      printf("Read %zu bytes\n", nbytes);

      printf("Key is: %s\n", msg.key);
      printf("Value size is: %zu\n", msg.value_sz);

      char value[msg.value_sz];
      nbytes = read(children[i].fd0[0], &value, msg.value_sz);
      printf("Read leftover %zu bytes: \"%s\"\n", nbytes, value);
    }
  }
}

void fork_recursively(int n) {
  pid_t pid;
  int nchild = child_count - n;

  if (n > 0) {
    ChildData child;

    if (-1 == pipe(child.fd0) || -1 == pipe(child.fd1))
      panicf("Failed to create a pipe");

    children[nchild] = child;

    pid = fork();

    if (-1 == pid)
      panicf("Failed to create a child process");

    else if (0 == pid)
      child_init(nchild);

    else {
      children[nchild].pid = pid;
      fork_recursively(--n);
    }
  } else {
    children[nchild].pid = pid;

    for (int i = 0; i < child_count; i++) {
      close(children[i].fd0[1]);
      close(children[i].fd1[0]);
    }

    server_loop();
  }
}

void* server_init(void* unused) {
  (void)unused;

  child_count = 1;  // sources->len;

  fork_recursively(child_count);
}

void* daemon_loop(void* unused) {
  (void)unused;

  for (;;) {
    slice_lock(sources);

    for (size_t i = 0; i < sources->len; i++) {
      Source* src = sources->data[i];
      //      debugf("Source type is: %d, parameter is: %s", src->type,
      //      src->param);

      switch (src->type) {
        case SOURCE_DIR:
          sync_dir(src->param, src->entries);
          break;
#ifdef HAVE_SQLITE
        case SOURCE_MOZILLA:
          sync_mozilla(src->param, src->entries);
          break;
#endif
        default:
          continue;
      }
    }

    slice_unlock(sources);

    if (nargs > 1) return 0;

    sleep(SYNCER_SLEEP_DURATION);
  }
}
