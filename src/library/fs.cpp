// fs.cpp: File System

#include "sfs/fs.h"

#include <algorithm>

#include <assert.h>
#include <stdio.h>
#include <string.h>

//get the details of inodes

void FileSystem::debugInodeBlock(Disk *disk, int inode_block_num)
{
    Block block;
    for (int i = 0; i < inode_block_num; i++)
    {
        disk->read(i + 1, block.Data);
        for (int j = 0; j < INODES_PER_BLOCK; j++)
        {
            if (block.Inodes[j].Valid == 0)
            {
                continue;
            }
            printf("Inode %d:\n", j);
            printf("    size: %u bytes\n", block.Inodes[j].Size);

            printf("    direct blocks:");
            for (int k = 0; k < POINTERS_PER_INODE; k++)
            {
                if (block.Inodes[j].Direct[k] > 0)
                {
                    printf(" %d", block.Inodes[j].Direct[k]);
                }
            }
            printf("\n");

            int indirect = block.Inodes[j].Indirect;
            if (indirect != 0)
            {
                printf("    indirect block: %d\n", indirect);
                readIndirectBlock(disk, indirect);
            }
        }
    }
}

//get indirect data blocks

void FileSystem::readIndirectBlock(Disk *disk, int block_num)
{
    Block block;
    disk->read(block_num, block.Data);
    printf("    indirect data blocks:");
    for (int i = 0; i < POINTERS_PER_BLOCK; i++)
    {
        if (block.Pointers[i] > 0)
        {
            printf(" %u", block.Pointers[i]);
        }
    }
    printf("\n");
}

// Debug file system -----------------------------------------------------------

void FileSystem::debug(Disk *disk)
{
    Block block;
    // Read Superblock
    disk->read(0, block.Data);
    printf("SuperBlock:\n");
    if (block.Super.MagicNumber == MAGIC_NUMBER)
    {
        printf("    magic number is valid\n");
    }
    else
    {
        printf("    magic number is invalid\n");
    }

    printf("    %u blocks\n", block.Super.Blocks);
    printf("    %u inode blocks\n", block.Super.InodeBlocks);
    printf("    %u inodes\n", block.Super.Inodes);
    // Read Inode blocks
    debugInodeBlock(disk, block.Super.InodeBlocks);
}

// init a free block when use it

void FileSystem::init_data_block(int block_num)
{
    Block data;
    memset(&data, 0, 4096);
    this->disk->write(block_num, (char *)&data);
}

// Format file system ----------------------------------------------------------

bool FileSystem::format(Disk *disk)
{
    // Write superblock
    if (disk->mounted())
    {
        return false;
    }
    size_t size = disk->size();
    Block superBlock;
    memset(&superBlock, 0, 4096);
    superBlock.Super.MagicNumber = MAGIC_NUMBER;
    superBlock.Super.Blocks = size;
    superBlock.Super.InodeBlocks = (size % 10 == 0) ? size / 10 : size / 10 + 1;
    superBlock.Super.Inodes = INODES_PER_BLOCK * superBlock.Super.InodeBlocks;
    disk->write(0, (char *)&superBlock.Super);
    // Clear all other blocks
    for (int i = 0; i < size - 1; i++)
    {
        Block temp;
        memset(temp.Data, 0, 4096);
        disk->write(i + 1, (char *)&temp);
    }
    return true;
}

//generate bitmap for filesystem

void FileSystem::get_bitmap(Block block)
{
    int bitmap[block.Super.Blocks];
    for (int i = 0; i < block.Super.Blocks; i++)
    {
        bitmap[i] = 0;
    }
    bitmap[0] = 1;
    for (int i = 0; i < block.Super.InodeBlocks; i++)
    {
        bitmap[i + 1] = 1;
        Block inodes_block;
        disk->read(i + 1, inodes_block.Data);
        for (int j = 0; j < INODES_PER_BLOCK; j++)
        {
            if (inodes_block.Inodes[j].Valid == 1)
            {

                for (int k = 0; k < POINTERS_PER_INODE; k++)
                {
                    int direct = inodes_block.Inodes[j].Direct[k];
                    if (direct != 0)
                    {
                        bitmap[direct] = 1;
                    }
                }
                if (inodes_block.Inodes[j].Indirect != 0)
                {
                    bitmap[inodes_block.Inodes[j].Indirect] = 1;
                    Block indirect;
                    this->disk->read(inodes_block.Inodes[j].Indirect, indirect.Data);
                    for (int k = 0; k < POINTERS_PER_BLOCK; k++)
                    {
                        if (indirect.Pointers[k] != 0)
                        {
                            bitmap[indirect.Pointers[k]] = 1;
                        }
                    }
                }
            }
        }
    }
    for (int i = 0; i < block.Super.Blocks; i++)
    {
        this->bitmap[i] = bitmap[i];
    }
}

// Mount file system -----------------------------------------------------------

