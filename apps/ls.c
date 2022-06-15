#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "defs.h"

int main() {
	int shmid_buffer;
	int *shaddr_buffer;
	shmaddr_buffer = (int *)connect(&shmid_buffer, 1, 4);
	*shmaddr_buffer = ROOTNO;	
	return 0;
}
