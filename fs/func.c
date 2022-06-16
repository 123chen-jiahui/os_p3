#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "fs.h"
extern char *img;
extern struct super_block *sb;
extern int cwd;

void ls() {
	struct inode *ip = iget(cwd);
	// printf("file type:%d\n", ip->type);
	// printf("cwd is %d\n", cwd);
	for (int i = 0; i < NDIRECT; i ++) {
		// printf("ip->data[%d] is %d\n", i, ip->data[i]);
		if (ip->data[i] == 0)
			continue;
		char *block = bget(ip->data[i]);
		struct dirent *dir = (struct dirent *)block;
		int size = 0;
		while (size < BSIZE) {
			if (dir->inum != 0) {
				struct inode *inner_ip = iget(dir->inum);
				if (inner_ip->type == _DIRE)	{
					if (strcmp(dir->name, ".") == 0 || strcmp(dir->name, "..") == 0)
						printf("%-20s", dir->name);
					else  {
						char filename[23];
						memset(filename, 0, sizeof(filename));
						sprintf(filename, "%s/", dir->name);
						printf("%-20s", filename);
					}
				}
				else
					printf("%-20s", dir->name);
				printf("%dbytes\n", inner_ip->size);
			}
			size += sizeof(struct dirent);
			dir ++;
		}
	}
}

void cd(char *dir_name) {
	int inum = cwd;
	int parsed_inum;
	while (*dir_name != '\0') {
		// printf("inum is %d\n", inum);
		// printf("dir_name is %s\n", dir_name);
		parsed_inum = path_parse(inum, &dir_name);
		if (parsed_inum == 0) {
			fprintf(stderr, "path doesn't exist\n");
			return;
		}
		inum = parsed_inum;
	}
	cwd = inum;
}

/*
void cd(int inum, char *dir_name) {
	if (*dir_name == '\0') { // 路径解析完毕
		cwd = inum;
		return;		
	}
	int ichild;
	if (*dir_name == '/') {
		ichild = ROOTNO;
		while (*dir_name == '/')
			dir_name ++;
		inum = ROOTNO;
		cd(inum, dir_name);	
	} else {
		// printf("before dir_name is %s\n", dir_name);
		// printf("dir_name[1] is %d\n", (int)dir_name[1]);
		char prefix[10];
		int i = 0;
		while (*dir_name != '/' && *dir_name != '\0') {
			prefix[i ++] = *dir_name;
			dir_name ++;
		}
		if (*dir_name == '/')
			dir_name ++;
		prefix[i] = '\0';

		ichild = find_file(inum, prefix);
		if (ichild == 0)
			return;
		cd(ichild, dir_name);
	}
}
*/

void pwd(int inum) {
	// printf("inum is %d\n", inum);
	assert(inum != 0);
	if (inum == 1) {
		printf("/");
		return;
	}
	int iparent = find_file(inum, "..");
	// printf("iparent is %d\n", iparent);
	pwd(iparent);	
	struct inode *ip = iget(iparent);
	for (int i = 0; i < NDIRECT; i ++) {
		if (ip->data[i] == 0)
			continue;
		char *block = bget(ip->data[i]);
		struct dirent *dir = (struct dirent *)block;
		int size = 0;
		while (size < BSIZE) {
			if (dir->inum == inum) {
				printf("%s/", dir->name);
				return;
			}
			size += sizeof(struct dirent);
			dir ++;
		}
	}
}

