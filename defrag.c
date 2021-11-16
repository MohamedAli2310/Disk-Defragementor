#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "defrag.h"

#define DEBUG 1
#define OFFSET_INODES (buffer + 512 + BLOCKSIZE + BLOCKSIZE * sb->inode_offset)
#define OFFSET_DATA   (buffer + 512 + BLOCKSIZE + BLOCKSIZE * sb->data_offset)

superblock *sb;
void *buffer;
void *new_inodes;
FILE *defraged;
int usedcount;
int disksize;

/*Function Prototypes*/
void read_disk(char*);
void sanity_check();
void get_n_inodes();
void initialize_newdisk();

int main(int argc, char *argv[]){
	usedcount = 0;
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

void initialize_newdisk(char *disk){
	char *defrag_name = (char *)malloc(strlen(disk)+strlen("-defrag")+1);
	strcpy(defrag_name, disk);
	strcat(defrag_name, "-defrag");
	defraged = fopen(defrag_name, "w+");
	free(defrag_name);
	if (defraged == NULL) 
			free_and_exit();
}

void read_disk(char *disk){
	FILE *fp = fopen(disk, "r");	
	if (fp == NULL){
		printf("File doesn't exist\n");
		exit(1);
	}
	initialize_newdisk(disk);
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

void block_copy(inode *orig, inode *new, int n){
	int remaining = orig->size;
	remaining = copy_direct(orig, new, n, remaining);
	remaining = copy_indirect(orig, new, n, remaining);
	remaining = copy_double(orig, new, n, remaining);
	remaining = copy_triple(orig, new, n, remaining);
	write_inode(new, n);
	return;
}

int copy_direct(inode *orig, inode *new, int n, int remaining){
	for (int i = 0; i < N_DBLOCKS; i++){
		if (remaining <= 0)
			break;
		block_paste(orig->dblocks[i]);
		new->dblocks[i] = usedcount;
		usedcount++;
		remaining-=BLOCKSIZE
	}
	return remaining;
}
int copy_indirect(inode *orig, inode *new, int n, int remaining){
	for (int i = 0; i < N_IBLOCKS; i++){
		if (remaining <= 0)
				break;
		//int indirect = orig->iblocks[i];
		fill_index_block(usedcount+1, usedcount + BLOCKSIZE/4);
		new->iblocks[i] = usedcount;
		usedcount++;
		/*follow pointers in the indirect pointers block*/
		for (int j = 0; j < BLOCKSIZE; j+=4){
			if (remaining <= 0)
                break;
			/*data offset in the original disk given by indirect block i at pointer j
			 * GET RID OF CASTING?*/
			int offset = *((int *)OFFSET_DATA + BLOCKSIZE * orig->iblocks[i] + j);
			block_copy(offset);
			usedcount++;
			remaining-=BLOCKSIZE;
		}

	}
	return remaining;
}
int copy_double(inode *orig, inode *new, int n, int remaining){
	if (remaining <= 0)
			break;
	fill_index_block(usedcound+1, usedcount + BLOCKSIZE/4);
	new->i2block = usedcount;
	usedcount++;
}
int copy_triple(inode *orig, inode *new, int n, int remaining){

}

int defrag(){
	new_inodes = malloc(sizeof(inode) * N_INODES);
	/*copy old inodes to new_inodes*/
	memcpy(new_inodes, OFFSET_INODES, sizeof(inode)*N_INODES);
	/*copy all files*/
	for (int i = 0; i < N_INODES; i++){
		inode *curr = (inode*) (OFFSET_INODES+ i * sizeof(inode));
		/*check if used*/
		if(curr->nlink > 0){
			inode *temp = (inode*) (new_inodes + i * sizeof(inode));
			block_copy(curr, temp, i);
		}
	}
	return 0;
}