bool FileSystem::mount(Disk *disk)
{
    if (disk->mounted())
    {
        return false;
    }
    Block block;
    // Read superblock
    disk->read(0, block.Data);
    uint32_t blocks = disk->size();
    uint32_t inode_blocks = (blocks % 10 == 0) ? blocks / 10 : blocks / 10 + 1;
    uint32_t inodes = inode_blocks * INODES_PER_BLOCK;
    if (block.Super.MagicNumber != MAGIC_NUMBER || block.Super.Blocks != blocks || block.Super.InodeBlocks != inode_blocks || block.Super.Inodes != inodes)
    {
        return false;
    }
    // Set device and mount
    disk->mount();
    // Copy metadata
    this->disk = disk;
    // Allocate free block bitmap
    this->blocks = blocks;
    this->inode_blocks = inode_blocks;
    this->inodes = inodes;
    this->get_bitmap(block);
    return true;
}

// Create inode ----------------------------------------------------------------

ssize_t FileSystem::create()
{
    // Locate free inode in inode table
    Block super_block;
    this->disk->read(0, super_block.Data);
    for (int i = 0; i < super_block.Super.InodeBlocks; i++)
    {
        Block temp;
        this->disk->read(i + 1, temp.Data);
        for (int j = 0; j < INODES_PER_BLOCK; j++)
        {
            if (temp.Inodes[j].Valid == 0)
            {
                temp.Inodes[j].Valid = 1;
                this->disk->write(i + 1, (char *)&temp);
                return j;
            }
        }
    }
    // Record inode if found
    return -1;
}

//load node by inumber

ssize_t FileSystem::load_node(size_t inumber, Inode *node)
{
    int inode_block = inumber / INODES_PER_BLOCK + 1;
    int index = inumber % INODES_PER_BLOCK;
    Block block;
    this->disk->read(inode_block, block.Data);
    if (block.Inodes[index].Valid == 0)
    {
        return -1;
    }
    // node = &block.Inodes[index];
    memcpy(node, &block.Inodes[index], sizeof(Inode));
    int size = block.Inodes[index].Size;
    return size;
}

// save the inumber
bool FileSystem::save_node(size_t inumber, Inode *node)
{
    int inode_block = inumber / INODES_PER_BLOCK + 1;
    int index = inumber % INODES_PER_BLOCK;
    Block block;
    this->disk->read(inode_block, block.Data);
    if (block.Inodes[index].Valid == 0)
    {
        return false;
    }
    memcpy(&block.Inodes[index], node, sizeof(Inode));
    this->disk->write(inode_block, (char *)&block);
    return true;
}
// Remove inode ----------------------------------------------------------------

bool FileSystem::remove(size_t inumber)
{
    // Load inode information
    Inode node;
    memset(&node, 0, sizeof(Inode));
    if (this->load_node(inumber, &node) < 0)
    {
        return false;
    }
    node.Valid = 0;
    node.Size = 0;
    for (int i = 0; i < POINTERS_PER_INODE; i++)
    {
        if (node.Direct[i] != 0)
        {
            int block_num = node.Direct[i];
            // int data = 0;
            this->init_data_block(block_num);
            this->bitmap[block_num] = 0;
            node.Direct[i] = 0;
        }
    }
    // Free indirect blocks
    if (node.Indirect != 0)
    {
        Block indirect_block;
        this->disk->read(node.Indirect, indirect_block.Data);
        for (int i = 0; i < POINTERS_PER_BLOCK; i++)
        {
            if (indirect_block.Pointers[i] != 0)
            {
                this->init_data_block(indirect_block.Pointers[i]);
                this->bitmap[indirect_block.Pointers[i]] = 0;
                indirect_block.Pointers[i] = 0;
            }
        }
        this->init_data_block(node.Indirect);
        this->bitmap[node.Indirect] = 0;
        node.Indirect = 0;
    }
    // Clear inode in inode table
    return save_node(inumber, &node);
}

// Inode stat ------------------------------------------------------------------

ssize_t FileSystem::stat(size_t inumber)
{
    // Load inode information
    Inode node;
    memset(&node, 0, sizeof(Inode));
    return this->load_node(inumber, &node);
}

// Read from inode -------------------------------------------------------------

