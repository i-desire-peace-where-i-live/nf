/* Author:			Sergey Simonenko <gforgx@protonmail.com>
 * SPDX-License-Identifier:	0BSD */

#ifndef NOTEFINDER_CFG_H
#define NOTEFINDER_CFG_H

#include "slice.h"
#include "source.h"

#define CONFIG_FILE ".config/notefinder"

Slice* get_sources_from_config(char* config_path);
#endif
