#define BSIZE 1024
#define ROOTNO 1 // root i-number


typedef unsigned int uint;

struct super_block {
	uint magic_num;	// just for fum
	uint size;		// size(in bytes)
	uint nblocks;	// number of blocks
	uint ninodes;	// number of inodes
	uint inodestart;
	uint bmapstart;	// start block of bit map block
	uint version;
	// no log
};

#define NDIRECT 12
#define _FILE 1
#define _DIRE 2

// The file system is a very simply and tiny file system
// so it only supports text file and direcory. 
// Do not support link files and devices
struct inode {
	uint type;		// file or directory
	uint size;		// the size of the file
	uint data[NDIRECT + 1];
};

#define MAXSIZE 20

// directory
struct dirent {
	int inum;			// inode number
	char name[MAXSIZE];
};

// int add_file(int , char *, uint);
int mkdir(int, char *);
int find_file(int, char *);
struct inode *iget(uint);
struct inode *ialloc(uint);
char *bget(uint);
int path_parse(int, char **);
char *bget(uint);
char *balloc();
int add_entry(struct inode *, int, char *);
void bfree(int);
