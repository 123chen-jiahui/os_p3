#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/types.h>
// #include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <error.h>
#include "fs.h"
#include "defs.h"

int cwd;
char *img;
struct super_block *sb;

void load_img(int fd) {
	img = (char *)mmap(NULL, 1024 * BSIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (img == MAP_FAILED) {
		perror("mmap");
		exit(-1);
	}
	sb = (struct super_block *)img;
	show_info();
	/*
	printf("load the image file successfully\n");
	printf("file system version: %u.0\n", sb->version);
	printf("total size %uKB\n", sb->size / 1024);
	printf("%u inodes\n", sb->ninodes);
	printf("%u blocks\n", sb->nblocks);
	*/
}

int main() {
	int fd = open("img", O_RDWR);
	if (fd == -1) {
		printf("can not open img\n");
		init();	
		fd = open("img", O_RDWR);
	}
	load_img(fd);		
	cwd = ROOTNO;
	/*
    int shmid_buffer;
    int *shmaddr_buffer;
    shmaddr_buffer = (int *)connect(&shmid_buffer, 1, 4);
	*shmaddr_buffer = ROOTNO;
	*/

	while (1) {
		printf("$ ");
		char command[50];
		char element[4][50];
		memset(element, 0, sizeof(element));
		int pieces = 0;
		int count = 0;
		int flag = 1; // 如果刚刚读到过空格，则置1
		fgets(command, 50, stdin);
		command[strlen(command) - 1] = '\0';
		// 将command中的字符串拆开
		for (int i = 0; command[i] != '\0'; i ++) {
			if (command[i] == ' ' && flag == 0) {
				flag = 1;
				count = 0;
				pieces ++;
			} else if (command[i] != ' ') {
				flag = 0;
				element[pieces][count ++] = command[i];
			}
		}
		if (flag == 0)
			pieces ++;
		/*************debug****************/
		// printf("element[0] is %s\n", element[0]);
		// printf("pieces is %d\n", pieces);
		// for (int i = 0; i < pieces; i ++)
			// printf("%s\n", element[i]);
		/*********************************/
		if (strcmp(element[0], "quit") == 0) {
			break;
		} else if (strcmp(element[0], "ls") == 0) {
			ls();
		} else if (strcmp(element[0], "pwd") == 0) {
			// printf("cwd is %d\n", cwd);
			pwd(cwd);
			printf("\n");
		} else if (strcmp(element[0], "mkdir") == 0) {
			mkdir(cwd, element[1]);
		} else if (strcmp(element[0], "cd") == 0) {
			cd(element[1]);
		} else if (strcmp(element[0], "echo") == 0) {
			// element[1]是文件内容
			// element[2]是重定向>
			// element[3]是路径名
			// 这里应该检查element[2]是否是>
			if (strcmp(element[2], ">") == 0) {
				echo(element[1], element[3]);
			}
		} else if (strcmp(element[0], "cat") == 0) {
			cat(element[1]);
		}
		/*
		if (strcmp(command, "quit") == 0) {
			break;
		} else if (strcmp(command, "ls") == 0) {
			ls();	
		} else {
			printf("invalid command:%s\n", command);
		}
		*/
	}
		
	/*
	while (1) {
		fprintf(stderr, "$ ");
		char command[20];
		fgets(command, 20, stdin);
		command[strlen(command) - 1] = '\0';
		// printf("command is %s len is %d\n", command, strlen(command));
		if (strcmp("quit", command) == 0)
			break;
		int pid = fork();
		if (pid == 0) { // child
			char path[50];
			sprintf(path, "./%s", command);
			execlp(path, command, NULL);
			fprintf(stderr, "invalid command:%s\n", command);
			exit(0);
		} else { // parent
			wait(NULL);	
		}	
	}
	*/
	return 0;
}
