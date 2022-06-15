#include <stdlib.h>
#include <error.h>
// #include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include "fs.h"
#include "defs.h"

extern char *img;
extern struct super_block *sb;

int init() {
	img = (char *)calloc(1024 * BSIZE, 1);
	sb = (struct super_block *)img;
	sb->magic_num = 20010315;
	sb->size = 1024 * 1024;
	sb->nblocks = 1024;
	sb->ninodes = 100;
	sb->inodestart = 1;
	sb->bmapstart = 7;
	sb->version = 1;

	
	char *bmap = img + 7 * BSIZE;
	*bmap = *bmap | (-1);
	// struct inode *root_inode = (struct inode *)(img + BSIZE + sizeof(struct inode));
	struct inode *root_inode = iget(ROOTNO);
	root_inode->type = _DIRE;
	// root_inode->data[0] = 8; // add_file will do this
	// add_file(ROOTNO, ".", _DIRE);
	// add_file(ROOTNO, "..", _DIRE);
	mkdir(0, "/");
	
	int fd = open("img", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);	
	if (fd == -1) {
		perror("can not create img");
		exit(-1);
	}
	write(fd, img, 1024 * BSIZE);
	show_info();
	return 0;
}
