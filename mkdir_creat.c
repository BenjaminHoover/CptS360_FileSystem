#include "cd_ls_pwd.c"

int tst_bit(char *buf, int bit) { // returning state of bit in buf
    /*unsigned int i = bit / 8;
    unsigned int j = bit % 8;

    if (buf[i] && 1 << j) {
        //printf("bit %d = 1\n", bit);
        return 1;
    }
    //printf("bit %d = 0\n", bit);
    return 0;*/
    return (buf[bit/8] && (1 << (bit % 8)));
}

int set_bit(char *buf, int bit) { // setting bit in buf to 1
    unsigned int i = bit / 8;
    unsigned int j = bit % 8;

    buf[i] = 1;
}

void clrBmap() {
    int i;
    char buf[BLKSIZE];
    MTABLE *mp = &mtable[0];
    get_block(dev, mp->bmap, buf);

    /*for (i = 0; i < mp->nblocks; i++) {
        if (tst_bit(buf, i) == 0){
            break;
        }
    }*/
    for (i = 49; i < mp->nblocks; i++) {
        unsigned int k = i / 8;
        unsigned int j = i % 8;
        buf[i] = 0;
    }
    put_block(dev, mp->bmap, buf);
}

int decFreeInodes(int dev) {
    get_block(dev, 1, buf);
    sp = (SUPER *)buf;
    sp->s_free_inodes_count--;
    put_block(dev, 1, buf);
    get_block(dev, 2, buf);
    gp = (GD *)buf;
    gp->bg_free_inodes_count--;
    put_block(dev, 2, buf);
}

/*int ialloc(int dev) { // allocate an inode number from inode_bitmap
    //printf("dev = %d\n", dev);
    int i;
    char buf[BLKSIZE];
    //MTABLE *mp = mtable[dev];
    // read inode_bitmap block
    get_block(dev, imap, buf);

    for (i = 0; i < ninodes; i++) {
        //printf("ialloc i = %d\n", i);
        if (tst_bit(buf, i) == 0){
            set_bit(buf, i);
            put_block(dev, imap, buf);
            //decFreeInodes(dev);
            printf("allocated ino = %d\n", i+1); // bits count from 0; ino from 1
            return i + 1;
        }
    }
    return 0;
}*/

int ialloc(int dev) { // allocate an inode number from inode_bitmap
    //printf("dev = %d\n", dev);
    int i;
    char buf[BLKSIZE];
    MTABLE *mp = &mtable[0];
    // read inode_bitmap block
    get_block(dev, mp->imap, buf);

    for (i = 0; i < mp->ninodes; i++) {
        //printf("ialloc i = %d\n", i);
        if (tst_bit(buf, i) == 0){
            set_bit(buf, i);
            put_block(dev, mp->imap, buf);
            decFreeInodes(dev);
            printf("allocated ino = %d\n", i+1); // bits count from 0; ino from 1
            return i + 1;
        }
    }
    return 0;
}

int decFreeBlocks(int dev) {
    get_block(dev, 1, buf);
    sp = (SUPER *)buf;
    sp->s_free_blocks_count--;
    put_block(dev, 1, buf);
    get_block(dev, 2, buf);
    gp = (GD *)buf;
    gp->bg_free_blocks_count--;
    put_block(dev, 2, buf);
}

/*int balloc(dev) {
    printf("dev = %d\n", dev);
    int i;
    char buf[BLKSIZE];
    //MTABLE *mp = &mtable[dev];
    // read block_bitmap block
    get_block(dev, bmap, buf);

    for (i = 0; i < nblocks; i++) {
        //printf("balloc i = %d\n", i);
        if (tst_bit(buf, i) == 0){
            set_bit(buf, i);
            put_block(dev, bmap, buf);
            //decFreeBlocks(dev);
            printf("allocated block = %d\n", i+1); 
            return i + 1;
        }
    }
    printf("ERROR: no space on bmap\n");
    return 0;
}*/

int balloc(dev) {
    printf("dev = %d\n", dev);
    int i;
    char buf[BLKSIZE], mbuf[BLKSIZE];
    MTABLE *mp;
    for (int j = 0; j < NMTABLE; j++) {
        if (mtable[j].dev == dev) {
            mp = &mtable[j];
            break;
        }
    }
    printf("dev = %d, mp->dev = %d\n", dev, mp->dev);
    printf("nblocks = %d\n", mp->nblocks);
    // read block_bitmap block
    get_block(dev, mp->bmap, buf);

    for (i = 0; i < mp->nblocks; i++) {
        printf("balloc i = %d\n", i);
        if (tst_bit(buf, i) == 0){
            set_bit(buf, i);
            put_block(dev, mp->bmap, buf);
            //decFreeBlocks(dev);
            printf("allocated block = %d\n", i+1); 
            return i + 1;
        }
    }
    printf("ERROR: no space on bmap\n");
    return 0;
}

