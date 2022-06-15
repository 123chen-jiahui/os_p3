#include "fs.h"
#include <string.h>
#include<stdio.h>
extern char *img;
extern struct super_block *sb;

// 显示文件系统的信息
void show_info() {
    printf("load the image file successfully\n");
    printf("file system version: %u.0\n", sb->version);
    printf("total size %uKB\n", sb->size / 1024);
    printf("%u inodes\n", sb->ninodes);
    printf("%u blocks\n", sb->nblocks);	
}

// 给定一个inum，返回struct inode *
struct inode *iget(uint inum) {
	return (struct inode *)(img + BSIZE + inum * sizeof(struct inode));
}

// 给定一个bnum，返回char *
char *bget(uint bnum) {
	return (img + bnum * BSIZE);
}

// 分配一个空闲的inode
struct inode *ialloc(uint type) {
	struct inode *ip = (struct inode *)(img + BSIZE + 2 * sizeof(struct inode));	
	while (ip->type != 0) {
		ip ++;
	}
	ip->type = type;
	ip->size = 0;
	memset(ip->data, 0, sizeof(uint) * NDIRECT);
	return ip;
}

// 分配新页
char *balloc() {
	// 查询bitmap
	char *bitmap = img + sb->bmapstart * BSIZE;
	int times = 0;
	int single = 0;
	for (; times < BSIZE; times ++) {
		char *byte = bitmap + times;
		for (int single = 0; single < 8; single ++) {
			int bit = (*byte >> single) & 1;
			// int bit = ((*byte >> (7 - single)) & 1);
			if (bit == 0) { // 有空闲的
				/******debug*******/
				// printf("single is %d. times is %d.\n", single, times);
				/******************/
				// *byte = *byte | (1 << (7 - single));
				*byte = *byte | (1 << single);
				// 计算是哪个block
				// int bnum = times * 8 + single + sb->bmapstart + 1;
				int bnum = times * 8 + single;
				return (img + BSIZE * bnum);
			}
		}
	}
	return NULL; // fail
}

// 回收指定bnum的block
// 将块内所有字节置为0，并在bitmap中将对应位置0
void bfree(int bnum) {
	char *block = bget(bnum);
	memset(block, 0, BSIZE);
		
	char *bitmap = img + sb->bmapstart * BSIZE;
	char *byte = bitmap + bnum / 8;
	int left = bnum % 8;
	int item = 0;
	for (int i = 0; i < 8; i ++) {
		if (i != left)
			item += (1 << i);
	}
	*byte &= item;
}

/**************这个函数已被废弃********************
// 插入一个目录
int add_file(int inum, char *name, uint type) {
	struct inode *ip = iget(inum);
	// 只有目录中才能添加文件
	if (ip->type != _DIRE) 
		return 0;
	
	// 枚举直接索引，找到第一个不为零的数
	int index;
	char *block;
	struct dirent *dir_entry;
	struct inode *new_ip;
	int flag = 0; // 没找到
	for (index = NDIRECT - 1; index >= 0 && flag == 0; index --) {
		if (ip->data[index] != 0) {
			block = bget(ip->data[index]);
			dir_entry = (struct dirent *)block;
			int size = 0;
			while (dir_entry->inum != 0 && size < BSIZE) {
				dir_entry ++;
				size += sizeof(struct dirent);
			}
			// 该页目录项满了
			if (size >= BSIZE) {
				index ++;	
				break;				
			}
			// 没满
			// 如果是.目录
			if ()
			new_ip = ialloc(type); // 申请新的inode
			// 将inum填入entry
			dir_entry->inum = new_ip - (struct inode *)(img + BSIZE);
			// 将文件名写入entry
			strncpy(dir_entry->name, name, MAXSIZE - 1);
			flag = 1;
			break;
		}
	}	
	if (flag == 0) {
		if (index >= NDIRECT)
			return 0;
		else {
			if (index < 0)
				index = 0;
			block = balloc();
			ip->data[index] = (block - img) / BSIZE;
			dir_entry = (struct dirent *)block;
			new_ip = ialloc(type);
			dir_entry->inum = new_ip - (struct inode *)(img + BSIZE);
			strncpy(dir_entry->name, name, MAXSIZE - 1);
			flag = 1;
		}
	}
	return 1;
}
************************************************/

