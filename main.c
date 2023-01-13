/* Author:			Sergey Simonenko <gforgx@protonmail.com>
 * SPDX-License-Identifier:	0BSD */

#include <pthread.h>
#include <stdlib.h>  // free()
#include <unistd.h>  // sleep()

#include "config.h"

#ifdef HAVE_PERL
#include "perl.h"
#endif

#include "common.h"
#include "util.h"

#ifdef HAVE_PERL
PerlInterpreter* perl;
#endif
extern int enable_debug;

void* init_server(void*);

int main(int argc, char** argv) {
  char* config_path;
  pthread_t server_tid;

  if (1 || getenv("DEBUG")) {  // FIXME
    enable_debug = 1;
    LOG_ENTRY;
  }

  config_path = join_path(get_home_dir(), CONFIG_FILE, NULL);

#ifdef HAVE_PERL
  if (!(perl = perl_alloc())) panicf("perl_alloc() returned NULL");
  perl_construct(perl);
  hook(perl, "startup");
#endif

  pthread_create(&server_tid, NULL, init_server, config_path);
  for (;;) sleep(100);  // FIXME

clean:
#ifdef HAVE_PERL
  hook(perl, "cleanup");
  perl_destruct(perl);
  perl_free(perl);
#endif

  pthread_join(server_tid, NULL);
  free(config_path);

  LOG_RETURN;
  exit(EXIT_SUCCESS);
}
