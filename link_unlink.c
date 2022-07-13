#include "rmdir.c"

INODE getINODE(char *name) {
    MINODE *mip;
    int ino;
    char *cp, temp[256], sbuf[BLKSIZE];
    DIR *dp;

    if (strcmp(name, "/")==0) {
        return root->INODE; // return root ino = 2
    }
    if (name[0] == '/') {
        mip = root; // if absolute pathname: start from root
    } else {
        mip = running->cwd; // if relative pathname: start from CWD
    }
    mip->refCount++; // in order to iput(mip) later

    tokenize(name); // assume: name[ ], nname are globals, they are
    for (int i = 0; i<n; i++) { // search for each component string
        if (!S_ISDIR(mip->INODE.i_mode)) { // check DIR type
            printf("%s is not a directory\n", name[i]);
            iput(mip);
            return;
        }
        ino = search(mip, name[i]);

        if (!ino) {
            printf("no such component name %s\n", name[i]);
            iput(mip);
            return;
        }
        iput(mip); // release current minode
        mip = iget(dev, ino); // swith to new minode
    }
  iput(mip);
  return mip->INODE;
}

int link(char *oldFileName, char *newFileName) {
    printf("inside link, old = %s, new = %s\n", oldFileName, newFileName);
    //INODE mipINODE= getINODE(oldFileName);
    char parent[128], child[128];
    int pino, oino, *my_oino, *my_pino;
    MINODE *pmip, *omip, *start;

    start = running->cwd;
    dev = running->cwd->dev;
    
    oino = getino(oldFileName, &dev);
    //oino = findino(start, &my_oino);
    omip = iget(dev, oino);
    printf("oino = %d, omip->ino = %d\n", oino, omip->ino);

    // omip is not a dir
    if (!(S_ISREG(omip->INODE.i_mode)) && ((omip->INODE.i_mode & 0xF000 ) != 0xA000)) {
        printf("ERROR: %s is a not a valid file or link file\n", oldFileName);
        return;
    }

    // newFileName doesn't exist
    if (getino(newFileName, &dev) != 0) {
        printf("ERROR: %s already exists\n", newFileName);
        return;
    }

    int childLocation = strlen(newFileName);
    while (childLocation > 0) { // start at back, go to first char
        if (newFileName[childLocation] == '/') {
            break;
        } else {
            childLocation--;
        }
    }

    if (childLocation <= 0) {
        parent[0] = '/';
        parent[1] = '\0';
    } else { strncpy(parent, newFileName, childLocation); }

    int childLen = strlen(newFileName) - childLocation;
    for (int i = 0; i < childLen; i++) {
        child[i] = newFileName[childLocation + i];
    }
    child[childLen] = 0;
    
    printf("parent = %s, child = %s\n", parent, child);
    //pino = getino(parent);
    pino = findino(start, &my_pino);
    pmip = iget(dev, my_pino);

    enter_name(pmip, oino, child);

    omip->INODE.i_links_count++;
    omip->dirty = 1;
    
    iput(omip);
    iput(omip);
    iput(pmip);

    printf("end link\n");
}

int truncate(MINODE *mip) {
    printf("inside truncate\n");
    char buf[BLKSIZE];
    DIR *dp;
    int bki = 0, tdbk = 0, dbk = 0, ibuf[256];

    while (mip->INODE.i_block[bki] != 0) {
        printf("bki = %d ", bki);
        if (bki < 12) { // direct blocks
            printf("direct block\n");
            dbk = mip->INODE.i_block[bki];
        } else if (bki >= 12 && bki < 256 + 12) { // indirect blocks
            printf("indirect block\n");
            get_block(mip->dev, mip->INODE.i_block[12], ibuf);
            dbk = ibuf[bki - 12];
        } else { // double indirect blocks
            printf("double indirect block\n");
            get_block(mip->dev, mip->INODE.i_block[13], ibuf);
            tdbk = ibuf[(int)(bki / (256 + 12)) - 1];
            get_block(mip->dev, tdbk, ibuf);
            dbk = ibuf[bki - (256 + 12)];
        }
        //get_block(mip->dev, dbk, buf);
        //dp = (DIR *)buf;
        bdealloc(mip->dev, dbk);
        bki++;
    }
    printf("blocks deallocated\n");

    //idealloc(mip->dev, mip->ino);
    mip->INODE.i_size = 0;
    mip->INODE.i_dtime = time(0L);
    mip->dirty = 1;
    printf("end truncate\n");
}

