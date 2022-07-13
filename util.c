// CptS 360 - Lab 6 - util.c
// Base Code references from K.C Wang, and his textbook: Systems Programming in Unix/Linux
// Contributers: Benjamin Hoover
// About lab: Create the foundations of a FS system and implement cd, ls, and pwd functions
// About file: Handles background functions that implement the file system and are used in higher level functions.

#include "type.h"
/*********** util.c file ****************/

/*char *name[64]; // token string pointers
char gline[256]; // holds token strings, each pointed by a name[i]
int nname; // number of token strings*/

int isAbsolutePath(char *pathname){
  for (int i = 0; i < strlen(pathname); i++) {
    if (pathname[i] == '/') {
      return 1;
    }
  }
  return 0;
}

MINODE *mialloc() { // allocate a free minode for use
  int i;
  for (i = 0; i < NMINODE; i ++) {
    MINODE *mp = &minode[i];
    if (mp->refCount == 0) {
      mp->refCount = 1;
      return mp;
    }
  }
  printf("FS panic: out of minodes\n");
  return 0;
}

int midalloc(MINODE *mip) { // release a used minode
  mip->refCount = 0;
}

int get_block(int dev, int blk, char *buf)
{
   lseek(dev, (long)blk*BLKSIZE, 0);
   int n = read(dev, buf, BLKSIZE);
   if (n < 0) { printf("get_block [%d %d] error\n", dev, blk); }
}   
int put_block(int dev, int blk, char *buf)
{
   lseek(dev, (long)blk*BLKSIZE, 0);
   int n = write(dev, buf, BLKSIZE);
   if (n != BLKSIZE) { printf("get_block [%d %d] error\n", dev, blk); }
}   

int tokenize(char *pathname) {
  // copy pathname into gpath[]; tokenize it into name[0] to name[n-1]
  // Code in Chapter 11.7.2 
  char *s;
  strcpy(gpath, pathname);
  n = 0;
  s = strtok(gpath, "/");
  while (s) {
    name[n++] = s;
    s = strtok(0, "/");
  }
}


MINODE *iget(int dev, int ino)
{
  // return minode pointer of loaded INODE=(dev, ino)
  // Code in Chapter 11.7.2
  MINODE *mip;
  //MINODE minode[NMINODE]; // added, one exists in main.c
  MTABLE *mp;
  INODE *ip;
  int i, block, offset;
  char buf[BLKSIZE];

  // search in-memory minodes first
  //printf("iget ino = %d\n", ino);
  for (int i=0; i < NMINODE; i++) {
    MINODE *mip = &minode[i]; // changed MINODE[] to minode[]
    //printf("refCount %d, dev %d, ino %d, inode %d\n", mip->refCount, mip->dev, mip->ino);
    if (mip->refCount && (mip->dev == dev) && (mip->ino == ino)) {
      mip->refCount++;
      return mip;
    }
  }
  //printf("In iget, ino not found, making a new one\n");
  // needed INODE = (dev, ino) not in memory
  mip = mialloc(); // allocate a free minode
  mip->dev = dev;
  mip->ino = ino; 
  // assign to (dev,ino)
  block = (ino-1)/8 + iblock; // disk block containing this inode 
  offset = (ino-1) %8; // which inode in this block
  get_block(dev, block, buf);
  ip = (INODE*)buf + offset;
  mip->INODE = *ip; // copy inode to minode.INODE
  // initialize minode
  mip->refCount = 1;
  mip->mounted = 0;
  mip->dirty = 0;
  mip->mptr = 0; // changed mountedptr to mptr
  return mip;
}

void iput(MINODE *mip)
{
  // dispose of minode pointed by mip
  // Code in Chapter 11.7.2
  INODE *ip;
  int i, block, offset;
  char buf[BLKSIZE];

  if (mip == 0) {return;}
  mip->refCount--; // decrease refCount by 1
  if (mip->refCount > 0) {return;} // still has user
  if (mip->dirty == 0) {return;} // no need to write back

  // wirte INODE back to disk
  block = (mip->ino -1) / 8 + iblock;
  offset = (mip->ino -1) % 8;

  // get block containing this inode
  get_block(mip->dev, block, buf);
  ip = (INODE *)buf + offset; // ip points at INODE
  *ip = mip->INODE; // copy INODE to inode in block
  put_block(mip->dev, block, buf); // write back to disk
  midalloc(mip); // mip->refCount = 0;
} 

int search(MINODE *mip, char *name)
{
  // search for name in (DIRECT) data blocks of mip->INODE
  // return its ino
  // Code in Chapter 11.7.2
  int i;
  char *cp, temp[256], sbuf[BLKSIZE];
  DIR *dp;
  for (i=0; i<12; i++) { // search DIR direct blocks only
    if (mip->INODE.i_block[i] == 0) {
        return 0;
    }
    get_block(mip->dev, mip->INODE.i_block[i], sbuf);
    dp = (DIR *)sbuf;
    cp = sbuf;
    while (cp < sbuf + BLKSIZE) {
      strncpy(temp, dp->name, dp->name_len);
      temp[dp->name_len] = 0;
      printf("%8d%8d%8u %s\n", dp->inode, dp->rec_len, dp->name_len, temp);
      if (strcmp(name, temp) == 0) {
        printf("found %s : inumber = %d\n", name, dp->inode);
        return dp->inode;
      }
      cp += dp->rec_len;
      dp = (DIR *)cp;
    }
  }
  return 0;
}

