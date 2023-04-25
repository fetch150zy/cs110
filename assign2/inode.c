#include <stdio.h>
#include <assert.h>

#include "inode.h"
#include "diskimg.h"

/**
 * Fetches the specified inode from the filesystem. 
 * Returns 0 on success, -1 on error.  
 */
int inode_iget(struct unixfilesystem *fs, int inumber, struct inode *inp) {
  // get the truth index of inode(inumber starts at 1 not 0)
  int inode_index = inumber - 1;

  // read a block, because inodes start at 2 and 512/32 inodes in one block
  struct inode inodes[INODES_PRE_BLOCK];
  if (diskimg_readsector(fs->dfd, 
    INODE_START_SECTOR + inode_index / INODES_PRE_BLOCK, inodes) < 0)
    return -1;
  
  // store the special inode(with inumber) to inp
  *inp = inodes[inode_index % INODES_PRE_BLOCK];
  return 0;
}

/**
 * Given an index of a file block, retrieves the file's actual block number
 * of from the given inode.
 *
 * Returns the disk block number on success, -1 on error.  
 */
int inode_indexlookup(struct unixfilesystem *fs, struct inode *inp, int blockNum) {
  // check alloced?
  if ((inp->i_mode & IALLOC) == 0)
    return -1;
  // if it is the small file, indirect addressing
  if ((inp->i_mode & ILARG) == 0)
    return inp->i_addr[blockNum];
  
  // one inode can store 8 addrs in i_addr[]
  int addrs_pre_inode = sizeof(inp->i_addr) / sizeof(uint16_t);
  // singly-indirect addressing 
  if (blockNum < ADDRS_PRE_BLOCK * addrs_pre_inode) {
    // all of 8 addrs store singly-indirect addrs, 8 * 256 blocks
    int block_offt = blockNum / ADDRS_PRE_BLOCK;
    uint16_t addr = inp->i_addr[block_offt];
    uint16_t addrs[ADDRS_PRE_BLOCK];
    if (diskimg_readsector(fs->dfd, addr, addrs) < 0)
      return -1;
    return addrs[blockNum % ADDRS_PRE_BLOCK];
  } else { // doubly-indirect addressing
    // blockNum - (256 * 7), just 8th with doubly-indirect addressing
    // 7 * 256 + 256 * 256 blocks
    int block_index = blockNum - (ADDRS_PRE_BLOCK * (addrs_pre_inode - 1));
    uint16_t addr = inp->i_addr[addrs_pre_inode - 1];
    uint16_t addrs[ADDRS_PRE_BLOCK];
    if (diskimg_readsector(fs->dfd, addr, addrs) < 0)
      return -1;
    // addressing again
    uint16_t new_addr = addrs[block_index / ADDRS_PRE_BLOCK];
    if (diskimg_readsector(fs->dfd, new_addr, addrs) < 0)
      return -1;
    return addrs[block_index % ADDRS_PRE_BLOCK];
  }
}

/**
 * Computes the size in bytes of the file identified by the given inode
 */
int inode_getsize(struct inode *inp) {
  return ((inp->i_size0 << 16) | inp->i_size1); 
}