/*int insertIntoList(MINODE *m) {
    for (int i=0; i < NMINODE; i++) {
        MINODE *mip = &minode[i]; // changed MINODE[] to minode[]
        if (mip->ino == 0) {
            mip = m; 
            printf("Insertion into minode list successful\n");
            return;
        }
    }
    printf("Insertion into minode list failed\n");
}*/

int make_dir(char *pathname) {
    printf("make_dir\n");
    MINODE *start, *pip;
    char parent[128], child[128];
    int pino, cwdino;
    u32 *my_ino;

    printf("dev = %d\n", dev);
    printf("running = %d\n", running->cwd->dev);
    if (isAbsolutePath(pathname) == 1) { // absolute pathname
        printf("absolute pathname %s\n", pathname);
        start = root;
        cwdino = running->cwd->ino;
        dev = root->dev;

        // /a/b/c
        parChildCreate(pathname, parent, child);
        printf("parent = %s, child = %s\n", parent, child);
        //getchar();
        pino = getino(parent, &dev);
        pip= iget(dev, pino);

        printf("pino = %d\n", pino);
    } else { // relative pathname
        printf("relative pathname %s\n", pathname);
        start = running->cwd;
        cwdino = running->cwd->ino;
        dev = running->cwd->dev;

        strcpy(child, pathname);
        printf("start->dev = %d\n", start->dev);
        //pino = findino(start, &my_ino); // get ino of . and ..
        //printf("pino = %d\n", pino);
        //MINODE *pip = iget(dev, pino);
        //pip = iget(dev, pino);
        pip = start;
        printf("child = %s\n", child);
    }

    printf("pip->ino = %d, pip->refcount = %d\n", pip->ino, pip->refCount);
    // verify parent INODE is a DIR
    if (pip->INODE.i_mode != DIR_MODE) { 
        printf("ERROR: parent is not a directory\n");
        return; 
    }
    // Verify child does not exist in parent directory
    if (childExists(pip, child) == 1) { 
        printf("ERROR: chid already exists\n");
        return; 
    } // prob
    printf("pip is a DIR and child doesn't exist\n");

    mymkdir(pip, child);
    printf("post mymkdir\n");

    pip->INODE.i_links_count++;
    pip->INODE.i_atime = time(0L);
    pip->dirty = 1;

    iput(pip);
    printf("mkdir end\n");
    printf("cwd = %d, %d\n", running->cwd->ino, running->cwd->INODE.i_links_count);
    printf("start = %d, %d\n", start->ino, start->INODE.i_links_count);
    //running->cwd = start;
    running->cwd = iget(dev, cwdino);
    printf("cwd = %d, %d\n", running->cwd->ino, running->cwd->INODE.i_links_count);
    printf("start = %d, %d\n", start->ino, start->INODE.i_links_count);
    //iput(running->cwd);
}

int childExists(MINODE *pip, char *child) {
    char *cp, sbuf[BLKSIZE];
    DIR *dp;

    get_block(pip->dev, pip->INODE.i_block[0], sbuf);
    dp = (DIR *)sbuf;
    cp = sbuf;
    while (cp < sbuf + BLKSIZE) {
        if (strcmp(dp->name, child) == 0) { // found ino
            printf("Found %s\n", dp->name);
            return 1;
        }
        cp += dp->rec_len;
        dp = (DIR *)cp;
    }
    return 0;
}

