#include "read_cat.c"

int mywrite_file(char *ifd, char *iline) {
    char line[BLKSIZE], sfd[5];
    int fd, nbytes;

    strcpy(sfd, ifd);
    strcpy(line, iline);

    if ((sfd[0] == '\0') || (line[0] == '\0')) {
        // get fd from user
        printf("Enter a fd: ");
        gets(sfd);
        printf("fd = %s\n", sfd);

        // get string from user
        printf("Enter a line: ");
        gets(line);
        printf("line = %s\n", line);
    }
    printf("fd = %s line = %s\n", sfd, line);
    
    if ((sfd == NULL) || (line == NULL)) {
        printf("ERROR: invalid inputs\n");
        return;
    }
    printf("inputs valid\n");

    fd = atoi(sfd);
    // verify fd is within range
    if (fd > 12 || fd < 0) {
        printf("ERROR: fd %d is out of range\n", fd);
        return;
    }

    if (running->fd[fd] == NULL) {
        printf("ERROR: fd %d is not opened\n", fd);
        return;
    }
    
    // check if file is already opened for rd, rw
    if ((running->fd[fd]->mode == 1) || (running->fd[fd]->mode == 2)) {
        nbytes = strlen(line);
        if (nbytes <= BLKSIZE) {
            strcat(line, "\n");
            return( mywrite(fd, line, nbytes) );
        } else {
            line[BLKSIZE] = 0;
            strcat(line, "\n");
            return( mywrite(fd, line, BLKSIZE) );
        }
    }
    printf("ERROR: file %d is not opened for writing\n", fd);
}

