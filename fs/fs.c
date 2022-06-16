#include "fs.h"
#include <string.h>
#include<stdio.h>
extern char *img;
extern struct super_block *sb;

// 显示文件系统的信息
void show_info() {
	printf("\n");
	printf("load the image file successfully\n");
	printf("==========================================================\n");
	printf("this is a tiny and ugly \"file system\" with many bugs\n");
	printf("file system info:\n");
	printf("file system version: %u.0\n", sb->version);
	printf("total size %uKB\n", sb->size / 1024);
	printf("%u inodes\n", sb->ninodes);
	printf("%u blocks\n", sb->nblocks);
	printf("==========================================================\n");
	printf("\n");
	printf("now you are in the file system, try commands as follows!\n");
	printf("\n");
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
	/******debug*****/
	// printf("inum %d\n", ip - (struct inode *)(img + BSIZE));
	/****************/
	return ip;
}

// 分配新页
char *balloc() {
	// 查询bitmap
	char *bitmap = img + sb->bmapstart * BSIZE;
	int times = 1; // 前8个（0-7）block不能被使用
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
	// printf("free bnum: %d\n", bnum);
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
	// printf("byte is %d\n", (int)*byte);
}

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


// 在ip中添加目录项(inum, name)
// 只是添加目录项，不做其他任何操作
int add_entry(struct inode *ip, int inum, char *name) {
	if (ip == NULL || ip->type != _DIRE)
		return 0;

	int mark = -1;
	int flag = 0;
	char *block;
	struct dirent *dir_entry;
	/********************************************************/
	/*该部分是多于的，因为判断文件是否已经存在由mkdir判断
	// 第一遍找，判断文件已经存在
	for (int index = 0; index < NDIRECT; index ++) {
		if (ip->data[index]) {
			block = bget(ip->data[index]);
			dir_entry = (struct dirent *)block;
			int size = 0;
			while (size < BSIZE) {
				if (dir_entry->inum != 0 && strcmp(dir_entry->name, name) == 0) {
					fprintf(stderr, "%s already exist\n", name);
					return 0;
				}
				size += sizeof(struct dirent);
				dir_entry ++;
			}
		}
	}
	*/
	// 第一遍找，在现有的块中找空闲的entry
	for (int index = 0; index < NDIRECT; index ++) {
		if (ip->data[index] == 0 && mark == -1)
			mark = index;
		if (ip->data[index] != 0) {
			block = bget(ip->data[index]);
			dir_entry = (struct dirent *)block;
			int size = 0;
			while (size < BSIZE) {
				if (dir_entry->inum == 0) { // 找到了
					// 将信息填入其中
					dir_entry->inum = inum;
					strncpy(dir_entry->name, name, MAXSIZE - 1);
					/*****debug*****/
					// printf("py is %d. inum is %d. name is %s\n", dir_entry - (struct dirent *)block, inum, name);
					/***************/
					return 1;
				}
				size += sizeof(struct dirent);
				dir_entry ++;
			}
		}
	}
	// 没找到，说明要申请新块来存目录项，也就是mark
	if (mark == -1) { // 说明ip->data[index]全不为0，无法申请
		return 0;
	} else {
		if ((block = balloc()) == NULL)
			return 0;
		dir_entry = (struct dirent *)block;
		dir_entry->inum = inum;
		strncpy(dir_entry->name, name, MAXSIZE - 1);
		ip->data[mark] = (block - img) / BSIZE;
		/*****debug*****/
		// printf("ip->data[makr] is %d\n", ip->data[mark]);
		// printf("mark is %d. inum is %d. name is %s\n", mark, inum, name);
		/***************/
		return 1;
	}
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
		int parsed_inum;
		char backup[20];
		while (1) {
			strcpy(backup, name);
			parsed_inum = path_parse(iparent, &name);
			if (parsed_inum == 0) {
				if (*name != 0) { // 中途就解析失败，说明路径错误
					fprintf(stderr, "invalid path\n");
					return 0;
				}	
				ip_child = ialloc(_DIRE);
				// ip_child->type = _DIRE;
				int inum = ip_child - (struct inode *)(img + BSIZE);
				add_entry(ip_child, inum, ".");
				add_entry(ip_child, iparent, "..");
				ip_parent = iget(iparent);
				add_entry(ip_parent, inum, backup);
				return 1;
			}
			if (*name == '\0') { // 竟能一条路走到底，则文件已存在
				fprintf(stderr, "%s already exist\n", backup);
				return 0;
			}
			iparent = parsed_inum;
		}

		/*
		ip_child = ialloc(_DIRE);
		// ip_child->type = _DIRE;
		int inum = ip_child - (struct inode *)(img + BSIZE);
		add_entry(ip_child, inum, ".");
		add_entry(ip_child, iparent, "..");
		ip_parent = iget(iparent);
		add_entry(ip_parent, inum, name);
		*/
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

// 递归地更新inum的大小
void update_size(struct inode *ip, int grow) {
	static int flag = 0; // 根目录不能被无限递归
	// ip必须是一个目录
	if (ip->type != _DIRE) {
		return;
	}
	
	int inum = ip - (struct inode *)(img + BSIZE);
	if (inum == ROOTNO) {
		if (flag == 1) {
			flag = 0;
			return;
		}
		else
			flag = 1;
	}
	ip->size += grow;
	int iparent = find_file(inum, "..");
	ip = iget(iparent);
	update_size(ip, grow);
}

// 调用函数需要保证iparent和inum的关系是父子关系
// 该函数在iparent中删除inum
void delete_file(int iparent, int inum) {
	struct inode *ip = iget(inum);
	struct inode *ip_parent = iget(iparent);
	char *block;
	struct dirent *dir;
	int size;
	// 如果被删除的是_FILE，直接删除
	// 只有_FILE被删除时，才需要更新size
	if (ip->type == _FILE) {
		for (int i = 0; i < NDIRECT; i ++) {
			if (ip->data[i]) {
				bfree(ip->data[i]);
				ip->data[i] = 0;
			}
		}
		update_size(ip_parent, -1 * ip->size);
	} else if (ip->type == _DIRE) {
		// 如果被删除的是_DIRE
		for (int i = 0; i < NDIRECT; i ++)	{
			if (ip->data[i]) { // 找到目录块
				block = bget(ip->data[i]);
				dir = (struct dirent *)block;
				size = 0;
				while (size < BSIZE) {
					if (dir->inum) {
						/*
						if (strcmp(dir->name, ".") == 0 || strcmp(dir->name, "..") == 0) {
							// 如果是当前目录和父目录，直接删除
							memset(dir, 0, sizeof(struct dirent));
						} else {
							struct inode *ip_child = iget(dir->inum);
							delete_file(inum, ip_child - (struct inode *)(img + BSIZE));
						}
						*/
						if (strcmp(dir->name, ".") && strcmp(dir->name, "..")) {
							struct inode *ip_child = iget(dir->inum);
							delete_file(inum, ip_child - (struct inode *)(img + BSIZE));

						}
					}
					size += sizeof(struct dirent);
					dir ++;
				}	
				/***testing********/
				bfree(ip->data[i]); // the bug took me 2 hours!
				ip->data[i] = 0;
				/***原本是注释的***/
			}
		}
	}

	// 在父目录中删除该项
	for (int i = 0; i < NDIRECT; i ++) {
		if (ip_parent->data[i]) {
			block = bget(ip_parent->data[i]);
			dir = (struct dirent *)block;
			size = 0;
			// int at_least_one = 0; // testing 
			while (size < BSIZE) {
				if (dir->inum == inum) {
					memset(dir, 0, sizeof(struct dirent));
				} else if (dir->inum != 0) {
					// at_least_one = 1; // testing
				}
				size += sizeof(struct dirent);
				dir ++;
			}
			/*******testing*******/
			/*
			if (at_least_one == 0) {
				bfree(ip_parent->data[i]);
				ip_parent->data[i] = 0;
			}
			*/
			/*****原本是不注释的**/
		}
	}

	// inode全置0
	memset(ip, 0, sizeof(struct inode));
}