int mymkdir(MINODE *pip, char *name) {
    printf("mymkdir begin\n");
    MINODE *mip;
    
    int ino = ialloc(pip->dev);
    int bno = balloc(pip->dev);
    printf("Post allocate\n");
    printf("ino = %d, bno = %d\n", ino, bno);
    if (ino == 0 || ino == NULL || bno == 0 || bno == NULL) {
        return;
    }

    mip = iget(dev, ino);
    //mip->ino = ino;
    //mip->refCount = 1;
    //mip->dev = dev;
    //mip->mounted = 1;

    mip->INODE.i_mode = DIR_MODE;
    mip->INODE.i_uid = running->uid;
    mip->INODE.i_gid = running->gid;
    mip->INODE.i_size = BLKSIZE;
    mip->INODE.i_links_count = 2;
    mip->INODE.i_atime = mip->INODE.i_ctime = mip->INODE.i_mtime = time(0L);
    mip->INODE.i_blocks = 2;
    mip->INODE.i_block[0] = bno;
    for (int i = 1; i <= 14; i++) {
        mip->INODE.i_block[i];
    }
    mip->dirty = 1;
    //insertIntoList(mip);
    iput(mip);

    char *cp, buf[BLKSIZE];
    DIR *dp;

    get_block(mip->dev, mip->INODE.i_block[0], buf);
    dp = (DIR *)buf;
    cp = buf;

    dp->inode = ino;
    strcpy(dp->name, ".");
    dp->name_len = 1;
    dp->rec_len = 12;
    dp->file_type = DIR_MODE;
    printf("inode = %d, name = %s, name_len = %d, rec_len = %d\n", dp->inode, dp->name, dp->name_len, dp->rec_len);
    //printf(strncmp(dp->name, ".", 1));
    cp += dp->rec_len;
    dp = (DIR *)cp;

    dp->inode = pip->ino;
    strcpy(dp->name, "..");
    dp->name_len = 2;
    dp->rec_len = 1012;
    dp->file_type = DIR_MODE;
    printf("inode = %d, name = %s, name_len = %d, rec_len = %d\n", dp->inode, dp->name, dp->name_len, dp->rec_len);

    put_block(mip->dev, mip->INODE.i_block[0], buf);
    
    enter_name(pip, ino, name);
    printf("Post Enter_name\n");

    pip->dirty = 1;
    pip->INODE.i_links_count++;
    printf("before iput\n");
    iput(pip);
    printf("mymkdir end\n");
}

int enter_name(MINODE *pip, int myino, char *myname) {
    printf("Enter_name start\n");
    char *cp, temp[256], sbuf[BLKSIZE];
    DIR *dp;

    for (int i=0; i<12; i++) { // search DIR direct blocks only
        printf("Searching DIR block %d, iblock = %d\n", i, pip->INODE.i_block[i]);
        if (pip->INODE.i_block[i] == 0) {
            printf("enter_name done\n");
            break;
        }
    get_block(pip->dev, pip->INODE.i_block[i], sbuf);
    int need_length = 4*( (8 + strlen(myname) + 3)/4 );
    dp = (DIR *)sbuf;
    cp = sbuf;

    printf("step to last entry in block %d = %d\n", i, pip->INODE.i_block[i] );
    while (cp + dp->rec_len < sbuf + BLKSIZE) { // step to last entry in block
      strncpy(temp, dp->name, dp->name_len);
      temp[dp->name_len] = 0;
      printf("%8d%8d%8u %s\n", dp->inode, dp->rec_len, dp->name_len, temp);
      
      if (strcmp(name, temp) == 0) {
        printf("found %s : inumber = %d\n", name, dp->inode);
        return;
      }
      
      cp += dp->rec_len;
      dp = (DIR *)cp;
    } // at last entry
    printf("%8d%8d%8u %s\n", dp->inode, dp->rec_len, dp->name_len, temp);
    int ideal_length = ( 4*((8 + dp->name_len + 3)/4));
    int remain = dp->rec_len - ideal_length;
    printf("remain = %d, ideal_length = %d, name = %s\n", remain, ideal_length, myname);
    if ( remain >= need_length ) { // enough room on block
        printf("adding to current block\n");
        dp->name[dp->name_len] = 0;
        dp->rec_len = ideal_length;
        //strncpy(dp->name, dp->name, ideal_length + 1);
        //temp[ideal_length] = 0;
        printf("dp: name_len = %d, file_type: %d, rec_len = %d, name = %s\n", dp->name_len, dp->file_type, dp->rec_len, dp->name);
        cp += ideal_length; 
        dp = (DIR *)cp;

        dp->inode = myino;
        strcpy(dp->name, myname);
        dp->name_len = strlen(myname);
        //dp->file_type = DIR_MODE;
        dp-> rec_len = remain;
        printf("dp: name_len = %d, file_type: %d, rec_len = %d, name = %s\n", dp->name_len, dp->file_type, dp->rec_len, dp->name);
        printf("writting back to block %d\n", i);
        put_block(pip->dev, pip->INODE.i_block[i], sbuf);
        printf("data block written to disk\n");
        return 0;
    } else { // no room on block
        printf("no space on current block, creating new one\n");
        int *bno = balloc(pip->dev);

        if (bno == 0) {
            return -1;
        }

        pip->INODE.i_block[i] = bno;
        pip->INODE.i_size += BLKSIZE;
        get_block(pip->dev, pip->INODE.i_block[i], sbuf);

        cp = sbuf;
        dp = (DIR *)cp;
        dp->inode = myino;

        strcpy(dp->name, myname);
        dp->name_len = strlen(myname);
        //dp->file_type = DIR_MODE;
        dp-> rec_len = BLKSIZE;   
        printf("dp: name_len = %d, file_type: %d, rec_len = %d, name = %s\n", dp->name_len, dp->file_type, dp->rec_len, dp->name);
        printf("writting back to block %d\n", i);
        put_block(pip->dev, pip->INODE.i_block[i], sbuf);
        printf("data block written to disk\n");
        return 0;
    }
    }
    //printf("End of ifelse statement\n");
    
    printf("Enter_name end\n");
}

