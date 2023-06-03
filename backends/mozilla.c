/* Author:			Sergey Simonenko <gforgx@protonmail.com>
 * SPDX-License-Identifier:	0BSD */

#include "config.h"

#ifndef __APPLE__  // FIXME: file copying issue
#ifdef HAVE_SQLITE

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <fcntl.h>
#include <limits.h>
#include <sqlite3.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../entry.h"
#include "../slice.h"
#include "../source.h"
#include "../util.h"

enum sql_select {
  SQL_SELECT_ID,
  SQL_SELECT_TITLE,
  SQL_SELECT_URL,
};

static int compare_entry(const void* a, const void* b) {
  const Entry* cmp = *(Entry**)a;
  const Entry* arr_memb = *(Entry**)b;

  return cmp->uuid - arr_memb->uuid;
}

static int update_entry_from_select_cb(void* param1, int argc, char** argv,
                                       char** column_name) {
  if (argc != SQL_SELECT_URL + 1) {
    debugf(
        "sqlite3 returned wrong number of columns (have: %d, "
        "expected: %d)",
        argc, SQL_SELECT_URL + 1);
    return 0;
  }

  Slice* entries = param1;
  uint64_t uuid = atol(argv[SQL_SELECT_ID]);
  char* name = argv[SQL_SELECT_TITLE];
  char* url = argv[SQL_SELECT_URL];

  if (!uuid || !name || !url) {
    debug("Some of required SELECT columns is NULL, skipping");
    return 0;
  }

  MKSEARCHKEY(Entry, uuid, uuid, p)

  // FIXME: maybe update properties for already-existent bookmarks?..
  if (!bsearch(&p, entries->data, entries->len, sizeof(char*), compare_entry)) {
    Entry* entry;
    entry = entry_new();
    entry->type = ENTRY_BOOKMARK;
    entry->uuid = uuid;

    entry_set(entry, "name", strdup_or_die(name), true);
    entry_set(entry, "url", strdup_or_die(url), true);

    slice_append(entries, entry);
    qsort(entries->data, entries->len, sizeof(char*), compare_entry);
  } else
    debugf("\"%ld\" already exists in the cache, skipping", uuid);

  return 0;
}

int sync_mozilla(char* param, Slice* entries) {
#define SELECT_QUERY                    \
  "select b.id,b.title,p.url"           \
  " from moz_bookmarks b, moz_places p" \
  " where b.fk = p.id"
  int ret = STATUS_ONLINE;

  char* sql_path = join_path(get_home_dir(), ".mozilla/firefox/", param,
                             "places.sqlite", NULL);

  int in, out;
  if (-1 == (in = open(sql_path, O_RDONLY))) {
    debug("couldn't open places.sqlite");
    return STATUS_ERR;
  }

  free_and_null(sql_path);

  char tmpf[PATH_MAX];
  snprintf(tmpf, PATH_MAX, "%s/.nf-XXXXXXXX", get_home_dir());
  // FIXME: check return value
  out = mkstemp(tmpf);

  struct stat fileinfo = {0};
  fstat(in, &fileinfo);

  if (-1 == copy_file_range(in, NULL, out, NULL, fileinfo.st_size, 0)) {
    ret = STATUS_ERR;
    goto clean;
  }

  sqlite3* db = NULL;
  char* err_msg = NULL;

  if (SQLITE_OK == sqlite3_open(tmpf, &db))
    sqlite3_exec(db, SELECT_QUERY, update_entry_from_select_cb, entries,
                 &err_msg);
  else {
    debug("SQL select failed");
    ret = STATUS_ERR;
  }

  sqlite3_free(err_msg);
  sqlite3_close(db);

clean:
  unlink(tmpf);
  close(in);
  close(out);

  return ret;
#undef SELECT_QUERY
}

#endif  // HAVE_SQLITE
#endif  // !__APPLE__
