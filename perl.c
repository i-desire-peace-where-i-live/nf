/* Author:			Sergey Simonenko <gforgx@protonmail.com>
 * SPDX-License-Identifier:	0BSD */

#include "config.h"

#ifdef HAVE_PERL

#include <EXTERN.h>
#include <perl.h>
#include <string.h>

#include "XSUB.h"
#include "common.h"
#include "entry.h"
#include "slice.h"
#include "source.h"
#include "syncer.h"
#include "util.h"

#define SCRIPT_PATH "/home/gforgx/test.pl"

XS(XS_Notefinder_CPrintf) {
  dXSARGS;
  char* arg1 = SvPV_nolen(ST(0));
  printf("Got from Perl: %s\n", arg1);

  ST(0) = newSVpv("hello from C", 0);
  XSRETURN(1);
}

static void xs_init(pTHX) {
  newXS("Notefinder::CPrintf", XS_Notefinder_CPrintf, __FILE__);
}

PerlInterpreter* init_perl(void) {
  PerlInterpreter* perl = perl_alloc();
  perl_construct(perl);

  return perl;
}

void hook(PerlInterpreter* perl, char* hook) {
  char* perl_args[] = {"", SCRIPT_PATH};

  perl_parse(perl, xs_init, 2, perl_args, NULL);
  perl_run(perl);

  char* args[] = {NULL};

  call_argv("update_entry_hook", G_DISCARD | G_NOARGS | G_EVAL, args);
}

#endif  // HAVE_PERL
