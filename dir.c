/* Author:			Sergey Simonenko <gforgx@protonmail.com>
 * SPDX-License-Identifier:	0BSD */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <unistd.h>

#include "entry.h"
#include "slice.h"
#include "source.h"
#include "util.h"

static int compare_entry(const void* a, const void* b) {
  const Entry* cmp = *(Entry**)a;
  const Entry* arr_memb = *(Entry**)b;

  return cmp->uuid - arr_memb->uuid;
}

static void update_entry_from_file(Entry* entry, char* path, char* localname,
                                   struct stat* st) {
  (void)map_lock(entry->props);

  entry_set_default(entry, "filepath", strdup_or_die(path), true);
  entry_set_default(entry, "name", strdup_or_die(localname), true);

  char* old_content;
  if ((old_content = entry_get(entry, "content"))) free(old_content);

  bool is_binary = false;
  entry_set(entry, "content", read_file(path, true, &is_binary), true);

  if (is_binary)
    entry->type = ENTRY_FILE;
  else
    entry->type = ENTRY_NOTE;

  (void)map_unlock(entry->props);
  //	memcpy(entry->created_time, localtime(&st->st_ctime),
  // sizeof(struct tm)); 	memcpy(entry->modified_time,
  // localtime(&st->st_mtime), sizeof(struct tm));
}

int sync_dir(char* param, Slice* entries) {
  //  debugf("Exporting to %s", param);
  qsort(entries->data, entries->len, sizeof(char*), compare_entry);

  for (size_t i = 0; i < entries->len; i++) {
    Entry* entry = entries->data[i];

    char* filepath = entry_get(entry, "filepath");

    if (ENTRY_FLAGS_APP_DELETED == (entry->flags & ENTRY_FLAGS_APP_DELETED)) {
      unlink(filepath);
      slice_remove_by_iter(entries, i--, entry_free);

      continue;
    } else if (ENTRY_FLAGS_APP_NEW == (entry->flags & ENTRY_FLAGS_APP_NEW)) {
      char path[PATH_MAX];

      strlcpy(path, param, PATH_MAX);
      strlcat(path, "/", PATH_MAX);
      strlcat(path, entry_get(entry, "name"), PATH_MAX);

      entry_set(entry, "filepath", strdup_or_die(path), true);
      entry->flags &= ~(ENTRY_FLAGS_APP_NEW);

      // FIXME!!!
      filepath = entry_get(entry, "filepath");
    }
    // skip untouched entries
    else if (ENTRY_FLAGS_APP_UPDATED !=
             (entry->flags & ENTRY_FLAGS_APP_UPDATED))
      continue;

    char* content = entry_get(entry, "content");

    if (!content) continue;

    FILE* fp = fopen(filepath, "w");
    fputs(content, fp);
    fclose(fp);

    entry->flags &= ~(ENTRY_FLAGS_APP_UPDATED);
  }

  //  debugf("Importing from %s", param);
  struct dirent* de;
  DIR* dir;

  if (!(dir = opendir(param))) {
    //    debugf("\"%s\" doesn't exist!", param);
    return STATUS_ERR;
  }

  while ((de = readdir(dir))) {
    if (0 == strcmp(de->d_name, ".") || 0 == strcmp(de->d_name, "..")) continue;

    char path[PATH_MAX];

    strlcpy(path, param, PATH_MAX);
    strlcat(path, "/", PATH_MAX);
    strlcat(path, de->d_name, PATH_MAX);

    //    debugf("found file named: \"%s\"", path);

    struct stat st;
    if (0 != stat(path, &st) || 0 == S_ISREG(st.st_mode)) {
      /*      debugf(
                "stat() failed for \"%s\"; most likely it's not a "
                "regular file",
                path);
                */
      continue;
    }

    MKSEARCHKEY(Entry, uuid, (uint64_t)st.st_ino, p)
    Entry* entry;
    Entry** res;

    if (!(res = (bsearch(&p, entries->data, entries->len, sizeof(char*),
                         compare_entry)))) {
      entry = entry_new();
      entry->uuid = (uint64_t)st.st_ino;
      slice_append(entries, entry);
    } else {
      entry = *res;
      //      debugf("\"%s\" already exists in the cache, updating", path);
    }

    update_entry_from_file(entry, path, de->d_name, &st);

    if (!res) qsort(entries->data, entries->len, sizeof(char*), compare_entry);
  }

  closedir(dir);

  return STATUS_ONLINE;
}
