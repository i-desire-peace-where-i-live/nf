/* Author:			Sergey Simonenko <gforgx@protonmail.com>
 * SPDX-License-Identifier:	0BSD */

#include "ipc.h"

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common.h"
#include "config.h"
#include "ipc.h"
#include "map.h"
#include "util.h"

static Map* data;
static int client_count = 0;
#define MAX_CLIENTS 1024
static SharedClientState clients[MAX_CLIENTS];
#undef MAX_CLIENTS

static void client_loop(SharedClientState c) {
  LOG_ENTRY;
  pid_t ppid = getppid();

  while (!kill(ppid, 0)) {
    if (c.sync) c.sync(&c);
    sleep(30);
  }

  LOG_RETURN;
}

void provider_init(SharedClientState p) {
  LOG_ENTRY;

  close(p.fd0[0]);
  close(p.fd1[1]);

  client_loop(p);
  LOG_RETURN;
  exit(EXIT_SUCCESS);
}
int send_client_data(int fd, uint64_t uuid, const char* key, char* value) {
  LOG_ENTRY;

  size_t value_sz = strlen(value) + 1;
  size_t total_sz = sizeof(IPCEntryData) + value_sz;
  IPCEntryData* msg = malloc_or_die(total_sz);

  msg->uuid = uuid;
  strlcpy(msg->key, key, MAX_KEY_LEN);
  msg->value_sz = value_sz;
  strlcpy(msg->key, key, MAX_KEY_LEN);
  strlcpy((char*)msg->value, value, value_sz);

  debugf("write() resulted in %zd bytes", write(fd, msg, total_sz));

  LOG_RETURN;
  return 0;
}

static int process_client_data(int client) {
  LOG_ENTRY;

  int count = 0;
  SharedClientState provider = clients[client];
  IPCEntryData msg;
  size_t nbytes;
  size_t max_value_sz = 0;

  while (-1 != (nbytes = read(provider.fd0[0], &msg, sizeof(IPCEntryData)))) {
    printf("Got: %zu bytes\n", nbytes);
    char value[msg.value_sz];

    if (msg.value_sz > max_value_sz) max_value_sz = msg.value_sz;

    nbytes = read(provider.fd0[0], &value, msg.value_sz);
    debugf("Read leftover %zu bytes for %lld, max value sz is: %zu: \"%s\"",
           nbytes, msg.uuid, max_value_sz, value);
    count += 1;
  }

  debugf("Exited loop.......................\n");

  LOG_RETURN;
  return count;
}

void server_event_loop(void) {
  LOG_ENTRY;

  for (int i = 0; i < client_count; i++) {
    printf("Got %d entries from client %d\n", process_client_data(i), i);
  }

  LOG_RETURN;
}

static void fork_clients(int forks_left) {
#define IN_CLIENT(pid) (0 == pid)
  pid_t pid;
  int client_num = client_count - forks_left;

  LOG_ENTRY;

  if (forks_left > 0) {
    SharedClientState client = clients[client_num];
    pid = fork();

    if (-1 == pid)
      panicf("Failed to create a client process");

    else if (IN_CLIENT(pid))
      provider_init(client);

    else {
      client.pid = pid;
      fork_clients(--forks_left);
    }
  } else {
    clients[client_num].pid = pid;

    for (int i = 0; i < client_count; i++) {
      /* Unnecessary ends of pipes */
      close(clients[i].fd0[1]);
      close(clients[i].fd1[0]);
    }

    while (1) server_event_loop();
  }

  LOG_RETURN;
#undef IN_CLIENT
}

void* init_server(char* config_path) {
  FILE* config_fp;
  char* config_entry = NULL;
  size_t line_width;
  int num = 0;

  LOG_ENTRY;

  config_fp = fopen(config_path, "r");
  if (!config_fp)
    panicf("couldn't open the configuration file: \"%s\"", config_path);

  // First just count the number of possible clients for memory allocation
  errno = 0;
  while (-1 != getline(&config_entry, &line_width, config_fp) && 0 == errno) {
    if (*config_entry != '#') client_count++;
  }
  if (0 != errno)
    panicf("Configuration cannot be parsed, error is: \"%s\"", strerror(errno));

  // Parse actual configuration
  data = map_new(client_count);
  rewind(config_fp);
  while (-1 != getline(&config_entry, &line_width, config_fp)) {
    Map* config;

    char* name = strtok(config_entry, ":");
    char* type = strtok(NULL, ":");
    char* param = strtok(NULL, ":");
    if (!name || !type || *type == '#' || !param) {
      continue;
    }

    SharedClientState client;
    if (-1 == pipe(client.fd0) || -1 == pipe(client.fd1))
      panicf("Failed to create a pipe");

    client.config =
        map_new(1);  // TODO: currenty we have only one notebook parameter
    map_put(client.config, "param", strdup(param));

    map_put(data, name, map_new(256));

    if (0 == strcmp(type, "dir"))
      client.sync = NULL;  // provider_sync_dir
    else if (0 == strcmp(type, "mozilla"))
      client.sync = NULL;  // provider_sync_mozilla;
    else if (0 == strcmp(type, "keep"))
      client.sync = NULL;  // provider_sync_keep;
    else
      client.sync = NULL;

    clients[num++] = client;
  }

  fclose(config_fp);
  free(config_entry);
  fork_clients(client_count);

out:
  map_free(data);

  LOG_RETURN;
  return 0;
}
