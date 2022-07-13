// CptS 360 - Lab 6 - util.c
// Base Code references from K.C Wang, and his textbook: Systems Programming in Unix/Linux
// Contributers: Benjamin Hoover
// About lab: Create the foundations of a FS system and implement cd, ls, and pwd functions
// About file: Implements the cd, ls, and pwd functions of the file system.

#include "util.c"

/************* cd_ls_pwd.c file **************/

int chdir(char *pathname)   
{
  printf("chdir %s\n", pathname);
  // READ Chapter 11.7.3 HOW TO chdir
  //int dev; 
  int dev;
  if (isAbsolutePath(pathname)) {
    dev = root->dev;
  } else  {
    dev = running->cwd->dev;
  }
  int ino = getino(pathname, &dev);
  //printf("pathname = %s, ino = %d\n", pathname, ino);
  if (ino == 0) { // pathname doesn't exist
    printf("%s not found\n", pathname);
    return 0; 
  } else { // pathname valid
    MINODE *mip = iget(dev, ino);
    if (mip->INODE.i_mode == DIR_MODE) { // pathname is valid directory
      iput(running->cwd); // release cwd
      running->cwd = mip; // cwd = new cwd
    } else {
      printf("%s is not a valid directory\n", pathname);
      return 0; // pathname exists but not directory 
    }
  }
}

int cd(char *pathname) {
  if (pathname) { // there is a pathname
    chdir(pathname);
  } else { // go to home dir
    chdir("/");
  }
}

int ls_file(MINODE *mip, char *name, int ino)
{
  //printf("ls_file: to be done: READ textbook for HOW TO!!!!\n");
  // READ Chapter 11.7.3 HOW TO ls
  char createTime[64];
  //int ino =  //search(mip, name);
  MINODE *tmip = iget(dev, ino);

  char *t1 = "xwrxwrxwr-------"; 
  char *t2 = "----------------";
  if (S_ISREG(tmip->INODE.i_mode)/*tmip->INODE.i_mode == FILE_MODE*/) { // if (S_ISREG()) 
    printf("%c",'-'); }
  if (tmip->INODE.i_mode == DIR_MODE) { // if (S_ISDIR()) 
    printf("%c",'d'); }
  if ((tmip->INODE.i_mode & 0xF000 ) == 0xA000) { // if (S_ISLNK()) 
    printf("%c",'l'); }
  for (int i = 8; i >= 0; i--){
      if (tmip->INODE.i_mode & (1 << i)) {// print r | w | x 
      printf("%c", t1[i]); 
    } else {
      printf("%c", t2[i]);
    }
  }
  printf("%4ld ", tmip->INODE.i_links_count);
  printf("%4d ", tmip->INODE.i_gid);
  printf("%4d ", tmip->INODE.i_uid);
  printf("%8ld ", tmip->INODE.i_size);

  strcpy(createTime, ctime(&mip->INODE.i_mtime));
  createTime[ strlen(createTime)-1] = 0;
  printf("%s ", createTime);
  printf("%s ", name);

  /*if ((mip->INODE.i_mode & 0xF000) == 0xA000) {
    // print linkame if symbolic
  } */
  //printf("\n");
  printf("\n");
  //getchar();
  iput(tmip);
}

int ls_dir(MINODE *mip)
{
  //printf("ls_dir: list CWD's file names; YOU do it for ls -l\n");

  char buf[BLKSIZE], temp[256];
  DIR *dp;
  char *cp;
  
  // Assume DIR has only one data block i_block[0]
  get_block(mip->dev, mip->INODE.i_block[0], buf); 
  dp = (DIR *)buf; // dp is partition of buf
  cp = buf; // cp is address of buf
  
  while (cp < buf + BLKSIZE){ // 
     strncpy(temp, dp->name, dp->name_len);
     temp[dp->name_len] = 0; // getting the name and null-ending it
     
     //printf("[%d %s]  ", dp->inode, temp); // print [inode# name]
     //printf("inode = %d   ", dp->inode);
     ls_file(mip, temp, dp->inode);
     
     cp += dp->rec_len; // advance cp to next entry
     dp = (DIR *)cp; // dp is partition of cp
  }
  printf("\n");
}

int ls(char *pathname)  
{
  printf("ls %s\n", pathname);
  int dev;
  if (isAbsolutePath(pathname)) {
    dev = root->dev;
  } else  {
    dev = running->cwd->dev;
  }
  //printf("ls CWD only! YOU do it for ANY pathname\n");
  if (pathname[0] != 0) { // print pathname
    int ino = getino(pathname, &dev);
    if (ino == 0) { // pathname doesn't exist
      printf("%s is not a valid pathname\n", pathname);
      return 0;
    } else {
      MINODE *mip = iget(dev, ino);
      //mip->refCount = mip->refCount - 2;
      if (mip->INODE.i_mode == DIR_MODE) {
        ls_dir(mip);
      } else {
        printf("%s is not a valid directory\n", pathname);
        return 0;
      }
      iput(mip);
    }
  } else { // print cwd
    ls_dir(running->cwd);
  }
}

char *pwd(MINODE *wd)
{
  //printf("pwd: READ HOW TO pwd in textbook!!!!\n");
  if (wd == root){
    printf("/\n");
    return;
  } else { // not root
    rpwd(wd);
    printf("\n");
  }
}

void rpwd(MINODE *wd) { // modified for cross-dev traversal upward
    u32 *my_ino, parent_ino;
    char *my_name[256];

    parent_ino = findino(wd, &my_ino); // get ino of . and ..
    //printf("my_ino = %u, parent_ino = %u\n", my_ino, parent_ino);

    MINODE *pip = iget(wd->dev, parent_ino);
    findmyname(pip, my_ino, my_name); // get name with ino
    //printf("my_name = %s\n", my_name);
    //getchar();
    if ((strcmp(my_name, "..") != 0) && (my_ino != parent_ino)) {
      rpwd(pip); // recursive call
      printf("/%s", my_name); 
    }
    if (root->dev != pip->dev) {
      //printf("mounted = %d\n", pip->mounted);
      printf("found mnt\n");
      //printf("pip->ino = %d\n", pip->ino);
      int i = 0;
      for (i = 0; i < NMTABLE; i++) {
        if (mtable[i].dev == pip->dev) { // filesys already mounted
            break;
        }
      }
      if (mtable[i].mntDirPtr == NULL) {
        printf("ERROR: mntDirPtr null\n");
      }
      //printf("mntDirPtr->ino = %d\n", mtable[i].mntDirPtr->ino);
      pip = mtable[i].mntDirPtr;
      //printf("after pip assignment, pip = %d\n", pip->ino);
      rpwd(pip);
      printf("/%s", my_name);
    }
}



