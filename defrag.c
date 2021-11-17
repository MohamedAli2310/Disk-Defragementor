#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "defrag.h"

#define DEBUG 1
#define OFFSET_INODES (buffer + 512 + BLOCKSIZE + BLOCKSIZE * sb->inode_offset)
#define OFFSET_DATA   (buffer + 512 + BLOCKSIZE + BLOCKSIZE * sb->data_offset)


/*TODO:
 * triple indirect
 * copy swap, boot, and sb
 * set free_block
 * create free_and_exit()
 * make sure you handle all errors using free_and_exit
 * function prototypes*/


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

/*pastes exactly one block to the new defraged disk given the offset of the old disk*/
void block_paste(int offset){
	if (fwrite((void *)(OFFSET_DATA + BLOCKSIZE*offset), 1, BLOCKSIZE, defraged) != BLOCKSIZE)
			free_and_exit();
}

void fill_index_block(int first, int last){
	int pointers[BLOCKSIZE/4];
	for (int i = first; i <= last; i++){
		pointers[i-first] = i;
	}
	if (fwrite(pointers, 1, BLOCKSIZE, defraged) != BLOCKSIZE)
			free_and_exit();
	if (fseek(defraged, 0, SEEK_END) == -1)
			free_and_exit();
}

void write_inode(inode *new, int offset){
	long inode_position = 512 + BLOCKSIZE + BLOCKSIZE * sb->inode_offset + offset * sizeof(inode);
	if (fseek(defraged, inode_position, SEEK_SET) == -1) 
			free_and_exit();
  	if (fwrite(new, 1, sizeof(inode), defraged) != sizeof(inode)) 
			free_and_exit();
  	if (fseek(defraged, 0, SEEK_END) == -1) 
			free_and_exit();	
}

void file_copy(inode *orig, inode *new, int n){
	int remaining = orig->size;
	// copy direct data blocks
	for (int i = 0; i < N_DBLOCKS; i++){
		if (remaining <= 0)
			break;
		block_paste(orig->dblocks[i]);
		new->dblocks[i] = usedcount;
		usedcount++;
		remaining-=BLOCKSIZE
	}
	//copy indirect blocks pointer blocks and data blocks
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
			/*offset to the location of the pointer in disk and then cast it into an int
			 * pointer to get 4 bits, then get the value of the pointer as an int to be
			 * used as an offset from data_offset*/
			int offset = *((int *)OFFSET_DATA + BLOCKSIZE * orig->iblocks[i] + j);
			block_paste(offset);
			usedcount++;
			remaining-=BLOCKSIZE;
		}

	}
	//copy double indirect pointer blocks and data blocks
	if (remaining <= 0)
			break;
	fill_index_block(usedcound+1, usedcount + BLOCKSIZE/4);
	new->i2block = usedcount;
	usedcount++;
	/*variables to count how many blocks are actually used without decreasing the value
	 * of "remaining", as the pointer blocks doesn't count towards inode->size*/
	int temp = remaining;
	int count = 0;
	for (int i = 0; i < BLOCKSIZE/4; i++){
		if (temp <= 0)
				break;
		/*number of blocks referred to by one pointer block (128 blocks for a 512 disk)*/
		int to_skip = BLOCKSIZE/4;
		/*skip 128 per pointer block plus 128 for the higher level pointer block*/
		int first = i * to_skip + to_skip;
		/*we have exactly 128 pointers per block, so last is 128 away from first*/
		int last = first + to_skip - 1;
		fill_index_block(usedcount + first, usedcount + last);
		count++;
		/*this makes sure that there is actual data per pointer without decreasing remaining*/
		temp-=BLOCKSIZE;
	}
	/*add however many blocks were used*/
	usedcount+=count;
	for (int i = 0; i < BLOCKSIZE; i+=4){
		if (remaining<=0)
				break;
		int outer_offset = (*(int *) OFFSET_DATA + BLOCKSIZE * orig->i2block + i);
		for (int j = 0; j < BLOCKSIZE; j+=4){
			if (remaining<=0)
					break;
			int inner_offset = *((int *)OFFSET_DATA + BLOCKSIZE * outer_offset + j);
			block_paste(inner_offset);
			usedcount++;
			remaining-=BLOCKSIZE;
		}

	}
	// copy triple indirect pointer blocks and data blocks
	
	/*TODO*/

	// write the new inode
	write_inode(new, n);
	return;
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
			file_copy(curr, temp, i);
		}
	}
	return 0;
}
