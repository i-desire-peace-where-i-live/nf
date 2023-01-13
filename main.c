/* Author:			Sergey Simonenko <gforgx@protonmail.com>
 * SPDX-License-Identifier:	0BSD */

#include "config.h"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE  // for strcasestr()
#endif

#include <fcntl.h>
#include <limits.h>
#include <pthread.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <unistd.h>

#ifdef HAVE_PERL
#include "perl.h"
#endif

#include "cfg.h"
#include "common.h"
#include "entry.h"
#include "server.h"
#include "slice.h"
#include "source.h"
#include "util.h"

Slice* sources = NULL;
char* last_query = NULL;
extern int debug_yes;
int nargs = 0;

int show_window(int, char**);
int exec_cmds(int, char**);

int main(int argc, char** argv) {
  if (getenv("DEBUG")) debug_yes = 1;
  debugf("This is main entry: %d\n", getpid());

  nargs = argc;

  char* config_path = join_path(get_home_dir(), CONFIG_FILE, NULL);

  sources = get_sources_from_config(config_path);
  free(config_path);

#ifdef HAVE_PERL
  PerlInterpreter* perl = init_perl();

  do_perl(perl, "hook_main_entry");
#endif
  pthread_t syncer_tid;
  pthread_create(&syncer_tid, NULL, daemon_loop, NULL);

  pthread_t server_tid;
  pthread_create(&server_tid, NULL, server_init, NULL);
#ifdef HAVE_GTK_3
  if (1 == argc)
    show_window(argc, argv);
  else
#endif
  {
    pthread_join(syncer_tid, NULL);
    exec_cmds(argc, argv);
  }
clean:
#ifdef HAVE_PERL
  do_perl(perl, "hook_clean");
  destroy_perl(perl);
#endif
  slice_free(sources, src_free);
  exit(EXIT_SUCCESS);
}
