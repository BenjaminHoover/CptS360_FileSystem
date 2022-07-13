#include "mkdir_creat.c"

int clr_bit(char *buf, int bit) {
    unsigned int i = bit / 8;
    unsigned int j = bit % 8;

    buf[i] &= ~(1 << j);
}

int incFreeInodes(int dev) {
    char buf[BLKSIZE];
    get_block(dev, 1, buf);
    sp = (SUPER *)buf;
    sp->s_free_inodes_count++;
    put_block(dev, 1, buf);
    get_block(dev, 2, buf);
    gp = (GD *)buf;
    gp->bg_free_inodes_count++;
    put_block(dev, 2, buf);
}

int idealloc(int dev, int ino) {
    int i;
    char buf[BLKSIZE];
    if (ino > ninodes) {
        printf("inumber %d out of range\n", ino);
        return;
    }
    // get inode bitmap block
    get_block(dev, imap, buf);
    clr_bit(buf, ino-1);
    put_block(dev, imap, buf);
    
    incFreeInodes(dev);
}

int incFreeBlocks(int dev) {
    char buf[BLKSIZE];
    get_block(dev, 1, buf);
    sp = (SUPER *)buf;
    sp->s_free_blocks_count++;
    put_block(dev, 1, buf);
    get_block(dev, 2, buf);
    gp = (GD *)buf;
    gp->bg_free_blocks_count++;
    put_block(dev, 2, buf);
}

int bdealloc(int dev, int bno) {
    int i;
    char buf[BLKSIZE];
    if (bno > nblocks) {
        printf("block number %d out of range\n", bno);
        return;
    }
    // get inode bitmap block
    get_block(dev, bmap, buf);
    clr_bit(buf, bno-1);
    put_block(dev, bmap, buf);
    
    incFreeBlocks(dev);
}

int rm_child(MINODE *parent, char *name) {
    char *cp, sbuf[BLKSIZE];
    DIR *dp, *pdp;
    int dpFound = 0;
    int i = 0;

    for (i=0; i<12; i++) { // search DIR direct blocks only
        if (parent->INODE.i_block[i] == 0) {
            printf("ERROR: %s not found\n", name);
            return 0;
        }
        get_block(parent->dev, parent->INODE.i_block[i], sbuf);
        dp = pdp = (DIR *)sbuf;
        cp = sbuf;
        while ((dpFound == 0) && (cp < sbuf + BLKSIZE)) {
            printf("%8d%8d%8u %s\n", dp->inode, dp->rec_len, dp->name_len, dp->name);
            if (strncmp(name, dp->name, dp->name_len) == 0) { // found it
                dpFound = 1;
            } else { // iterate to next
                pdp = dp;
                cp += dp->rec_len;
                dp = (DIR *)cp;
            }
        }
        if (dpFound == 1) {
            printf("%s found\n", name);
            break;
        }
    }
    printf("dp->name = %s, name = %s\n", dp->name, name);
    //parent->INODE.i_size = parent->INODE.i_size - BLKSIZE;

    if (pdp == dp) { // first entry
        printf("first entry\n");
        bdealloc(dev, dp->inode); // deallocate block
        parent->INODE.i_size = parent->INODE.i_size - BLKSIZE;
        for (; i <11; i++) { // move all blocks right 1
            parent->INODE.i_block[i] = parent->INODE.i_block[i + 1];
        }
    } else if ((cp + dp->rec_len) >= sbuf + BLKSIZE) { // last entry
        printf("last entry\n");

        pdp->rec_len = pdp->rec_len + dp->rec_len;
    } else { // middle entry
        printf("middle entry\n");
        int removedRec = dp->rec_len;
        //int temp;
        while (cp < sbuf + BLKSIZE && (pdp->rec_len != 0)) { // move all entries left
            //pdp = dp;
            cp += dp->rec_len;
            pdp = (DIR *)cp;
            printf("dp = %s, pdp = %s\n", dp->name, pdp->name);
            printf("dplen = %d, pdplen = %d\n", dp->rec_len, pdp->rec_len);
            memcpy(dp, pdp, pdp->rec_len);
            printf("after dp = %s, pdp = %s\n", dp->name, pdp->name);
            if (pdp->rec_len != 0) {
                dp = (DIR *)cp;
            }
            //getchar();
        }
        //getchar();
        printf("done\n");
        printf("reclen = %d\n", dp->rec_len);
        dp->rec_len += removedRec;
        printf("reclen = %d\n", dp->rec_len);
    }

    printf("Saving changes\n");
    put_block(parent->dev, parent->INODE.i_block[0], sbuf);
    parent->dirty = 1;
    printf("Saving complete, rm_child end\n");
}

