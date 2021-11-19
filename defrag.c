#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "defrag.h"

#define DEBUG 0
#define OFFSET_INODES (void *)(buffer + 512 + BLOCKSIZE + BLOCKSIZE * sb->inode_offset)
#define OFFSET_DATA   (void *)(buffer + 512 + BLOCKSIZE + BLOCKSIZE * sb->data_offset)
#define OFFSET_SWAP   (void *)(buffer + 512 + BLOCKSIZE + BLOCKSIZE * sb->swap_offset)

superblock *sb;
void *buffer;
void *new_inodes;
FILE *defraged;
int usedcount;
int disksize;
int n_inodes;

/*Function Prototypes
 * This design was inspired 
 * by and adapted from 
 * https://github.com/
 * phuchcao/defrag/
 * */
void read_disk(char*);
void sanity_check();
void get_n_inodes();
void initialize_newdisk();
void block_paste(int);
void fill_index_block(int, int);
void write_inode(inode *, int);
void file_copy(inode *, inode *, int);
void defrag();
void free_and_exit();
void free_all();


int main(int argc, char *argv[]){
	usedcount = 0;
	if (argc != 2){
		printf("use ./defrag.o -h for help\n");
	}
	else{
		if (strcmp(argv[1], "-h") == 0){
			printf("Usage: ./defrag.o [DISK IMAGE]\n Output: a new disk image with \"-defrag\" concatenated to the end of the input file name\n");
			exit(1);
		}
		else{
			read_disk(argv[1]);
		}
	}
	/*Checks the attributes of the suberblock*/
	if (DEBUG == 1)
			sanity_check();
	get_n_inodes();
	defrag();
	if (DEBUG == 1){
		int *x;
		x = malloc(sizeof(int));
		long o = 1024+sb->data_offset*BLOCKSIZE+10*BLOCKSIZE;
		fseek(defraged, o, SEEK_SET);
    	fread(x, 1, sizeof(int), defraged);
		fseek(defraged, 0, SEEK_END);
    	printf("TEST: %d\n", *x);
	}

	/*set free block pointer to usedcount*/
	sb->free_block = usedcount;
	
	/*write boot and superblock*/
	if (fseek(defraged, 0, SEEK_SET) == -1)
			free_and_exit();
	if (fwrite(buffer, 1, 512 + BLOCKSIZE, defraged) != 512 + BLOCKSIZE) 
			free_and_exit();
	if (fseek(defraged, 0, SEEK_END) == -1) 
			free_and_exit();
	/*end of empty data*/
	void *data_end = (OFFSET_SWAP - 1);
	/*pointer to the current free data block*/
	void *current_free = (OFFSET_DATA + BLOCKSIZE * usedcount);
	/*end of used data blocks*/
	void *useddata_end = buffer + 512 + BLOCKSIZE + BLOCKSIZE * (sb->data_offset + usedcount -1) + 1;
	/*number of free blocks to fill with pointers to the next free block*/
	int to_fill = ((data_end - useddata_end + 1) / BLOCKSIZE);
	if (DEBUG == 1)
		printf("TOFILL: %d\n", to_fill);
	/*fill all blocks with pointers to next free block except the last one*/
	for (int i = 0; i < to_fill - 1; i++){
		*(int *) current_free = usedcount;
		fwrite(current_free, 1, BLOCKSIZE, defraged);
		usedcount++;
		current_free+=BLOCKSIZE;
	}
	/*fill the last block with  -1*/
	*(int *)(current_free) = -1;
	fwrite(current_free, 1, BLOCKSIZE, defraged);
	if (DEBUG == 1){
		unsigned long p = ftell(defraged);
		printf("ppppp: %ld\n", p);
	}
	/*write swap region*/
	if (OFFSET_SWAP < buffer+disksize)
		fwrite(OFFSET_SWAP, 1, buffer + disksize - OFFSET_SWAP - 1, defraged);

	free_all();
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
	usedcount = 0;
	FILE *fp = fopen(disk, "r");
	if (fp == NULL){
		printf("File doesn't exist\n");
		exit(1);
	}
	initialize_newdisk(disk);
	if (fseek(fp, 0, SEEK_END) == -1)
			free_and_exit();
	disksize = ftell(fp);
	if (disksize == -1) 
			free_and_exit();
	if (fseek(fp, 0, SEEK_SET) == -1)
			free_and_exit();
	buffer = malloc(disksize);
	if (buffer == NULL){
		printf("Error in malloc for disksize\n");
		exit(1);
	}
	/*Read disk into buffer*/
	if (fread(buffer, 1, disksize, fp) == -1)
			free_and_exit();

	/*read superblock and define block size
	 * boot size is assumed to be 512*/
	sb = (superblock*)(buffer+512);
#undef BLOCKSIZE
#define BLOCKSIZE sb->size

	/*SEEK TO SKIP TO DATA SECTION. I STRAIGHT UP SPENT 6 HOURS DEBUGGING
	 * BECAUSE I FORGOT THIS :((((((((((((((((((((((((((((((((((((((((((((*/
	if (fseek(defraged, 512 + BLOCKSIZE + BLOCKSIZE * sb->data_offset, SEEK_SET) == -1) 
			free_and_exit();
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
	n_inodes = (sb->data_offset - sb->inode_offset) * BLOCKSIZE/ sizeof(inode);
#undef N_INODES
#define N_INODES n_inodes
	if (DEBUG==1){
		printf("number of inodes: %d\n", n_inodes);
	}
}

