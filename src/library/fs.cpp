// fs.cpp: File System

#include "sfs/fs.h"

#include <algorithm>

#include <assert.h>
#include <stdio.h>
#include <string.h>


//get the details of inodes
void FileSystem::debugInodeBlock(Disk *disk, int inode_block_num){
    Block block;
    for (int i = 0; i < inode_block_num; i++)
    {
        disk->read(i+1,block.Data);
        for (int j = 0; j < INODES_PER_BLOCK; j++)
        {
            if (block.Inodes[j].Valid == 0)
            {
                continue;
            }
            printf("Inode %d:\n",j);
            printf("    size: %u bytes\n",block.Inodes[j].Size);
            int flag = 0;
            
            for (int  k = 0; k < POINTERS_PER_INODE; k++)
            {
                if (block.Inodes[j].Direct[k] >0)
                {
                    if (flag==0)
                    {
                        printf("    direct blocks:"); 
                        flag = 1;  
                    }
                    printf(" %d",block.Inodes[j].Direct[k]);
                }
            }
            if (flag ==1)
            {
                printf("\n");
            }
            int indirect = block.Inodes[j].Indirect;
            if (indirect !=0)
            {
                printf("    indirect block: %d\n",indirect);
                readIndirectBlock(disk,indirect);
            }
            
             
        }  
    }
    
}

void FileSystem::readIndirectBlock(Disk *disk, int block_num){
    Block block;
    disk->read(block_num,block.Data);
    printf("    indirect data blocks:");
    for (int i = 0; i < POINTERS_PER_BLOCK; i++)
    {
        if (block.Pointers[i] > 0)
        {
            printf(" %u",block.Pointers[i]);
        }
    }
    printf("\n");
}

// Debug file system -----------------------------------------------------------

void FileSystem::debug(Disk *disk) {
    Block block;

    // Read Superblock
    disk->read(0, block.Data);

    printf("SuperBlock:\n");
    if (block.Super.MagicNumber ==MAGIC_NUMBER)
    {
        printf("    magic number is valid\n");
    }else
    {
        printf("    magic number is invalid\n");
    }
    
    printf("    %u blocks\n"         , block.Super.Blocks);
    printf("    %u inode blocks\n"   , block.Super.InodeBlocks);
    printf("    %u inodes\n"         , block.Super.Inodes);
    debugInodeBlock(disk, block.Super.InodeBlocks);
    // Read Inode blocks
}

// Format file system ----------------------------------------------------------

bool FileSystem::format(Disk *disk) {
    // Write superblock
    size_t size = disk->size();
    Block superBlock;
    superBlock.Super.MagicNumber = MAGIC_NUMBER;
    superBlock.Super.Blocks = size;
    superBlock.Super.InodeBlocks = (size%10 ==0)? size/10 : size/10+1;
    superBlock.Super.Inodes = INODES_PER_BLOCK * superBlock.Super.InodeBlocks;

    disk->write(0,(char *)&superBlock.Super);


    // Clear all other blocks
    // for (int i = 0; i < size-1; i++)
    // {
    //     Block temp;
    //     disk->write(i+1,temp.Data);
    // }
    
    
    return true;
}

// Mount file system -----------------------------------------------------------

bool FileSystem::mount(Disk *disk) {
    // Read superblock

    // Set device and mount

    // Copy metadata

    // Allocate free block bitmap

    return true;
}

// Create inode ----------------------------------------------------------------

ssize_t FileSystem::create() {
    // Locate free inode in inode table

    // Record inode if found
    return 0;
}

// Remove inode ----------------------------------------------------------------

bool FileSystem::remove(size_t inumber) {
    // Load inode information

    // Free direct blocks

    // Free indirect blocks

    // Clear inode in inode table
    return true;
}

// Inode stat ------------------------------------------------------------------

ssize_t FileSystem::stat(size_t inumber) {
    // Load inode information
    return 0;
}

// Read from inode -------------------------------------------------------------

ssize_t FileSystem::read(size_t inumber, char *data, size_t length, size_t offset) {
    // Load inode information

    // Adjust length

    // Read block and copy to data
    return 0;
}

// Write to inode --------------------------------------------------------------

ssize_t FileSystem::write(size_t inumber, char *data, size_t length, size_t offset) {
    // Load inode
    
    // Write block and copy to data
    return 0;
}
