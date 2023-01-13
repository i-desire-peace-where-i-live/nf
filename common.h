/* Author:			Sergey Simonenko <gforgx@protonmail.com>
 * SPDX-License-Identifier:	0BSD */

#ifndef NOTEFINDER_COMMON_H
#define NOTEFINDER_COMMON_H

#define CONFIG_FILE ".config/notefinder"

/* vim notion to jump to particular line */
#define LOG_ENTRY \
  debugf("Entered %s() \"+%d %s\"", __func__, __LINE__, __FILE__)
#define LOG_RETURN \
  debugf("Returning from %s() \"+%d %s\"", __func__, __LINE__, __FILE__)

#endif