ssize_t FileSystem::read(size_t inumber, char *data, size_t length, size_t offset)
{
    // Load inode information
    Inode node;
    memset(&node, 0, sizeof(Inode));
    int max_size = this->load_node(inumber, &node);

    // Adjust length
    int direct_size = POINTERS_PER_INODE * 4096;
    if (max_size < offset)
    {
        return -1;
    }
    else if (max_size < offset + length)
    {
        length = max_size - offset;
    }
    // Read block and copy to data
    int off_block = offset / 4096;
    int off_byte = offset % 4096;
    int data_offset = 0;
    //如果从直接块开始，就读取直接块的数据
    while (off_block < POINTERS_PER_INODE && (int)length > 0)
    {
        int copy_length = (length > 4096 - off_byte) ? 4096 - off_byte : length;
        Block data_block;
        this->disk->read(node.Direct[off_block], data_block.Data);
        memcpy(data + data_offset, &data_block + off_byte, copy_length);
        off_block++;
        length = length - copy_length;
        data_offset = data_offset + copy_length;
        off_byte = 0;
    }
    int indirect_off_block = off_block - 5;
    //如果从间接块开始，或者还有数据需要读
    //将代码这样分开可以减少读磁盘的次数，只需要读一次间接索引块
    if ((int)length > 0)
    {
        Block indirect_block;
        if (node.Indirect == 0)
        {
            return -1;
        }
        this->disk->read(node.Indirect, indirect_block.Data);
        while (indirect_off_block < POINTERS_PER_BLOCK && (int)length > 0)
        {
            int copy_length = (length > 4096 - off_byte) ? 4096 - off_byte : length;
            Block data_block;
            this->disk->read(indirect_block.Pointers[indirect_off_block], data_block.Data);
            memcpy(data + data_offset, &data_block + off_byte, copy_length);
            indirect_off_block++;
            length = length - copy_length;
            data_offset = data_offset + copy_length;
            off_byte = 0;
        }
    }
    return data_offset;
}

// Write to inode --------------------------------------------------------------
ssize_t FileSystem::write(size_t inumber, char *data, size_t length, size_t offset)
{
    // Load inode
    Inode node;
    memset(&node, 0, sizeof(Inode));
    if (this->load_node(inumber, &node) < 0)
    {
        return -1;
    }
    int off_block = offset / 4096;
    int off_byte = offset % 4096;
    int data_offset = 0;
    //如果从直接块开始写
    while (off_block < POINTERS_PER_INODE && length > 0)
    {
        Block start_block;
        //如果没有数据块就分配数据块
        if (node.Direct[off_block] == 0)
        {
            int new_free = this->get_free_block();
            //没有空闲块的话将更改的块写回，然后返回
            if (new_free <= 0)
            {
                node.Size += data_offset;
                this->save_node(inumber, &node);
                return data_offset;
            }
            node.Direct[off_block] = new_free;
            this->bitmap[new_free] = 1;
            Block temp;
            memset(&temp, 0, 4096);
            this->disk->write(new_free, (char *)&temp);
        }
        //从一个已有数据的数据块开始写的情况
        if (off_byte > 0)
        {
            this->disk->read(node.Direct[off_block], start_block.Data);
        }
        int copy_length = (length > 4096 - off_byte) ? 4096 - off_byte : length;
        memcpy(&start_block + off_byte, data + data_offset, copy_length);
        this->disk->write(node.Direct[off_block], (char *)&start_block);
        length = length - copy_length;
        off_byte = 0;
        data_offset = data_offset + copy_length;
        off_block++;
    }
    int indirect_off_block = off_block - 5;
    //如果直接块不够，分配间接块，并写入数据
    if (length > 0)
    {
        if (node.Indirect == 0)
        {
            //分配间接块
            int new_free = this->get_free_block();
            if (new_free <= 0)
            {
                node.Size += data_offset;
                this->save_node(inumber, &node);
                return data_offset;
            }
            node.Indirect = new_free;
            this->bitmap[new_free] = 1;
            this->init_data_block(new_free);
        }
        Block indirect_block;
        this->disk->read(node.Indirect, indirect_block.Data);
        while (indirect_off_block < POINTERS_PER_BLOCK && length > 0)
        {
            Block start_block;
            //如果没有数据块就分配数据块
            if (indirect_block.Pointers[indirect_off_block] == 0)
            {
                int new_free = this->get_free_block();
                if (new_free <= 0)
                {
                    this->disk->write(node.Indirect, (char *)&indirect_block);
                    node.Size += data_offset;
                    this->save_node(inumber, &node);
                    return data_offset;
                }
                indirect_block.Pointers[indirect_off_block] = new_free;
                this->bitmap[new_free] = 1;
                this->init_data_block(new_free);
            }
            if (off_byte > 0)
            {
                this->disk->read(indirect_block.Pointers[indirect_off_block], start_block.Data);
                off_byte = 0;
            }
            int copy_length = ((int)length > 4096 - off_byte) ? 4096 - off_byte : length;
            memcpy(&start_block + off_byte, data + data_offset, copy_length);
            this->disk->write(indirect_block.Pointers[indirect_off_block], (char *)&start_block);
            length = length - copy_length;
            data_offset = data_offset + copy_length;
            indirect_off_block++;
        }
        this->disk->write(node.Indirect, (char *)&indirect_block);
    }
    node.Size += data_offset;
    this->save_node(inumber, &node);
    return data_offset;
}

int FileSystem::get_free_block()
{
    for (int i = 0; i < this->blocks; i++)
    {
        if (this->bitmap[i] == 0)
        {
            return i;
        }
    }
    return -1;
}
