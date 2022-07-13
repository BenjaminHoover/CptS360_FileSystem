// CptS 360 - Lab 6 - util.c
// Base Code references from K.C Wang, and his textbook: Systems Programming in Unix/Linux
// Contributers: Benjamin Hoover
// About lab: Create the foundations of a FS system and implement cd, ls, and pwd functions
// About file: Holds main functions and initailizes the file system. 

/****************************************************************************
*                   KCW testing ext2 file system                            *
*****************************************************************************/
/*#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>*/
#include <time.h>

//#include "type.h"
//#include "util.c"
#include "mount_umount.c"

// globals
/*MINODE minode[NMINODE];
MINODE *root;

PROC   proc[NPROC], *running;*/ // moved to type.h

/*char gpath[128]; // global for tokenized components
char *name[32];  // assume at most 32 components in pathname
int   n;         // number of component strings

int fd, dev;
int nblocks, ninodes, bmap, imap, inode_start; // moved to type.h*/

MINODE *iget();

int init()
{
  int i, j;
  MINODE *mip;
  PROC   *p;

  printf("init()\n");

  for (i=0; i<NMINODE; i++){ // initialize all minodes as free
    mip = &minode[i];
    mip->dev = mip->ino = 0;
    mip->refCount = 0;
    mip->mounted = 0;
    mip->mptr = 0;
  }
  for (i=0; i<NPROC; i++){ // initialize all procs
    p = &proc[i];
    p->pid = i;
    p->uid = p->gid = i; // has this as = i in book CHANGED FROM 0 TO i
    p->cwd = 0;
    p->status = READY; // CHANGED FROM FREE TO READY
    for (j=0; j<NFD; j++) // initialize all file descriptions as null
      p->fd[j] = 0;
      p->next = &proc[i+1];
  }
  for (i = 0; i < NMTABLE; i++) { // initialize all mtable entries as free
    mtable[0].dev = 0;
  }
  proc[NPROC - 1].next = &proc[0]; // circular list
  running = &proc[0]; // first process running
}

// load root INODE and set root pointer to it
/*int mount_root(char *rootdev)
{ 
  printf("mount_root()\n");
  root = iget(dev, 2); // get root inode
  return 0;
}*/

int mount_root(char *rootdev) {
  int i; 
  MTABLE *mp;
  SUPER *sp;
  GD *gp;
  char buf[BLKSIZE];

  dev = open(rootdev, O_RDWR);
  if (dev < 0) {
    printf("panic : can't open root device\n");
    exit(1);
  }
  // get super block of rootdev
  get_block(dev, 1, buf);
  sp = (SUPER *)buf;

  if (sp->s_magic != SUPER_MAGIC) {   // check magic number
    printf("super magic = %x : %s is not an EXT2 filesys\n", sp->s_magic, rootdev);
    exit(0);
  }
  // fill mount table mtable[0] with rootdev information
  mp = &mtable[0]; // use mtable[0]
  mp->dev = dev;
  // copy super blcok info into mtable[0]
  ninodes = mp->ninodes = sp->s_inodes_count;
  nblocks = mp->nblocks = sp->s_blocks_count;
  strcpy(mp->devName, rootdev);
  strcpy(mp->mntName, "/");

  get_block(dev, 2, buf);
  gp = (GD *)buf;
  bmap = mp->bmap = gp->bg_block_bitmap;
  imap = mp->imap = gp->bg_inode_bitmap;
  iblock = mp->iblock = gp->bg_inode_table;
  printf("bmap = %d imap %d iblock = %d\n", bmap, imap, iblock);

  // call iget(), which inc minode's refCount
  root = iget(dev, 2); // get root inode
  mp->mntDirPtr = root; // double link root->mnPtr = mp;
  // set proc CWDs
  for (i = 0; i < NPROC; i++) { // set proc's CWD
    proc[i].cwd = iget(dev, 2); // each inc refCount by 1
  }
  printf("mount : %s mounted on / \n", rootdev);
  return 0;
}

