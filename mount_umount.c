#include "write_cp.c"

int mount(char *filesys, char *mount_point) {
    char buf[BLKSIZE];
    int ino;
    SUPER *sp;
    GD *gp;
    MTABLE *fs;
    MINODE *mip;
    int dev;
    if (isAbsolutePath(mount_point)) {
        dev = root->dev;
    } else  {
        dev = running->cwd->dev;
    }

    if (filesys[0] == '\0' && mount_point[0] == '\0') { // display current mounted filesystems
        return displayMount();
    } else if (filesys && !mount_point) { // only entered fpathname
        printf("ERROR: must include a mount_point\n");
        return -1;
    }

    // check if filesys is already mounted
    for (int i = 0; i < NMTABLE; i++) {
        if (strcmp(mtable[i].devName, filesys) == 0) { // filesys already mounted
            printf("ERROR: %s already mounted\n", filesys);
            return -1;
        }
    }

    // allocate free mtable entry
    fs = (MTABLE *)malloc(sizeof(MTABLE));
    if (!fs) {
        printf("ERROR: unable to allocate mtable entry\n");
        return -1;
    }
    
    fs->dev = open(filesys, O_RDWR); // open filesys for read
    printf("dev = %d, fs->dev = %d\n", dev, fs->dev);

    get_block(fs->dev, 1, buf); // get filesys superblock
    sp = (SUPER *)buf; 
    
    if (sp->s_magic != SUPER_MAGIC) { // check if filesys is EXT2 system with magic number
        printf("ERROR: super magic = %x : %s is not an EXT2 filesys\n", sp->s_magic, filesys);
        free(fs);
        return -1;
    }
    strcpy(fs->devName, filesys);

    // get mount_point's ino and minode
    ino = getino(mount_point, &dev);
    mip = iget(dev, ino);

    if (mip->INODE.i_mode != DIR_MODE) {
        printf("ERROR: %s is not a directory\n", mount_point);
        iput(mip);
        free(fs);
        return -1;
    }
    if (mip->refCount != 1) { // double check this
        printf("ERROR: directory is busy\n");
        iput(mip);
        free(fs);
        return -1;
    }

    // "Record mtable entries contents
    get_block(dev, 2, buf);
    gp = (GD *)buf;

    fs->mntDirPtr = (MINODE *)malloc(sizeof(MINODE));
    fs->mntDirPtr = mip;
    strcpy(fs->mntName, mount_point);
    // inodes
    fs->iblock = gp->bg_inode_table;
    fs->imap = gp->bg_inode_bitmap;
    fs->ninodes = sp->s_inodes_count;
    fs->free_inodes = sp->s_free_inodes_count;
    // blocks
    fs->bmap = gp->bg_block_bitmap;
    fs->nblocks = sp->s_blocks_count = 1440;
    fs->free_blocks = sp->s_free_blocks_count;

    mip->mounted = 1; // mark mount_point as mounted
    mip->mptr = fs; // point mptr at mtable entry
    //MINODE *rip = iget(fs->dev, 2);
    //rip->mptr = fs;
    //mip->mptr->mntDirPtr = mip;
    printf("mip->mounted = %d, mip->mptr->devName = %s\n", mip->mounted, mip->mptr->devName);
    printf("fs->mntDirPtr->ino = %d, mip->mptr->mntDirPtr->ino = %d\n", fs->mntDirPtr->ino, mip->mptr->mntDirPtr->ino);
    int k = 0;
    while (mtable[k].dev != NULL) {
        k++;
    }
    mtable[k] = *fs;
    
    return 0;
}

int displayMount() {
    printf("Currently mounted filesystems\n");
    printf("dev devName mntName\n");
    printf("===---===---===---===---===\n");
    for (int i = 0; i < NMTABLE; i++) {
        if (mtable[i].dev != NULL) {
            printf("%d ", mtable[i].dev);
            printf("%s ", mtable[i].devName);
            printf("%s\n", mtable[i].mntName);
        } else {
            printf("NULL\n");
        }
    }
    printf("===---===---===---===---===\n");
    return;
}

int umount(char *filesys) {
    printf("begin umount\n");
    MTABLE *fs;
    MINODE *mip, *rip;
    int i = 0;

    // check that filesys is mounted
    for (; i < NMTABLE; i++) {
        if (strcmp(mtable[i].devName, filesys) == 0) {
            fs = &mtable[i];
            break;
        }
    }
    if (fs->dev < 0) { // filesys not mounted
        printf("ERROR: %s is not mounted\n", filesys); 
        return -1;
    }

    for (int i = 0; i < NMINODE; i++) { // check for active files in filesys
        if (minode[i].dev == fs->dev) {
            printf("ERROR: %s is busy\n", filesys);
            return -1;
        }
    }

    // alter mount_point;
    mtable[i].dev = NULL;
    strcpy(mtable[i].devName, "");
    mip = fs->mntDirPtr;
    mip->mounted = 0; 
    iput(mip);
    printf("end umount\n");
    return 0;
}

int cs() { // switch running processes between P0 and P1
    if (&proc[0] == running) { // at P0
        running = running->next;
    } else {
        running = &proc[0];
    }
}