int getino(char *pathname, int *dev) // alter to include cross-dev-traversal
{
  // return ino of pathname
  // Code in Chapter 11.7.2
  MINODE *mip;
  int i, ino;
  if (strcmp(pathname, "/")==0) {
    return 2; // return root ino = 2
  }
  if (isAbsolutePath(pathname) == 1) {
    *dev = root->dev;
    mip = root; // if absolute pathname: start from root
  } else {
    *dev = running->cwd->dev;
    mip = running->cwd; // if relative pathname: start from CWD
  }
  mip->refCount++; // in order to iput(mip) later
  printf("inside iget, dev = %d\n", *dev);
  tokenize(pathname); // assume: name[ ], nname are globals, they are

  for (int i = 0; i<n; i++) { // search for each component string
    if (!S_ISDIR(mip->INODE.i_mode)) { // check DIR type
      printf("%s is not a directory\n", name[i]);
      iput(mip);
      return 0;
    }
    printf("mip->ino = %d\n", mip->ino);
    if (mip->mounted == 1) { // is mip mounted?
      printf("mip is mounted\n");
      printf("pdev = %d, ndev = %d\n", *dev, mip->mptr->dev);
      *dev = mip->mptr->dev;
      ino = 2;
    } else { // continue as usual
      ino = search(mip, name[i]);
      if (!ino) {
        printf("no such component name %s\n", name[i]);
        iput(mip);
        return 0;
      }
    }
    iput(mip); // release current minode
    mip = iget(*dev, ino); // swith to new minode
    if (mip->mounted == 1) {
      printf("MOUNTED\n");
      *dev = mip->mptr->dev;
      printf("fs->dev = %d\n", mip->mptr->dev);
      printf("dev = %d\n", *dev);
      ino = 2;
      iput(mip);
      mip = iget(*dev, 2);
      printf("mip = %d, = %d\n", mip->ino, mip->INODE.i_mode);
    }
  }
  iput(mip);
  return ino;
}

int findmyname(MINODE *parent, u32 myino, char *myname) 
{
  // WRITE YOUR code here:
  // search parent's data block for myino;
  // copy its name STRING to myname[ ];
  char *cp, sbuf[BLKSIZE];
  DIR *dp;

  get_block(parent->dev, parent->INODE.i_block[0], sbuf);
  dp = (DIR *)sbuf;
  cp = sbuf;

  while (cp < sbuf + BLKSIZE) {
    //printf("inode = %d, my_ino  %d\n", dp->inode, myino);
    if (dp->inode == myino) { // found ino
      strcpy(myname, dp->name);
      myname[dp->name_len] = 0;
      printf("name = %s\n", myname);
      return 1;
    }
    cp += dp->rec_len;
    dp = (DIR *)cp;
  }
  printf("nothing\n");
  return 0;
}

int findino(MINODE *mip, u32 *myino) // myino = ino of . return ino of ..
{
  // mip->a DIR minode. Write YOUR code to get mino=ino of .
  //                                         return ino of ..
  printf("Inside findino\n");
  char *cp, sbuf[BLKSIZE];
  DIR *dp;

  get_block(mip->dev, mip->INODE.i_block[0], sbuf);
  dp = (DIR *)sbuf;
  cp = sbuf;
  while (cp < sbuf + BLKSIZE) {
    //printf("dp->name = %s, cp = %d, to = %d\n", dp->name, cp, (sbuf + BLKSIZE));
    if (strncmp(dp->name, ".", dp->name_len) == 0) { // found ino
      printf("return = %d\n", dp->inode);
      *myino = dp->inode;
    } else if (strncmp(dp->name, "..", dp->name_len) == 0) {
      printf("return ..  = %d\n", dp->inode);
      return dp->inode;
    }
    cp += dp->rec_len;
    dp = (DIR *)cp;
  }
}

char *parChildCreate(char *pathname, char* parent, char *child) { // return child and parent from pathname
  printf("Inside parChildCreate\n");
  //char parent[128];
  int childLocation = strlen(pathname);
  while (childLocation > 0) { // start at back, go to first char
    if (pathname[childLocation] == '/') {
      break;
    } else {
      childLocation--;
    }
  }
  printf("current childLocation = %d\n", childLocation);
  if (childLocation <= 0) {
    parent[0] = '/';
    parent[1] = '\0';
  } else { 
    strncpy(parent, pathname, childLocation); 
    parent[childLocation] = 0;
  }
  int childLen = strlen(pathname) - childLocation;
  for (int i = 0; i < childLen; i++) {
    child[i] = pathname[childLocation + i + 1];
  }
  child[childLen] = 0;
  printf("end parChildCreate\n");
  //return parent;
}
