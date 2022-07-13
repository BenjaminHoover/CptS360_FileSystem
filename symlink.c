#include "link_unlink.c"

int readlink(char *pathname, char *buffer) {
    printf("inside readlink, pathaname = %s\n", pathname);
    int ino;
    MINODE *mip;
    
    ino = getino(pathname, &dev);
    mip = iget(dev, ino);
    printf("ino = %d, mip->ino = %d\n", ino, mip->ino);

    // mip is LNK
    if ((mip->INODE.i_mode & 0xF000 ) != 0xA000) {
        printf("ERROR: %s is a not a valid link file\n", pathname);
        return;
    }

    //char sbuf[mip->INODE.i_size];
    strncpy(buffer, mip->INODE.i_block, mip->INODE.i_size);
    buffer[mip->INODE.i_size] = 0;
    return mip->INODE.i_size;
    printf("end readlink\n");
}

int readlinkhelp(char *pathname){
    printf("readlinkhelp pathname : %s\n", pathname);
    char *buffer[60];
    int size;

    printf("before readlink\n");
    size = readlink(pathname, buffer);

    printf("readlink return: %s, size: %d\n", buffer, size);
}

int symlink(char *oldFileName, char *newFileName) {
    printf("inside symlink, old = %s, new = %s\n", oldFileName, newFileName);
    //INODE mipINODE= getINODE(oldFileName);
    char parent[128], child[128], sbuf[BLKSIZE];
    int pino, oino, nino;
    MINODE *pmip, *omip, *nmip;

    oino = getino(oldFileName, &dev);
    omip = iget(dev, oino);
    printf("oino = %d, omip->ino = %d\n", oino, omip->ino);

    // omip is not a dir
    if (!(S_ISREG(omip->INODE.i_mode)) && ((omip->INODE.i_mode & 0xF000 ) != 0xA000)) {
        printf("ERROR: %s is a not a valid file or link file\n", oldFileName);
        return;
    }

    int fail = 1;
    fail = creat_file(newFileName);
    if (fail == 0){
        return;
    }
    nino = getino(newFileName, &dev);
    nmip = iget(dev, nino);
    // newFileName exists
    if (/*getino(newFileName, &dev)*/ nino == 0) {
        printf("ERROR: %s wasn't created\n", newFileName);
        return;
    }

    nmip->INODE.i_mode = 0xA000; // change to LNK
    
    strncpy(nmip->INODE.i_block, oldFileName, strlen(oldFileName));
    /*get_block(nmip->dev, nmip->INODE.i_block[0], sbuf);
    strncpy(sbuf, oldFileName, strlen(oldFileName));
    put_block(nmip->dev, nmip->INODE.i_block[0], sbuf);*/
    printf("putting old file name into link block\n");

    nmip->INODE.i_size = strlen(oldFileName);
    nmip->dirty = 1;
    iput(nmip);

    printf("end symlink\n");
    readlinkhelp(newFileName);
}