int mywrite(int fd, char buf[], int nbytes) {
    printf("Inside mywrite\n");
    OFT *oftp = running->fd[fd];
    MINODE *mip = oftp->mptr;
    char *cq = buf;
    int blk, tblk, ibuf[256], tbuf[BLKSIZE];
    char wbuf[BLKSIZE];

    while (nbytes > 0) {
        int lbk = oftp->offset / BLKSIZE;
        int startByte = oftp->offset % BLKSIZE;
        printf("lbk = %d, startByte = %d\n", lbk, startByte);
        printf("ino = %d\n", mip->ino);

        if (lbk < 12) { // direct block
            if ((mip->INODE.i_block[lbk] == 0) || (lbk == 0)) { // data block doesn't exist
                printf("Allocating direct data block %d\n", lbk);
                mip->INODE.i_block[lbk] = balloc(mip->dev); 
                printf("Before bzero\n");
                bzero(tbuf, BLKSIZE);
                printf("post bzero\n");
                put_block(mip->dev, mip->INODE.i_block[lbk], tbuf);
            }
            printf("direct data block %d exists\n", lbk);
            blk = mip->INODE.i_block[lbk];
        } else if (lbk >= 12 && lbk < 256 + 12) { // indirect blocks
            if (mip->INODE.i_block[12] == 0) {
                printf("Allocating indirect data block =====---=====\n");
                mip->INODE.i_block[12] = balloc(mip->dev);
                if (mip->INODE.i_block[12] == 0) { // no space on block, allocate a new block
                    printf("ERROR: cannot allocate a new block\n");
                    return -1;
                }
                printf("Before bzero\n");
                bzero(tbuf, BLKSIZE);
                printf("post bzero\n");
                put_block(mip->dev, mip->INODE.i_block[12], tbuf);
            } 
            printf("indirect data block exists\n");
            get_block(mip->dev, mip->INODE.i_block[12], ibuf);
            blk = ibuf[lbk - 12];
            if (blk == 0) {
                printf("Allocating blk\n");
                blk = balloc(mip->dev);
                if (blk == 0) { // no space on block, allocate a new block
                    printf("ERROR: cannot allocate a new block\n");
                    return -1;
                }
                printf("Before bzero\n");
                bzero(tbuf, BLKSIZE);
                printf("post bzero\n");
                put_block(mip->dev, blk, tbuf);
                ibuf[lbk - 12] = blk;
                put_block(mip->dev, mip->INODE.i_block[12], ibuf);
            }

        } else { // double indirect blocks
            if (mip->INODE.i_block[13] == 0) {
                printf("Allocating double indirect data block =====---=====\n");
                mip->INODE.i_block[13] = balloc(mip->dev);
                if (mip->INODE.i_block[13] == 0) { // no space on block, allocate a new block
                    printf("ERROR: cannot allocate a new block\n");
                    return -1;
                }
                printf("Before bzero\n");
                bzero(tbuf, BLKSIZE);
                printf("post bzero\n");
                put_block(mip->dev, mip->INODE.i_block[13], tbuf);
                //printf("nblocks = %d\n", nblocks);
                //getchar();
                //mip->INODE.i_block[13] = 0;
            }
            printf("double indirect data block exists\n");
            get_block(mip->dev, mip->INODE.i_block[13], ibuf);
            tblk = ibuf[lbk - (256 + 12)/*(int)(lbk / (256 + 12)) - 1*/];
            if (tblk == 0) {
                printf("allocating tblk\n");
                tblk = balloc(mip->dev);
                if (tblk == 0) { // no space on block, allocate a new block
                    printf("ERROR: cannot allocate a new block\n");
                    return -1;
                }
                printf("Before bzero\n");
                bzero(tbuf, BLKSIZE);
                printf("post bzero\n");
                put_block(mip->dev, tblk, tbuf);
                ibuf[(int)(lbk / (256 + 12)) - 1] = tblk;
                put_block(mip->dev, mip->INODE.i_block[12], ibuf);
            }
            printf("tblk = %d\n", tblk);
            get_block(mip->dev, tblk, ibuf);
            blk = ibuf[lbk - (256 + 12)];
            if (blk == 0) {
                printf("allocating blk\n");
                blk = balloc(mip->dev);
                if (blk == 0) { // no space on block, allocate a new block
                    printf("ERROR: cannot allocate a new block\n");
                    return -1;
                }
                printf("Before bzero\n");
                bzero(tbuf, BLKSIZE);
                printf("post bzero\n");
                put_block(mip->dev, blk, tbuf);
                ibuf[lbk - (256 + 12)] = tblk;
                put_block(mip->dev, tblk, ibuf);
            }
            printf("blk = %d\n", blk);
            /*get_block(mip->dev, mip->INODE.i_block[13], ibuf);
            tblk = ibuf[(int)(lbk / (256 + 12)) - 1];
            get_block(mip->dev, tblk, ibuf);
            blk = ibuf[lbk - (256 + 12)];*/

        }
        printf("blocks aquired\n");
        get_block(mip->dev, blk, wbuf);
        char *cp = wbuf + startByte;
        int remain = BLKSIZE - startByte;

        /*while (remain > 0) { // optimize like read
            *cp++ = *cq++;
            nbytes--; remain--;
            oftp->offset++;
            if (oftp->offset > oftp->mptr->INODE.i_size) {
                mip->INODE.i_size++;
            }
            if (nbytes <= 0) {
                break;
            }
        }*/
        if (nbytes < remain) {
            oftp->offset = oftp->offset + nbytes; 
            if (oftp->offset > oftp->mptr->INODE.i_size) {
                mip->INODE.i_size = mip->INODE.i_size + nbytes;
            }
            strncpy(cp, cq, nbytes);
            nbytes = 0; 
            remain = remain - nbytes;
        } else {
            oftp->offset = oftp->offset + remain; 
            if (oftp->offset > oftp->mptr->INODE.i_size) {
                mip->INODE.i_size = mip->INODE.i_size + remain;
            }
            strncpy(cp, cq, remain);
            nbytes = nbytes - remain; 
            remain = 0;
        }

        put_block(mip->dev, blk, wbuf);
    }
    mip->dirty = 1;
    printf("wrote %d char into file descriptor fd = %d\n", nbytes, fd);
    return nbytes;
}

int mycp(char *src, char *dst) {
    int fd = myopen_file(src, "0");
    int gd = myopen_file(dst, "1");
    mypfd();

    if (fd == -1) {
        printf("%s is not a vaild file\n", src);
        return;
    }
        if (gd == -1) {
        printf("%s is not a vaild file, creating new file\n", gd);
        creat_file(dst);
        gd = myopen_file(dst, "1");
    }

    while (n=myread(fd, buf, BLKSIZE)) {
        //printf("%s", buf);
        mywrite(gd, buf, n); 
    }
    myclose_file(fd);
    myclose_file(gd);
}

int mymv(char *src, char *dest) {
    int sino = getino(src, &dev);
    if (!sino) {
        printf("ERROR: %s does not exist\n", src);
        return;
    }
    // src same dev as dest

    if (1) { // same dev
        link(src, dest);
        unlink(src);
    } else { // diff dev

    }
}
