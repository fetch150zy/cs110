#include <stdio.h>
#include <assert.h>

#include "file.h"
#include "inode.h"
#include "diskimg.h"

/**
 * Fetches the specified file block from the specified inode.
 * Returns the number of valid bytes in the block, -1 on error.
 */
int file_getblock(struct unixfilesystem *fs, int inumber, int blockNum, void *buf) {
  struct inode file;
  if (inode_iget(fs, inumber, &file) < 0) // get inode
    return -1;
  int sector = inode_indexlookup(fs, &file, blockNum); // get sector(block)
  if (sector < 0)
    return -1;
  if (diskimg_readsector(fs->dfd, sector, buf) < 0) // read sector(block)
    return -1;
  int file_bytes = inode_getsize(&file); // get file size
  if (file_bytes < 0)
    return -1;
  return (blockNum == file_bytes / DISKIMG_SECTOR_SIZE 
        ? file_bytes % DISKIMG_SECTOR_SIZE : DISKIMG_SECTOR_SIZE);
}
