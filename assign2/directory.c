#include "directory.h"
#include "inode.h"
#include "diskimg.h"
#include "file.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

/**
 * Looks up the specified name (name) in the specified directory (dirinumber).  
 * If found, return the directory entry in space addressed by dirEnt.  Returns 0 
 * on success and something negative on failure. 
 */
int directory_findname(struct unixfilesystem *fs, const char *name,
		       int dirinumber, struct direntv6 *dirEnt) {
  struct inode dir;
  // dir is also a file
  if (inode_iget(fs, dirinumber, &dir) < 0)
    return -1;
  // check is a dir?
  if (!((dir.i_mode & IFMT) == IFDIR))
    return -1;
  // get dir size
  int dir_bytes = inode_getsize(&dir);
  if (dir_bytes <= 0)
    return -1;

  int blocks = (dir_bytes - 1) / DISKIMG_SECTOR_SIZE + 1;
  for (int i = 0; i < blocks; ++i) {
    struct direntv6 ds[DIRS_PRE_BLOCK];
    int valid_bytes = file_getblock(fs, dirinumber, i, ds);
    if (valid_bytes < 0)
      return -1;
    int total_dir = valid_bytes / sizeof(struct direntv6);
    for (int j = 0; j < total_dir; ++j) {
      if (0 == strcmp(ds[j].d_name, name)) {
        *dirEnt = ds[j];
        return 0;
      }
    }
  }
  return -1;
}
