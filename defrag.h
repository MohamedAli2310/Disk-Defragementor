#ifndef __DEFRAG_H__
#define __DEFRAG_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define N_DBLOCKS 10
#define N_IBLOCKS 4
#define BLOCKSIZE 512
#define N_INODES 

typedef struct superblock {
  int size;
  int inode_offset;
  int data_offset;
  int swap_offset;
  int free_inode;
  int free_block;
} superblock;

typedef struct inode {
  int next_inode;
  int protect;
  int nlink;
  int size;
  int uid;
  int gid;
  int ctime;
  int mtime;
  int atime;
  int dblocks[N_DBLOCKS];
  int iblocks[N_IBLOCKS];
  int i2block;
  int i3block;
} inode;

#endif
