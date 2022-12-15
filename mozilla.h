/* Author:			Sergey Simonenko <gforgx@protonmail.com>
 * SPDX-License-Identifier:	0BSD */

#ifndef NOTEFINDER_MOZILLA_H
#define NOTEFINDER_MOZILLA_H

#include "slice.h"

enum sql_select {
	SQL_SELECT_ID,
	SQL_SELECT_TITLE,
	SQL_SELECT_URL,
};

int sync_mozilla(char*, Slice*);

#endif