/*int truncate(MINODE *mip) {
    printf("inside truncate\n");
    char buf[BLKSIZE], temp[256];
    DIR *dp;
    char *cp;

    for (int i = 0; i < 12; i++) {
        if (mip->INODE.i_block[i] != NULL) {
            get_block(dev, mip->INODE.i_block[i], buf); 
            dp = (DIR *)buf; // dp is partition of buf
            cp = buf; // cp is address of buf
            
            while (cp < buf + BLKSIZE){ 
                printf("i = %d, dev = %d, inode = %d\n", i, mip->dev, dp->inode);
                bdealloc(mip->dev, dp->inode);
                
                cp += dp->rec_len; // advance cp to next entry
                dp = (DIR *)cp; // dp is partition of cp
            }
        }
        else {
            printf("Direct blocks emptied\n");
            break;
        }
    } 
    if (mip->INODE.i_block[12] != NULL) {
        get_block(dev, mip->INODE.i_block[12], buf);
        dp = (DIR *)buf; // dp is partition of buf
        cp = buf; // cp is address of buf
            
        while (cp < buf + BLKSIZE){ 
            printf("i = %d, dev = %d, inode = %d\n", 12, mip->dev, dp->inode);
            bdealloc(mip->dev, dp->inode);
                
            cp += dp->rec_len; // advance cp to next entry
            dp = (DIR *)cp; // dp is partition of cp
        }
    }
    printf("Indirect blocks emptied\n");
    if (mip->INODE.i_block[13] != NULL) {
        get_block(dev, mip->INODE.i_block[13], buf); 
        dp = (DIR *)buf; // dp is partition of buf
        cp = buf; // cp is address of buf
            
        while (cp < buf + BLKSIZE){ 
            printf("i = %d, dev = %d, inode = %d\n", 13, mip->dev, dp->inode);
            bdealloc(mip->dev, dp->inode);
                
            cp += dp->rec_len; // advance cp to next entry
            dp = (DIR *)cp; // dp is partition of cp
        }
    }
    printf("Double indirect blocks emptied\n");
    
    idealloc(mip->dev, mip->ino);
    mip->INODE.i_size = 0;
    mip->dirty = 1;
    printf("end truncate\n");
}*/

int unlink(char *pathname) {
    printf("start unlink, pathname = %s\n", pathname);
    char parent[128], child[128];
    int ino, pino;
    MINODE *mip, *pmip;

    ino = getino(pathname, &dev);
    mip = iget(dev, ino);

    // mip is not a dir
    if (!(S_ISREG(mip->INODE.i_mode)) && ((mip->INODE.i_mode & 0xF000 ) != 0xA000)) {
        printf("ERROR: %s is a not a valid file or link file\n", pathname);
        return;
    }

    // pathname exists
    if (getino(pathname, &dev) == 0) {
        printf("ERROR: %s already exists\n", pathname);
        return;
    }
    if ((running->uid != SUPER_USER) && (running->uid != mip->INODE.i_uid)) {
        printf("ERROR: you do not have permission to modify this directory\n");
        iput(mip);
        return -1;
    }

    int childLocation = strlen(pathname);
    while (childLocation > 0) { // start at back, go to first char
        if (pathname[childLocation] == '/') {
            break;
        } else {
            childLocation--;
        }
    }

    if (childLocation <= 0) {
        parent[0] = '/';
        parent[1] = '\0';
    } else { strncpy(parent, pathname, childLocation); }

    int childLen = strlen(pathname) - childLocation;
    for (int i = 0; i < childLen; i++) {
        child[i] = pathname[childLocation + i];
    }
    child[childLen] = 0;

    printf("parent = %s, child = %s\n", parent, child);
    pino = getino(parent, &dev);
    pmip = iget(dev, pino); 

    printf("pmip=>ino = %d, pino = %d\n", pmip->ino, pino);
    rm_child(pmip, child);

    pmip->dirty = 1;
    iput(pmip);
    mip->INODE.i_links_count--;

    if (mip->INODE.i_links_count > 0) {
        mip->dirty = 1;
        printf("file modified\n");
    } else {
        truncate(mip);
        idealloc(mip->dev, mip->ino);
        printf("file deleted\n");
    }

    iput(mip);

    printf("end unlink\n");
}