/*pastes exactly one block to the new defraged disk given the offset of the old disk*/
void block_paste(int offset){
	if (DEBUG == 1)	
		printf("fillindex: %d\n", usedcount);	
	if (fwrite((void *)(OFFSET_DATA + BLOCKSIZE*offset), 1, BLOCKSIZE, defraged) != BLOCKSIZE)
			free_and_exit();
}

void fill_index_block(int first, int last){
	/*DEBUGGING BLOCK TO MAKE SURE INDICES ARE WRITTEN TO THE CORRECT BYTES*/	
	if (DEBUG == 1){
		unsigned long position = ftell(defraged);
		printf("FTELL: %ld\n", position);
	}
	int pointers[BLOCKSIZE/4];
	for (int i = first; i <= last; i++){
		if (DEBUG == 1)
			printf("first: %d, last: %d, index: %d, i: %d\n", first, last, i-first, i);
		pointers[i-first] = i;
	}
	if (fwrite(pointers, 1, BLOCKSIZE, defraged) != BLOCKSIZE)
			free_and_exit();
	if (DEBUG == 1){
		fseek(defraged, BLOCKSIZE, SEEK_END);
		fread(pointers, 1, BLOCKSIZE, defraged);
		printf("lets see: %d\n", pointers[0]);
	}

	if (fseek(defraged, 0, SEEK_END) == -1)
			free_and_exit();

}

void write_inode(inode *new, int offset){
	long inode_position = 512 + BLOCKSIZE + BLOCKSIZE * sb->inode_offset + offset * sizeof(inode);
	if (DEBUG == 1)
			printf("inodepos: %ld\n", inode_position);
	/*seek to inode position*/
	if (fseek(defraged, inode_position, SEEK_SET) == -1) 
			free_and_exit();
	/*write inode*/
  	if (fwrite(new, 1, sizeof(inode), defraged) != sizeof(inode)) 
			free_and_exit();
	/*seek back to the end of the file to write more data blocks*/
  	if (fseek(defraged, 0, SEEK_END) == -1) 
			free_and_exit();	
}

