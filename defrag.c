#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "defrag.h"

#define DEBUG 1

superblock *sb;
void *buffer;
void *new_inodes;
FILE *defraged;
int disksize;

/*Function Prototypes*/
void read_disk(char*);
void sanity_check();
void get_n_inodes();

int main(int argc, char *argv[]){
	if (argc != 2){
		printf("use ./defrag.o -h for help\n");
	}
	else{
		read_disk(argv[1]);
	}
	/*Checks the attributes of the suberblock*/
	sanity_check();
	get_n_inodes();
	return(0);
}

void read_disk(char *disk){
	FILE *fp = fopen(disk, "r");	
	if (fp == NULL){
		printf("File doesn't exist\n");
		exit(1);
	}
	if (fseek(fp, 0, SEEK_END) == -1){
		printf("Error seeking to end of file\n");
		exit(1);
	}
	disksize = ftell(fp);
	if (disksize == -1) exit(1);
	//file size too big?
	if (fseek(fp, 0, SEEK_SET) == -1){
        printf("Error seeking to beginning of file\n");
        exit(1);
    }
	/*malloc in chunks for huge disks?*/
	buffer = malloc(disksize);
	if (buffer == NULL){
		printf("Error in malloc for disksize\n");
		exit(1);
	}
	/*Read disk into buffer*/
	if (fread(buffer, 1, disksize, fp) == -1){
		printf("Error reading disk into buffer\n");
		exit(1);
	}
	/*read superblock and define block size
	 * boot size is assumed to be 512*/
	sb = (superblock*)(buffer+512);
#undef BLOCKSIZE
#define BLOCKSIZE sb->size

	fclose(fp);
	return;
}

void sanity_check(){
	printf("--------SUPERBLOCK-------------------------------------\n");
	printf("size: %d\n", sb->size);
	printf("inode_offset: %d\n", sb->inode_offset);
	printf("date_offset: %d\n", sb->data_offset);
	printf("swap_offset: %d\n", sb->swap_offset);
	printf("free_inode: %d\n", sb->free_inode);
	printf("free_block: %d\n", sb->free_block);
	printf("------------------------------------------------------\n");
}

void get_n_inodes(){
	int n_inodes = (sb->data_offset - sb->inode_offset) * BLOCKSIZE 
			/ sizeof(inode);
	if (DEBUG==1){
		printf("number of inodes: %d\n", n_inodes);
	}
#undef N_INODES
#define N_INODES n_inodes
}

int defrag(){
	new_inodes = malloc(sizeof(inode) * N_INODES);
	int inodes_position = buffer + 512 + BLOCKSIZE + 
			BLOCKSIZE * sb->inode_offset;
	/*copy old inodes to new_inodes*/
	memcpy(new_inodes, inodes_position, sizeof(inode)*N_INODES);


}