// 在inum中找name
int find_file(int inum, char *name) {
	struct inode *ip = iget(inum);
	for (int i = 0; i < NDIRECT; i ++) {
		if (ip->data[i] == 0)
			break;
		char *block = bget(ip->data[i]);
		struct dirent *dir = (struct dirent *)block;
		int size = 0;
        while (size < BSIZE && dir->inum) {
			if (strcmp(dir->name, name) == 0)
				return dir->inum;
            size += sizeof(struct dirent);
            dir ++;
        }
	}	
	return 0;
}

/*
int find_parent_dir(int inum) {
	for (int i = 1; i < sb->ninodes; i ++) { // 枚举所有inode
		struct inode *ip = iget(i);
		for (int j = 0; j < NDIRECT; j ++) {
			if (ip->data[i] == 0)	
		}	
	}
}
*/


// 在ip中添加目录项(inum, name)
int add_entry(struct inode *ip, int inum, char *name) {
	if (ip == NULL || ip->type != _DIRE)
		return 0;

	int index;
	char *block;
	struct dirent *dir_entry;
	int flag = 0; // 没找到
	for (index = NDIRECT - 1; index >= 0 && flag == 0; index --) {
		if (ip->data[index] != 0) {
			block = bget(ip->data[index]);
			dir_entry = (struct dirent *)block;
			int size = 0;
			while (dir_entry->inum != 0 && size < BSIZE) {
				dir_entry ++;
				size += sizeof(struct dirent);
			}
			// 该页目录项满了
			if (size >= BSIZE) {
				index ++;	
				break;				
			}
			// 没满
			// 将inum填入entry
			dir_entry->inum = inum;
			// 将文件名写入entry
			strncpy(dir_entry->name, name, MAXSIZE - 1);
			flag = 1;
			break;
		}
	}	
	if (flag == 0) {
		if (index >= NDIRECT)
			return 0;
		else {
			if (index < 0)
				index = 0;
			block = balloc();
			ip->data[index] = (block - img) / BSIZE;
			dir_entry = (struct dirent *)block;
			dir_entry->inum = inum;
			strncpy(dir_entry->name, name, MAXSIZE - 1);
			flag = 1;
		}
	}
	return 1;
}

// 在iparent中创建一个名为name的目录
// 自动创建.和..
// iparent为0表示创建根目录
// 逻辑：
// 如果给的iparent是0，说明是在img初始化的时候被调用的，目的
// 是要建立根目录，由于根目录的索引节点在inode#1，所以直接对
// 该索引节点进行操作：将.和..放入目录项，都指向根目录
// 如果iparent非零，说明是正常的调用mkdir命令，首先申请一个
// 索引节点，然后将.和..放入该索引节点的目录项内，最后在父目录
// 中将放入新创建的目录
int mkdir(int iparent, char *name) {
	struct inode *ip_child, *ip_parent;
	if (iparent == 0) {
		ip_child = (struct inode *)(img + BSIZE) + 1;	
		ip_child->type = _DIRE;
		add_entry(ip_child, ROOTNO, ".");
		add_entry(ip_child, ROOTNO, "..");
	} else {
		ip_child = ialloc(_DIRE);
		// ip_child->type = _DIRE;
		int inum = ip_child - (struct inode *)(img + BSIZE);
		add_entry(ip_child, inum, ".");
		add_entry(ip_child, iparent, "..");
		ip_parent = iget(iparent);
		add_entry(ip_parent, inum, name);
	}
}

// 路径解析
// path_parse(inum, "aa/bb/cc.txt")
// path变为bb/cc.txt
// 返回aa的inum
int path_parse(int inum, char **path) {
	// printf("*path is %s\n", *path);
	// printf("%c\n", **path);
	if (**path == '/') {
		while (**path == '/')
			(*path) ++;	
		return ROOTNO;	
	} else {
		char prefix[20];
		int i = 0;
		while (**path != '/' && **path != '\0') {
			prefix[i ++] = **path;
			(*path) ++;
		}
		if (**path == '/')
			(*path) ++;
		prefix[i] = '\0';
		// printf("prefix is %s\n", prefix);
		return find_file(inum, prefix);
	}
}

