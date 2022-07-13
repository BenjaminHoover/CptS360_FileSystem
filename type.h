// CptS 360 - Lab 6 - util.c
// Base Code references from K.C Wang, and his textbook: Systems Programming in Unix/Linux
// Contributers: Benjamin Hoover
// About lab: Create the foundations of a FS system and implement cd, ls, and pwd functions
// About file: Header file that declares definitions, globals, and structs along with the header files. 

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>

/*************** type.h file ************************/
typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

typedef struct ext2_super_block SUPER;
typedef struct ext2_group_desc  GD;
typedef struct ext2_inode       INODE;
typedef struct ext2_dir_entry_2 DIR;

SUPER *sp;
GD    *gp;
INODE *ip;
DIR   *dp;

// Default dir ad regular file modes
#define DIR_MODE 0x41ED
#define FILE_MODE 0x81AE // Incorrect?
#define SUPER_MAGIC 0xEF53
#define SUPER_USER 0

// Proc status
#define FREE        0
#define READY       1

// file system table sizes
#define BLKSIZE  1024
#define NMINODE   128
#define NFD        16
#define NPROC       2
#define NMTABLE 10
#define NOFT 40


char gpath[128]; // global for tokenized components
char *name[32];  // assume at most 32 components in pathname
int   n;         // number of component strings

int fd, dev;
int nblocks, ninodes, bmap, imap, inode_start; 
int iblock;
char buf[BLKSIZE];
//char *rootdev = "mydisk"; //default root_device

// in-memory inodes structure
typedef struct minode{
  INODE INODE; // disk inode
  int dev, ino;
  int refCount; // use count
  int dirty; // modified flag
  int mounted; // mounted flag
  struct mtable *mptr; // mount table pointer 
}MINODE;

// oft structure
typedef struct oft{
  int  mode; // mode of opened file
  int  refCount; // number of PROCs sharing this instance
  MINODE *mptr; // pointer to minode of file
  int  offset; // byte offset for read/write
}OFT;

// PROC structure
typedef struct proc{
  struct proc *next;
  int          pid;
  int          ppid;
  int          status;
  int          uid, gid;
  MINODE      *cwd;
  OFT         *fd[NFD];
}PROC;

// mount table structure
typedef struct mtable {
  int dev; // device number; 0 for free
  int ninodes; // from superblock
  int nblocks; 
  int free_blocks; // from superblock and GD
  int free_inodes;
  int bmap; // from group descriptor
  int imap;
  int iblock; // inodes start block
  MINODE *mntDirPtr; // mount point DIR pointer
  char devName[64]; // device name
  char mntName[64]; // mount point DIR name
} MTABLE;

// globals
MINODE minode[NMINODE];
MTABLE mtable[NMTABLE];
MINODE *root;

PROC   proc[NPROC], *running;