int rmdir (char *pathname) {
    printf("rmdir begin, pathname = %s\n", pathname);
    char *cp, temp[256], sbuf[BLKSIZE], name[128], parent[128], child[128];
    DIR *dp;
    MINODE *start, *mip, *pmip;
    u32 *myino;
    int pino, ino;

    // get dev
    /*if (pathname == root) { // absolute pathname
        start = root;
        dev = root->dev;
        printf("Relative, dev = %d\n", dev);
    } else { // relative pathname
        start = running->cwd;
        dev = running->cwd->dev;
        printf("Relative, dev = %d\n", dev);
    }*/

    printf("dev = %d\n", dev);
    printf("running = %d\n", running->cwd->dev);
    if (isAbsolutePath(pathname) == 1) { // absolute pathname
        printf("absolute pathname %s\n", pathname);
        start = root;
        dev = root->dev;

        // /a/b/c
        parChildCreate(pathname, parent, child);
        printf("parent = %s, child = %s\n", parent, child);
        strcpy(name, child);

        pino = getino(parent, &dev);
        pmip= iget(dev, pino);

        printf("pino = %d\n", pino);
    } else { // relative pathname
        printf("relative pathname %s\n", pathname);
        start = running->cwd;
        dev = running->cwd->dev;

        strcpy(name, pathname);
        printf("start->dev = %d\n", start->dev);
    }

    // get ino of pathname and minode pointer
    ino = getino(pathname, &dev);
    mip = iget(dev, ino);
    //mip->refCount--;
    printf("ino = %d, mip->ino = %d, mip->refCount = %d\n", ino, mip->ino, mip->refCount);

    if ((running->uid != SUPER_USER) && (running->uid != mip->INODE.i_uid)) {
        printf("ERROR: you do not have permission to modify this directory\n");
        iput(mip);
        return -1;
    }
    if (mip->INODE.i_mode != DIR_MODE) {
        printf("ERROR: %s is not a directory\n", pathname);
        iput(mip);
        return;
    }
    if (mip->refCount != 1) {
        printf("ERROR: directory is busy\n");
        iput(mip);
        return;
    }

    get_block(mip->dev, mip->INODE.i_block[0], sbuf);
    dp = (DIR *)sbuf;
    cp = sbuf;
    int numEntries = 0;
    while (cp < sbuf + BLKSIZE) { // ensure directory is empty
      strncpy(temp, dp->name, dp->name_len);
      temp[dp->name_len] = 0;
      printf("%8d%8d%8u %s\n", dp->inode, dp->rec_len, dp->name_len, temp);
      numEntries++;
      if (strncmp(dp->name, "..", 2) == 0) {
          printf("pino %d found\n", dp->inode);
          pino = dp->inode;
      }
      cp += dp->rec_len;
      dp = (DIR *)cp;
    } 
    if (numEntries > 2) { // must be greater than 2
        printf("ERROR: directory is not empty\n");
        printf("Number of entries: %d\n", numEntries);
        iput(mip);
        return;
    }
    printf("Directory empty\n");
    //pino = findino(mip, myino);
    pmip = iget(mip->dev, pino);
    //findmyname(pmip, ino, name);
    printf("pino = %d, pmip->ino = %d, name = %s\n", pino, pmip->ino, name);

    // remove name from parent directory
    rm_child(pmip, name);
    printf("%s removed from parent directory\n", pathname);

    // save changes
    pmip->INODE.i_links_count--;
    pmip->dirty = 1;
    iput(pmip);
    printf("Changes to parent directory saved\n");

    // deallocate its data blocks and inode
    //bdealloc(mip->dev, mip->INODE.i_block[0]);
    for (int i = 0; i < 12; i++){
         if (mip->INODE.i_block[i]==0)
             continue;
         bdealloc(mip->dev, mip->INODE.i_block[i]);
    }
    idealloc(mip->dev, mip->ino);
    iput(mip);
    printf("Memory of %s deallocated\n", pathname);
    printf("end rmdir\n");
}