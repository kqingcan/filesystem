// fs.h: File System

#pragma once

#include "sfs/disk.h"

#include <stdint.h>

class FileSystem {
public:
    const static uint32_t MAGIC_NUMBER	     = 0xf0f03410;
    const static uint32_t INODES_PER_BLOCK   = 128;
    const static uint32_t POINTERS_PER_INODE = 5;
    const static uint32_t POINTERS_PER_BLOCK = 1024;

private:
    struct SuperBlock {		// Superblock structure
    	uint32_t MagicNumber;	// File system magic number
    	uint32_t Blocks;	// Number of blocks in file system
    	uint32_t InodeBlocks;	// Number of blocks reserved for inodes
    	uint32_t Inodes;	// Number of inodes in file system
    };

    struct Inode {
    	uint32_t Valid;		// Whether or not inode is valid
    	uint32_t Size;		// Size of file
    	uint32_t Direct[POINTERS_PER_INODE]; // Direct pointers
    	uint32_t Indirect;	// Indirect pointer
    };

    union Block {
    	SuperBlock  Super;			    // Superblock
    	Inode	    Inodes[INODES_PER_BLOCK];	    // Inode block
    	uint32_t    Pointers[POINTERS_PER_BLOCK];   // Pointer block
    	char	    Data[Disk::BLOCK_SIZE];	    // Data block
    };

    // TODO: Internal helper functions

    // TODO: Internal member variables
    Disk *disk;
    uint32_t blocks;
    uint32_t inode_blocks;
    uint32_t inodes;
    size_t old_blocks[200];
    int valid_num;
    int bitmap[200];

public:
    static void debugInodeBlock(Disk *disk, int inode_block_num);
    static void readIndirectBlock(Disk *disk, int block_num);
    static void debug(Disk *disk);
    static bool format(Disk *disk);
    // static bool remove_inode(Disk *disk, int inumber);

    bool mount(Disk *disk);
    ssize_t load_node(size_t inumber, Inode *node);
    bool save_node(size_t inumber, Inode *node);
    void init_data_block(int block_num);
    // void read_data_block(char *data,int block_num,int data_offset,int copy_length);
    ssize_t create();
    bool    remove(size_t inumber);
    ssize_t stat(size_t inumber);

    ssize_t read(size_t inumber, char *data, size_t length, size_t offset);
    ssize_t write(size_t inumber, char *data, size_t length, size_t offset);
    int get_free_block();
};
