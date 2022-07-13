#include "symlink.c"

int myopen_file(char *pathname, char *mode) {
    printf("open_close_lseek\n");
    MINODE *mip;
    char parent[128], child[128];
    int ino, ofmode;
    OFT *oftp = (OFT *)malloc(sizeof(OFT));
    
    printf("dev = %d\n", dev);
    printf("running = %d\n", running->cwd->dev);
    if (isAbsolutePath(pathname) == 1) { // absolute pathname
        printf("absolute pathname %s\n", pathname);
        dev = root->dev;
    } else { // relative pathname
        printf("relative pathname %s\n", pathname);
        dev = running->cwd->dev;
    }
    ino = getino(pathname, &dev);
    if (ino == 0) {
        printf("%s doesn't exist, creating %s\n", pathname, pathname);
        creat_file(pathname);
        ino = getino(pathname, &dev);
    }
    mip = iget(dev, ino);
    printf("mip->ino = %d, mip->INODE.i_mode = %d\n", mip->ino, mip->INODE.i_mode);

    // verify INODE is a file
    if (!S_ISREG(mip->INODE.i_mode)) { 
        printf("ERROR: INODE is not a file\n");
        return -1; 
    }
    printf("INODE is a file\n");

    // check if file is already opened
    for (int j = 0; j < 12; j++) {
        if ((running->fd[j] != NULL) && (running->fd[j]->mptr == mip)) { // file is already opened
            if (running->fd[j]->mode != 0) { // file is opened for W, RW, or APPEND
                printf("ERROR: file already opened\n");
            }
        }
    }
    printf("file not yet opened\n");

    // set mode from input
    if (strcmp(mode, "0") == 0) {
        printf("setting mode to read\n");
        ofmode = 0;
    } else if (strcmp(mode, "1") == 0) {
        printf("setting mode to write\n");
        ofmode = 1;
    } else if (strcmp(mode, "2") == 0) {
        printf("setting mode to read/write\n");
        ofmode = 2;
    } else if (strcmp(mode, "3") == 0) {
        printf("setting mode to append\n");
        ofmode = 3;
    }

    oftp->mode = ofmode;
    oftp->refCount = 1;
    oftp->mptr = mip;

    switch(ofmode) {
        case 0: // read
            printf("Opening %s for read\n", pathname);
            oftp->offset = 0;
        break;
        case 1: // write
            printf("Opening %s for write\n", pathname);
            truncate(mip);
            oftp->offset = 0;
        break;
        case 2: // read write
            printf("Opening %s for read write\n", pathname);
            oftp->offset = 0;
        break;
        case 3: // append
            printf("Opening %s for append\n", pathname);
            oftp->offset = mip->INODE.i_size;
        break;
        default:
            printf("ERROR: unrecognized mode\n");
            return -1;
        break;
    }

    int i = 0;
    while ((running->fd[i] != NULL) && (i < 16)) { // get smallest fd that is null
        printf("fd[%d] mode = %d, ino = %d\n", i, running->fd[i]->mode, running->fd[i]->mptr->ino);
        i++;
    }
    if (i >= 12) { // shouldn't be here
        printf("ERROR: null fd not found\n");
        return -1;
    } else {
        printf("null fd %d found\n", i); 
    }

    running->fd[i] = oftp;
    printf("fd[%d] mode: = %d, ino = %d\n", i, running->fd[i]->mode, running->fd[i]->mptr->ino);

    if (ofmode == 0) { // for R
        mip->INODE.i_atime = time(0L);
    } else { // for W|RW|APPEND
        mip->INODE.i_atime = time(0L);
        mip->INODE.i_mtime = time(0L);
    }
    mip->dirty = 1;

    printf("open_close_lseek end\n");
    return i;
}

int myclose_file(int fd) {
    printf("\nclose_file\n");
    OFT *oftp;
    MINODE *mip;

    // verify fd is within range
    if (fd > 12 || fd < 0) {
        printf("ERROR: fd %d is out of range\n", fd);
        return;
    }

    // verify running->fd[fd] is pointing at a OFT entry
    if (running->fd[fd] == NULL) {
        printf("ERROR: fd %d is not an OFT entry\n", fd);
        return;
    }

    // remove file from running->fd
    oftp = running->fd[fd];
    running->fd[fd] = 0;
    oftp->refCount--;
    if (oftp->refCount > 0) {
        return 0;
    }

    printf("file removed from running->fd\n");

    mip = oftp->mptr;
    iput(mip);
    free(oftp);
    printf("close_file end\n");
    return 0;
}

int mylseek(int fd, int position) {
    printf("lseek\n");
    OFT *oftp = running->fd[fd];
    int originalPos = oftp->offset;
    printf("After intial\n");

    if ((position < 0) || (position > oftp->mptr->INODE.i_size)) {
        printf("ERROR: postion %d is not within file\n", position);
        return;
    }
    oftp->offset = position;

    printf("lseek end\n");
    return originalPos;
}

int mypfd() {
    printf(" fd   mode    offset   INODE\n");
    printf("----  ----    ------  -------\n");
    for (int i = 0; i < 12; i++) {
        printf(" %d    ", i);
        if (running->fd[i] != NULL) {
            //printf("mode = %d ", running->fd[i]->mode);
            switch(running->fd[i]->mode){
                case 0: printf("READ");
                break;
                case 1: printf("WRITE");
                break;
                case 2: printf("RE/WR");
                break;
                case 3: printf("APPEN");
                break;
                default: printf("ERROR");
                break; 
            }
            printf("    %d ", running->fd[i]->offset);
            printf(" [%d, %d]\n", running->fd[i]->mptr->dev, running->fd[i]->mptr->ino);
        } else {
            printf("NULL\n");
        }
    }
    printf("-----------------------------\n");
}

int mydup (int fd) {
    printf("dup\n");
    int i = 0;

    // verify fd is within range
    if (fd > 16 || fd < 0) {
        printf("ERROR: fd %d is out of range\n", fd);
        return;
    }

    // verify fd is an opened descriptor
    if (running->fd[fd] == NULL) {
        printf("ERROR: fd %d is not an OFT entry\n", fd);
        return;
    }

    while ((running->fd[i] != NULL) && (i < 16)) {
        i++;
    }

    // copy fd into new fd
    running->fd[i] = running->fd[fd];
    running->fd[fd]->refCount++;

    printf("dup end\n");
}

int mydup2(int fd, int gd) {
    printf("dup2\n");

    // verify fd is within range
    if (fd > 16 || fd < 0) {
        printf("ERROR: fd %d is out of range\n", fd);
        return;
    }

    // verify fd is within range
    if (gd > 16 || gd < 0) {
        printf("ERROR: gd %d is out of range\n", gd);
        return;
    }

    // verify fd is an opened descriptor
    if (running->fd[fd] == NULL) {
        printf("ERROR: fd %d is not an OFT entry\n", fd);
        return;
    }

    // verify fd is an opened descriptor
    if (running->fd[gd] == NULL) {
        printf("ERROR: gd %d is not an OFT entry\n", gd);
        return;
    }

    myclose_file(gd);

    // copy fd into new fd
    running->fd[gd] = running->fd[fd];
    running->fd[fd]->refCount++;

    printf("dup2 end\n");
}