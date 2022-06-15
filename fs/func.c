#include <stdio.h>
#include <string.h>
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
			break;
		char *block = bget(ip->data[i]);
		struct dirent *dir = (struct dirent *)block;
		int size = 0;
		while (size < BSIZE && dir->inum) {
			struct inode *inner_ip = iget(dir->inum);
			if (inner_ip->type == _DIRE)	{
				if (strcmp(dir->name, ".") == 0 || strcmp(dir->name, "..") == 0)
					printf("%s\n", dir->name);
				else 
					printf("%s/\n", dir->name);
			}
			else
				printf("%s\n", dir->name);
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
		while (size < BSIZE && dir->inum) {
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
					staff = BSIZE < size ? BSIZE: size; // 表示要转移的大小
					// printf("staff is %d\n", staff);
					memmove(block, content, staff);
					// 将该页放入ip->data中
					int i;
					for (i = 0; i < NDIRECT; i ++) {
						if (ip->data[i] == 0) {
							// printf("NO: %d\n", i);
							ip->size += staff;
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
				// 更新大小，更新目录项
				// printf("inum is %d\n", inum);
				ip_parent = iget(inum);
				ip_parent->size += ip->size;
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
	// printf("here!\n");
	// printf("parent is %d\n", find_file(inum, ".."));
	// ip_parent = iget(find_file(inum, ".."));
	ip_parent = iget(inum);
	// printf("before ip_parent->size is %d\n", ip_parent->size);
	ip_parent->size -= ip->size; // 暂时删除size,size更新后会加回去
	// printf("after ip_parent->size is %d\n", ip_parent->size);
	int bnum = ip->data[ip->size / BSIZE]; // 找到上次写到最后的块号bnum
	block = bget(bnum);
	int written = ip->size % BSIZE;
	int left = BSIZE - written;
	staff = left < size ? left : size;
	memmove(block + written, content, staff); // 拷贝内容到未满的页面
	size -= staff;
	ip->size += staff;
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
				ip->size += staff;
				ip->data[i] = (block - img) / BSIZE;
				break;
			}
		}
		if (i >= NDIRECT) {
			// 到这里说明ip->data没有空间了, 应返回错误
			// 同时应该修改ip->size
			fprintf(stderr, "file out of memory\n");
			break;
		}
		size -= staff;
	}
	ip_parent->size += ip->size;
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
