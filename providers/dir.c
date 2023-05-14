/* Author:			Sergey Simonenko <gforgx@protonmail.com>
 * SPDX-License-Identifier:	0BSD */

#include <dirent.h>
#include <string.h>
#include <sys/param.h>
#include <sys/stat.h>

#include "../common.h"
#include "../entry.h"
#include "../ipc.h"
#include "../util.h"

ClientStatus provider_delete_dir(SharedClientState* p, Entry* e) { return 0; }

ClientStatus provider_put_dir(SharedClientState* p, Entry* e) {
  LOG_ENTRY;

  LOG_RETURN;
  return CLIENT_STATUS_OK;
}

ClientStatus provider_sync_dir(SharedClientState* p) {
  LOG_ENTRY;

  ClientStatus ret = CLIENT_STATUS_OK;
  char* dir_path = map_get(p->config, "param");
  DIR* dir;
  struct dirent* de;

  if (!(dir = opendir(dir_path))) {
    debugf("\"%s\" doesn't exist!", dir_path);
    ret = CLIENT_STATUS_ERR;
    goto out;
  }

  while ((de = readdir(dir))) {
    if (0 == strcmp(de->d_name, ".") || 0 == strcmp(de->d_name, "..")) continue;

    char path[PATH_MAX];

    strlcpy(path, dir_path, PATH_MAX);
    strlcat(path, "/", PATH_MAX);
    strlcat(path, de->d_name, PATH_MAX);

    struct stat st;
    // TODO: this has to be recursive
    if (0 != stat(path, &st) || 0 == S_ISREG(st.st_mode)) {
      debugf(
          "stat() failed for \"%s\"; most likely it's not a "
          "regular file",
          path);

      continue;
    }

    bool is_binary = false;
    char* text = read_file(path, true, &is_binary);

    send_client_data(p->fd0[1], (uint64_t)st.st_ino, "title", de->d_name);
    if (!is_binary)
      send_client_data(p->fd0[1], (uint64_t)st.st_ino, "text", text);
  }

out:
  closedir(dir);

  LOG_RETURN;
  return ret;
}
