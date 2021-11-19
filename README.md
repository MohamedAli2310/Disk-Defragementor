# Disk Defragmentor
A utility to defragment an inode disk.

### Compilation

Compile using make

Make clean to get rid of executables

Run using ./defrag.o [Disk Name]

### Basic structure and design
- Disk is read into memory
- Superblock is checked for blocksize and the number of inodes
- The new defraged disk is initialized
- we seek forward to the data offset to start defragging
- Defrag happens as follows:
  - inodes are copied to use as a template when copying
  - Every inode is checked for inode->nlink to see if it's in use
  - An inode not in use will be copied as is
  - An inode in use will be traced block by block and written in order in the data block
  - The tracing happens as follows:
    - A variable holds the value for the number of blocks remaining in the file
    - The 10 direct blocks are checked first
    - If there is still data left, indirect blocks are traced
    - Pointer block is written and taken care of
    - Same goes for double and triple indirect blocks
  - The new node is written in its place in the nodes area
- Superblock is updated with the new free block pointer
- Boot and superblock are copied over
- Free blocks are entirely filled with pointers for the next free block
- Last free block has -1 all over it
- Swap is checked and copied over if present
- The buffer and the copied list of inodes is freed
- The new file is closed

#### Structs:

- superblock
- inode

#### Globals:

- sb: a superblock struct that holds the superblock of the fragmented disk
- buffer: a pointer to the memory chunk wher the disk image is read
- new\_inodes: a pointer to the memory chunk where the inodes are memcopied over
- defraged: the new defragged FILE