int creat_file(char *pathname) {
    printf("creat_file, pathname = %s\n", pathname);
    MINODE *start, *pip;
    char parent[128], child[128];
    int pino, cwdino, dev;
    u32 *my_ino;

    printf("dev = %d\n", dev);
    printf("running = %d\n", running->cwd->dev);
    if (isAbsolutePath(pathname) == 1) { // absolute pathname
        printf("absolute pathname %s\n", pathname);
        start = root;
        cwdino = running->cwd->ino;
        dev = root->dev;

        // /a/b/c
        parChildCreate(pathname, parent, child);
        printf("parent = %s, child = %s\n", parent, child);
        pino = getino(parent, &dev);
        pip = iget(dev, pino);
        printf("pino = %d\n", pino);
    } else { // relative pathname
        printf("relative pathname %s\n", pathname);
        start = running->cwd;
        cwdino = running->cwd->ino;
        dev = running->cwd->dev;

        strcpy(child, pathname);
        //pino = findino(start, &my_ino); // get ino of . and ..
        //printf("pino = %d\n", pino);
        //pip = iget(dev, start->ino);
        pip = start;
        printf("child = %s\n", child);
    }

    // verify parent INODE is a DIR
    if (pip->INODE.i_mode != DIR_MODE) { 
        printf("ERROR: parent is not a directory\n");
        return 0; 
    }
    // Verify child does not exist in parent directory
    if (childExists(pip, child) == 1) { 
        printf("ERROR: child already exists\n");
        return 0; 
    } // prob
    printf("pip is a DIR and child doesn't exist\n");

    my_creat(pip, child);
    printf("post my_creat\n");

    pip->INODE.i_atime = time(0L);
    pip->dirty = 1;

    iput(pip);
    printf("creat end\n");
    printf("cwd = %d, %d\n", running->cwd->ino, running->cwd->INODE.i_links_count);
    printf("start = %d, %d\n", start->ino, start->INODE.i_links_count);
    //running->cwd = start;
    running->cwd = iget(dev, cwdino);
    printf("cwd = %d, %d\n", running->cwd->ino, running->cwd->INODE.i_links_count);
    printf("start = %d, %d\n", start->ino, start->INODE.i_links_count);
    //iput(running->cwd);
    return 1;
}

int my_creat(MINODE *pip, char *name) {
    printf("my_creat begin\n");
    
    MINODE *mip;
    
    int ino = ialloc(pip->dev);
    int bno = balloc(pip->dev);
    printf("Post allocate\n");
    printf("ino = %d\n", ino);
    if (ino == 0 || ino == NULL) {
        return;
    }

    mip = iget(pip->dev, ino);
    mip->ino = ino;
    mip->refCount = 1;
    mip->dev = pip->dev;
    mip->mounted = 1;

    mip->INODE.i_mode = FILE_MODE;
    mip->INODE.i_uid = running->uid;
    mip->INODE.i_gid = running->gid;
    mip->INODE.i_size = 0;
    mip->INODE.i_links_count = 1;
    mip->INODE.i_atime = mip->INODE.i_ctime = mip->INODE.i_mtime = time(0L);
    mip->INODE.i_blocks = 2;
    mip->INODE.i_block[0] = bno;
    for (int i = 1; i <= 14; i++) {
        mip->INODE.i_block[i];
    }
    mip->dirty = 1;
    iput(mip);

    enter_name(pip, ino, name);
    printf("Post Enter_name\n");

    pip->dirty = 1;
    printf("before iput\n");
    iput(pip);
    printf("my_creat end\n");
}