char *disk = "diskimage";
int main(int argc, char *argv[ ]) /* HERE IS MAIN *******************************************************************************/
{
  int ino, f;
  char buf[BLKSIZE];
  char line[128], cmd[32], pathname1[128], pathname2[128];

  if (argc > 1)
    disk = argv[1];

  printf("checking EXT2 FS ....");
  if ((fd = open(disk, O_RDWR)) < 0){
    printf("open %s failed\n", disk);
    exit(1);
  }
  dev = fd;   

  /********** read super block  ****************/
  get_block(dev, 1, buf);
  sp = (SUPER *)buf;

  /* verify it's an ext2 file system ***********/
  if (sp->s_magic != 0xEF53){
      printf("magic = %x is not an ext2 filesystem\n", sp->s_magic);
      exit(1);
  }     
  printf("EXT2 FS OK\n");
  ninodes = sp->s_inodes_count;
  nblocks = sp->s_blocks_count;

  get_block(dev, 2, buf); 
  gp = (GD *)buf;

  bmap = gp->bg_block_bitmap;
  imap = gp->bg_inode_bitmap;
  inode_start = gp->bg_inode_table;
  printf("bmp=%d imap=%d inode_start = %d\n", bmap, imap, inode_start);

  init();  
  mount_root(disk);
  printf("root refCount = %d\n", root->refCount);
  //clrBmap(); // clear bmap
  //char bbuf[BLKSIZE];
  //bzero(bbuf, BLKSIZE);
  //put_block(dev, bmap, bbuf);
  printf("creating P0 as running process\n");
  running = &proc[0];
  running->status = READY;
  running->cwd = iget(dev, 2);
  printf("root refCount = %d\n", root->refCount);

  while(1){
    printf("input command : [ls|cd|pwd|mkdir|creat|rmdir|link|unlink|symlink|readlink]\n");
    printf("                [open|close|read|write|cat|cp|pfd|mount|umount|cs] ");
    fgets(line, 128, stdin);
    printf("line = %s", line);
    line[strlen(line)-1] = 0;
    if (line[0]==0)
       continue;
    pathname1[0] = 0;
    pathname2[0] = 0;

    printf("running dev = %d, dev = %d\n", running->cwd->dev, dev);
    sscanf(line, "%s %s %s", cmd, pathname1, pathname2);
    printf("cmd=%s pathname=%s %s\n", cmd, pathname1, pathname2);
    
    if (strcmp(cmd, "ls")==0)
       ls(pathname1);
    if (strcmp(cmd, "cd")==0)
       cd(pathname1);
    if (strcmp(cmd, "pwd")==0)
       pwd(running->cwd);
    if (strcmp(cmd, "mkdir")==0)
       make_dir(pathname1);
    if (strcmp(cmd, "creat")==0)
       creat_file(pathname1);
    if (strcmp(cmd, "rmdir")==0)
       rmdir(pathname1);
    if (strcmp(cmd, "link")==0)
       link(pathname1, pathname2);
    if (strcmp(cmd, "unlink")==0)
       unlink(pathname1);
    if (strcmp(cmd, "symlink")==0)
       symlink(pathname1, pathname2); 
    if (strcmp(cmd, "readlink")==0)
       readlinkhelp(pathname1);
    if (strcmp(cmd, "cat")==0)
       mycat(pathname1);
    if (strcmp(cmd, "cp")==0)
       mycp(pathname1, pathname2);
    if (strcmp(cmd, "open")==0)
       myopen_file(pathname1, pathname2); 
    if (strcmp(cmd, "pfd")==0)
       mypfd();
    if (strcmp(cmd, "close")==0) {
       f = atoi(pathname1);
       myclose_file(f);
    }
    if (strcmp(cmd, "read")==0)
       myread_file(pathname1, pathname2);
    if (strcmp(cmd, "write")==0)
       mywrite_file(pathname1, pathname2);
    if (strcmp(cmd, "mount")==0)
       mount(pathname1, pathname2);
    if (strcmp(cmd, "umount")==0)
       umount(pathname1);
    if (strcmp(cmd, "cs")==0)
       cs();
    if (strcmp(cmd, "quit")==0)
       quit();
  }
}

int quit()
{
  int i;
  MINODE *mip;
  for (i=0; i<NMINODE; i++){
    mip = &minode[i];
    if (mip->refCount > 0)
      iput(mip);
  }
  exit(0);
}
