/* Author:			Sergey Simonenko <gforgx@protonmail.com>
 * SPDX-License-Identifier:	0BSD */

#include <EXTERN.h>
#include <perl.h>

#include "XSUB.h"
#include "common.h"
#include "entry.h"
#include "slice.h"
#include "source.h"
#include "syncer.h"
#include "util.h"

PerlInterpreter* init_perl(void);
void hook(PerlInterpreter*, char*);
