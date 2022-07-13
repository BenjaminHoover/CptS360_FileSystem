#include "open_close_lseek.c"

int myread_file(char *ifd, char *iline) {
    char line[128], sfd[2];
    int fd, nbytes, n = 1;

    strcpy(sfd, ifd);
    strcpy(line, iline);

    if ((sfd[0] == '\0') || (line[0] == '\0')) {
        // get fd from user
        printf("Enter a fd: ");
        gets(sfd);
        printf("fd = %s\n", sfd);

        // get string from user
        printf("Enter number of bytes: ");
        gets(line);
        printf("bytes = %s\n", line);
    }
    printf("fd = %s nbytes = %s\n", sfd, line);
    
    if ((sfd == NULL) || (line == NULL)) {
        printf("ERROR: invalid inputs\n");
        return -1;
    }
    printf("inputs valid\n");
       
    fd = atoi(sfd);
    nbytes = atoi(line);
    // verify fd is within range
    if (fd > 12 || fd < 0) {
        printf("ERROR: fd %d is out of range\n", fd);
        return;
    }

    if (running->fd[fd] == NULL) {
        printf("ERROR: fd %d is not opened\n", fd);
        return;
    }
    
    printf("========   READ BEGIN   ========\n");
    // check if file is already opened for WR, RW, or APPEND
    if ((running->fd[fd]->mode == 0) || (running->fd[fd]->mode == 2) || (running->fd[fd]->mode == 3)) {
        if (nbytes <= BLKSIZE) {
            //printf("============================= nbytes short\n");
            n = myread(fd, buf, nbytes);
            buf[n] = 0;
            printf("%s", buf);
        } else {
            while (nbytes > BLKSIZE) {
                //printf("============================= nbytes big\n");
                n = myread(fd, buf, BLKSIZE);
                buf[n] = 0;
                printf("%s", buf);
                nbytes = nbytes - BLKSIZE;
            }
            //printf("============================= nbytes small now\n");
            n = myread(fd, buf, nbytes);
            buf[n] = 0;
            printf("%s", buf);
        }
        printf("\n");
        return;
    }
    printf("ERROR: file %d is not opened for reading\n", fd);
}

int myread(int fd, char *buf, int nbytes) {
    //printf("myread entry\n");
    //printf("fd = %d, buf = %s, nbytes = %d\n", fd, buf, nbytes);
    int count = 0;
    OFT *oftp = running->fd[fd];
    MINODE *mip = oftp->mptr;
    int blk, tblk;
    int ibuf[256]; 
    char readbuf[BLKSIZE], catbuf[BLKSIZE];
    //printf("before i_size = %d\n", running->fd[fd]->mptr->INODE.i_size);
    int avil = oftp->mptr->INODE.i_size - oftp->offset; // double check fileSize
    char *cq = buf;
    strcpy(cq, "");
    //printf("before while\n");
    while (nbytes && avil) {
        //printf("nbytes = %d, avil = %d\n", nbytes, avil);
        int lbk = oftp->offset / BLKSIZE;
        int startByte = oftp->offset % BLKSIZE;
        //printf("lbk = %d, startByte = %d\n", lbk, startByte);
        
        if (lbk < 12) { // direct blocks
            //printf("direct blocks================================================================================================================\n");
            blk = mip->INODE.i_block[lbk];
        } else if (lbk >= 12 && lbk < 256 + 12) { // indirect blocks
            //printf("indirect blocks============================================================================================================\n");
            get_block(mip->dev, mip->INODE.i_block[12], ibuf);
            blk = ibuf[lbk - 12];
        } else { // double indirect blocks
            //printf("double indirect blocks======================================================================================================\n");
            get_block(mip->dev, mip->INODE.i_block[13], ibuf);
            tblk = ibuf[(int)(lbk / (256 + 12)) - 1];
            get_block(mip->dev, tblk, ibuf);
            blk = ibuf[lbk - (256 + 12)];
        }
        //printf("blk = %d\n", blk);
        get_block(mip->dev, blk, readbuf);
        //printf("block gotten\n");

        char *cp = readbuf + startByte;
        int remain = BLKSIZE - startByte; 

        /*while (remain > 0) { // optimize this
            *cq++ = *cp++;
            oftp->offset++;
            count++;
            avil--; nbytes--; remain--;
            if (nbytes <= 0 || avil <= 0) {
                break;
            }
        }*/

        if (avil < remain) {
            strncat(cq, cp, avil);
            oftp->offset = oftp->offset + avil;
            count = count + avil;
            avil = 0; 
            nbytes = nbytes - avil; 
            remain = remain - avil;
        } else if (nbytes < avil && nbytes < remain) {
            strncat(cq, cp, nbytes);
            oftp->offset = oftp->offset + nbytes;
            count = count + nbytes;
            avil = avil - nbytes; 
            nbytes = 0; 
            remain = remain - nbytes;
        } else {
            strncat(cq, cp, remain);
            oftp->offset = oftp->offset + remain;
            count = count + remain;
            avil = avil - remain; 
            nbytes = nbytes - remain; 
            remain = 0;
        }
        if (remain <= 0 && nbytes > 0) {
            strcpy(catbuf, cq);
        }
    } // end of large while loop
    //printf("myread: read %d char from file descriptor %d\n", count, fd);
    return count;
}

int mycat(char *filename) {
    printf("mycat\n");
    char mybuf[BLKSIZE], dummy = 0;
    int n, fd;

    fd = myopen_file(filename, "0");
    printf("========   CAT BEGIN   ========\n");
    while(n = myread(fd, mybuf, BLKSIZE)){
        //printf("n = %d\n", n);
        mybuf[n] = 0;
        printf("%s", mybuf);
    }
    myclose_file(fd);
    printf("mycat end\n");
}
