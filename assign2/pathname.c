
#include "pathname.h"
#include "directory.h"
#include "inode.h"
#include "diskimg.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

/**
 * Returns the inode number associated with the specified pathname.  This need only
 * handle absolute paths.  Returns a negative number (-1 is fine) if an error is 
 * encountered.
 */
int pathname_lookup(struct unixfilesystem *fs, const char *pathname) {
  if (0 == strcmp(pathname, "/")) { // only "/"
    return ROOT_INUMBER;
  } else {
    int inumber = ROOT_INUMBER;
    const char* path = pathname + 1;
    char* start = strchr(path, '/');
    struct direntv6 d;
    while (NULL != start) {
      char* new_path = start + 1;
      char dir[DIR_MAX_LENGTH];
      int dir_len = strlen(path) - strlen(new_path);
      strncpy(dir, path, dir_len);
      dir[dir_len - 1] = '\0';
      if (directory_findname(fs, dir, inumber, &d) < 0)
        return -1;
      inumber = d.d_inumber;
      path = new_path;
      start = strchr(path, '/');
    }
    // end of path
    if (directory_findname(fs, path, inumber, &d) < 0)
      return -1;
    return d.d_inumber;
  }
}