void echo(char *content, char *path) {
	// printf("path is %s\n", path);
	int inum = cwd;
	int parsed_inum;
	char backup[20]; // 暂存path
	struct inode *ip, *ip_parent;
	int size = strlen(content) + 1;
	// printf("content is %s, size if %ld\n", content, sizeof(content));
	int staff = 0; // 表示本次拷贝的字节数
	int grow = 0; // 用于统计增加的字节数
	char *block; // 用于表示块
	// char *start = content; // content的副本
	while (1) {
		strcpy(backup, path);
		parsed_inum = path_parse(inum, &path);
		// printf("parsed_inum is %d\n", parsed_inum);
		if (parsed_inum == 0) { // 文件不存在
			if (*path != '\0') { // 不是path的最后一项
				fprintf(stderr, "path doesn't exist\n");
				return;
			} else {
				// printf("hi\n");
				// printf("%s\n", backup);
				// 创建inode
				ip = ialloc(_FILE);
				// 修改ip->size
				// ip->size = size;
				// 分配页以存储数据
				while (size > 0) {
					content += staff;
					block = balloc();
					// printf("block alloc: %d\n", (block - img) / BSIZE);
					staff = BSIZE < size ? BSIZE: size; // 表示要转移的大小
					// printf("staff is %d\n", staff);
					memmove(block, content, staff);
					// 将该页放入ip->data中
					int i;
					for (i = 0; i < NDIRECT; i ++) {
						if (ip->data[i] == 0) {
							// printf("NO: %d\n", i);
							// ip->size += staff;
							grow += staff;
							ip->data[i] = (block - img) / BSIZE;
							break;
						}
					}
					if (i >= NDIRECT) {
						// 到这里说明ip->data没有空间了, 应返回错误
						fprintf(stderr, "file out of memory\n");
						break;
						// return;
					}
					size -= staff;
				}
				// 修改父目录的信息, inum就是父目录
				// (注意，这里应该递归地更新大小)更新大小，更新目录项
				// printf("inum is %d\n", inum);
				ip_parent = iget(inum);
				ip->size += grow;
				update_size(ip_parent, grow);
				// ip_parent->size += ip->size;
				add_entry(ip_parent, ip - (struct inode *)(img + BSIZE), backup);
				// printf("add ok? %d\n", add_entry(ip_parent, ip - (struct inode *)(img + BSIZE), backup));
				return;
			}
		}
		if (*path == '\0')
			break;
		inum = parsed_inum;
	}
	
	// 文件已经存在，则以追加的方式写文件（续写）
	// 此时的inum是被写文件的inode number
	// ip_parent是被写文件的父目录
	
	// 文件类型不能是目录
	// printf("inum is %d\n", inum);
	ip = iget(parsed_inum);
	if (ip->type == _DIRE) {
		fprintf(stderr, "can not write a directory\n");
		return;
	}
	ip_parent = iget(inum);
	// printf("before ip_parent->size is %d\n", ip_parent->size);
	// ip_parent->size -= ip->size; // 暂时删除size,size更新后会加回去
	// printf("after ip_parent->size is %d\n", ip_parent->size);
	int bnum = ip->data[ip->size / BSIZE]; // 找到上次写到最后的块号bnum
	block = bget(bnum);
	int written = ip->size % BSIZE;
	int left = BSIZE - written;
	staff = left < size ? left : size;
	memmove(block + written, content, staff); // 拷贝内容到未满的页面
	size -= staff;
	// ip->size += staff;
	grow += staff;
	// written += staff;
	while (size) { // 如果还有内容未拷贝，要分配新块
		block = balloc();
		content += staff;
		staff = size < BSIZE ? size : BSIZE;
		memmove(block, content, staff);
		// 将该块加入inode中
		int i;
		for (i = 0; i < NDIRECT; i ++) {
			if (ip->data[i] == 0) {
				// ip->size += staff;
				grow += staff;
				ip->data[i] = (block - img) / BSIZE;
				break;
			}
		}
		if (i >= NDIRECT) {
			// 到这里说明ip->data没有空间了, 应返回错误
			// 同时应该修改ip->size
			ip->size += grow;
			update_size(ip_parent, grow);
			fprintf(stderr, "file out of memory\n");
			return;
			// break;
		}
		size -= staff;
	}
	ip->size += grow;
	update_size(ip_parent, grow);
	// ip_parent->size += ip->size;
	// printf("ip_parent->size if %d\n", ip_parent->size);
	// add_entry(ip_parent, ip - (struct inode *)(img + BSIZE), backup);
}

void cat(char *path) {
	int inum = cwd;
	int parsed_inum;
	while (*path != '\0') {
		parsed_inum = path_parse(inum, &path);
		if (parsed_inum == 0) {
			fprintf(stderr, "path doesn't exist\n");
			return;
		}
		inum = parsed_inum;
	}
	struct inode *ip = iget(inum);
	if (ip->type != _FILE) {
		fprintf(stderr, "can not cat directory\n");
		return;
	}
	int size = ip->size;
	for (int i = 0; i < NDIRECT; i ++) {
		if (ip->data[i] == 0)
			break;
		char *block = bget(ip->data[i]);
		int to_print = BSIZE < size ? BSIZE : size;
		for (int j = 0; j < to_print; j ++)
			printf("%c", *(block + j));
		size -= to_print;
	}
	printf("\n");
}

// flag为0表示删除文件，为1表示删除目录
void rm(char *path, int flag) {
	int inum = cwd;
	int parsed_inum;
	char backup[20];
	while (1) {
		strcpy(backup, path);
		parsed_inum = path_parse(inum, &path);
		if (parsed_inum == 0) {
			fprintf(stderr, "path doesn't exist\n");
			return;
		}
		if (*path == '\0')
			break;
		inum = parsed_inum;
	}
	struct inode *ip = iget(parsed_inum);
	if (flag == 0 && ip->type == _DIRE) {
		fprintf(stderr, "please use rm -r to remove directory\n");
		return;
	} else if (flag == 1 && ip->type == _FILE) {
		fprintf(stderr, "please use rm to remove file\n");
		return;
	}
	/*****debug*****/
	// printf("about to delete inum: %d, parsed_inum: %d\n", inum, parsed_inum);
	/***************/
	delete_file(inum, parsed_inum);
	/*
	// 到目前为止，被删除的文件和父目录已经准备好了
	struct inode *ip = iget(parsed_inum);
	if (ip->type != _FILE) {
		fprintf(stderr, "can not remove %s: wrong type\n", backup);
		return;
	}
	struct inode *ip_parent = iget(inum);
	for (int i = 0; i < NDIRECT; i ++) {
		if (ip->data[i])
			bfree(ip->data[i]);
	}

	// 在父目录中删除该项
	for (int i = 0; i < NDIRECT; i ++) {
		if (ip_parent->data[i] == 0)
			continue;
		char *block = bget(ip_parent->data[i]);
		int at_least_one = 0;
		struct dirent *dir = (struct dirent *)block;
		int size = 0;
		while (size < BSIZE) {
			if (strcmp(dir->name, backup) == 0) {
				memset(dir, 0, sizeof(struct dirent));
			} else if (dir->inum != 0) {
				at_least_one = 1;	
			}
			size += sizeof(struct dirent);
			dir ++;
		}
		if (at_least_one == 0) { // 顺便把目录中空的页释放了
			bfree(ip_parent->data[i]);
		}
	}

	// inode全置0
	memset(ip, 0, sizeof(struct inode));
	*/
}

/*
void rm_dir(char *paht) {
	
}
*/