void file_copy(inode *orig, inode *new, int n){
	int remaining = orig->size;
	if (DEBUG == 1){
		printf("-----inode: %d------------------------------------\n", n);
		printf("usedcount: %d\n", usedcount);
	}
	

	// copy direct data blocks
	for (int i = 0; i < N_DBLOCKS; i++){
		if (DEBUG == 1)
			printf("dblocks: %d\n", i);
		if (remaining <= 0)
			break;
		if (DEBUG == 1){
			unsigned long position = ftell(defraged);
    		printf("FTELL2bef: %ld\n", position);
		}
		block_paste(orig->dblocks[i]);
		if (DEBUG == 1){
			unsigned long position = ftell(defraged);
	    	printf("FTELL2aft: %ld\n", position);
		}
		new->dblocks[i] = usedcount;
		usedcount++;
		remaining-=BLOCKSIZE;
	}
	if (DEBUG == 1)
			printf("usedcount: %d\n", usedcount);


	//copy indirect blocks pointer blocks and data blocks
	for (int i = 0; i < N_IBLOCKS; i++){
		if (DEBUG == 1)
				printf("iblocks: %d\n", i);
		if (remaining <= 0)
				break;
		fill_index_block(usedcount+1, usedcount + BLOCKSIZE/4);
	  	if(DEBUG == 1){
			int *x;
    		x = malloc(sizeof(int));
    		long o = 1024+sb->data_offset*BLOCKSIZE+10*BLOCKSIZE;
    		fseek(defraged, o, SEEK_SET);
    		fread(x, 1, sizeof(int), defraged);
			fseek(defraged, 0, SEEK_END);
    		printf("TEST: %d\n", *x);
		}
		new->iblocks[i] = usedcount;
		usedcount++;
		/*follow pointers in the indirect pointers block*/
		for (int j = 0; j < BLOCKSIZE; j+=4){
			if (remaining <= 0)
                break;
			/*offset to the location of the pointer in disk and then cast it into an int
			 * pointer to get 4 bits, then get the value of the pointer as an int to be
			 * used as an offset from data_offset*/
			int offset = *((int *)(OFFSET_DATA + BLOCKSIZE * orig->iblocks[i] + j));
			if (DEBUG == 1){
				printf("usedcount: %d\n", usedcount);
				printf("OFFSET_DATA: %p, BLOCKSIZE: %d, orig->iblocks[i]: %d, j: %d\n", 
							OFFSET_DATA, BLOCKSIZE, orig->iblocks[i], j);
			}
			block_paste(offset);
			usedcount++;
			remaining-=BLOCKSIZE;
		}

	}
	if (DEBUG == 1)
			printf("after indirect\n");
	/*copy double indirect pointer blocks and data blocks
	 * loop is only to enable breaking*/
	for (int _ = 0; _<1; _++){
		if (remaining <= 0)
				break;
		fill_index_block(usedcount+1, usedcount + BLOCKSIZE/4);
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
			int outer_offset = (*(int *)(OFFSET_DATA + BLOCKSIZE * orig->i2block + i));
			for (int j = 0; j < BLOCKSIZE; j+=4){
				if (remaining<=0)
						break;
				int inner_offset = *((int *)(OFFSET_DATA + BLOCKSIZE * outer_offset + j));
				block_paste(inner_offset);
				usedcount++;
				remaining-=BLOCKSIZE;
			}
		}
	}
	/*copy triple indirect pointer blocks and data blocks
	 * loop is only to enable breaking*/
	for (int _ = 0; _<1; _++){
		if (remaining <= 0)
				break;
		fill_index_block(usedcount + 1, usedcount + BLOCKSIZE/4);
		new->i3block = usedcount;
		usedcount++;
		int temp = remaining;
		int count = usedcount;
		for (int i = 0; i < BLOCKSIZE/4; i++){
			if(temp<=0)
					break;
			/*number of blocks referred to by one pointer block (128 blocks for a 512 disk)*/
			int to_skip = BLOCKSIZE/4;
			/*skip 128 per pointer block plus 128 for the higher level pointer block*/
			int first = i * to_skip + to_skip;
			/*we have exactly 128 pointers per block, so last is 128 away from first*/
			int last = first + to_skip - 1;
			fill_index_block(count + first, count + last);
			usedcount++;
			temp = temp-BLOCKSIZE;
		}
		/*second level pointers (after 128 blocks of lvl0 and 128*128 of lvl1)*/
		int lvl2count = count + BLOCKSIZE/4*BLOCKSIZE/4 + BLOCKSIZE/4;
		for (int i = 0; i < BLOCKSIZE*BLOCKSIZE; i+=(BLOCKSIZE/4)){
			if(temp<=0)
					break;
			fill_index_block(lvl2count + i, lvl2count + BLOCKSIZE/4-1);
			usedcount++;
			temp = temp-BLOCKSIZE;
			}
		for (int i = 0; i < BLOCKSIZE; i+=4){
			if (remaining <= 0)
					break;
			int out_offset =  *((int *)(OFFSET_DATA + BLOCKSIZE*orig->i3block + i));
			for (int j = 0; j < BLOCKSIZE; j+=4){
				if (remaining <= 0)
						break;
				int mid_offset = *((int *)(OFFSET_DATA + BLOCKSIZE*out_offset + j));
				for (int k = 0; k< BLOCKSIZE; k+=4){
					if (remaining <= 0)
							break;
					int in_offset = *((int *)(OFFSET_DATA + BLOCKSIZE*mid_offset + k));
					block_paste(in_offset);
					usedcount++;
					remaining-=BLOCKSIZE; 
				}
			}
		}
	}

	// write the new inode
	write_inode(new, n);
	return;
}

void defrag(){
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
		else{
			write_inode(curr, i);
		}
	}
}

void free_and_exit(){
	free(buffer);
	free(new_inodes);
	fclose(defraged);
	printf("an error has occured. Exiting.\n");
	exit(1);
}

void free_all(){
	free(buffer);
	free(new_inodes);
	fclose(defraged);
}
