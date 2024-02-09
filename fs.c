#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "fs.h"

SUPERBLOCK* superblock;
INODE* inodes;
DATA_BLOCK* data_blocks;

void init_fs(char* fs_name, int blocks) {
    FILE* fs = fopen(fs_name, "w");
    if (fs == NULL) {
        printf("Error: could not create disk file.\n");
        return;
    }
    superblock = (SUPERBLOCK*) malloc(sizeof(SUPERBLOCK)); // allocate memory for superblock
    superblock->magic_number = MAGIC_NUMBER; // magic number to identify if the file is file system
    superblock->total_inodes = MAX_FILES; // every file has an inode
    superblock->total_data_blocks = blocks;
    superblock->user_space = blocks * BLOCK_SIZE; // user space = data blocks only
    superblock->used_user_space = 0;
    superblock->block_size = BLOCK_SIZE;
    superblock->total_size = sizeof(SUPERBLOCK) + (sizeof(INODE) * MAX_FILES) + (sizeof(DATA_BLOCK) * blocks);
    fwrite(superblock, sizeof(SUPERBLOCK), 1, fs); // write superblock to file
    inodes = (INODE*) malloc(sizeof(INODE) * MAX_FILES); // allocate memory for inodes
    for (int i = 0; i < MAX_FILES; i++) {
        inodes[i].name[0] = '\0';
        inodes[i].is_used = NOT_USED;
        inodes[i].size = 0;
        inodes[i].first_block = END_OF_FILE;
    }
    fwrite(inodes, sizeof(INODE), MAX_FILES, fs); // write inodes to file
    data_blocks = (DATA_BLOCK*) malloc(sizeof(DATA_BLOCK) * blocks); // allocate memory for data blocks
    for (int i = 0; i < blocks; i++) {
        data_blocks[i].is_used = NOT_USED;
        data_blocks[i].next_block = END_OF_FILE;
    }
    fwrite(data_blocks, sizeof(DATA_BLOCK), blocks, fs); // write data blocks to file
    fclose(fs);
}

void select_fs(char* fs_name) {
    FILE* fs = fopen(fs_name, "r+");
    if (fs == NULL) {
        printf("Error: could not open disk file.\n");
        return;
    }
    // load superblock, inodes, and data blocks into memory
    fseek(fs, 0, SEEK_SET);
    superblock = (SUPERBLOCK*) malloc(sizeof(SUPERBLOCK));
    fread(superblock, sizeof(SUPERBLOCK), 1, fs);
    if (superblock->magic_number != MAGIC_NUMBER) {
        printf("Error: file is not a file system.\n");
        return;
    }
    inodes = (INODE*) malloc(sizeof(INODE) * superblock->total_inodes);
    fread(inodes, sizeof(INODE), superblock->total_inodes, fs);
    data_blocks = (DATA_BLOCK*) malloc(sizeof(DATA_BLOCK) * superblock->total_data_blocks);
    fread(data_blocks, sizeof(DATA_BLOCK), superblock->total_data_blocks, fs);
    fclose(fs);
}

void copy_file_to_fs(char* fs_name, char* path_to_file) {
    FILE* fs = fopen(fs_name, "r+");
    if (fs == NULL) {
        printf("Error: could not open disk file.\n");
        return;
    }
    // load superblock, inodes, and data blocks into memory
    fseek(fs, 0, SEEK_SET);
    fread(superblock, sizeof(SUPERBLOCK), 1, fs);
    if (superblock->magic_number != MAGIC_NUMBER) {
        printf("Error: file is not a file system.\n");
        return;
    }
    fread(inodes, sizeof(INODE), superblock->total_inodes, fs);
    fread(data_blocks, sizeof(DATA_BLOCK), superblock->total_data_blocks, fs);
    int inode_index = get_free_inode();
    if (inode_index == -1) {
        printf("Error: no available inodes.\n");
        return;
    }
    // get file name from path
    char* file_name = strrchr(path_to_file, '/');
    if (file_name == NULL) {
        file_name = path_to_file;
    } else {
        file_name++; // skip '/'
    }
    if (strlen(file_name) >= MAX_FILE_NAME) {
        printf("Error: file name too long.\n");
        return;
    }
    // check if there is already a file with the same name
    int inode_with_name = get_inode_by_name(file_name);
    if (inode_with_name != -1 && inodes[inode_with_name].is_used == USED) {
        printf("Error: file already exists.\n");
        return;
    }
    // if not, set inode name to file name
    strncpy(inodes[inode_index].name, file_name, MAX_FILE_NAME);
    inodes[inode_index].name[MAX_FILE_NAME - 1] = '\0';
    // open file to copy
    FILE* file = fopen(path_to_file, "r");
    if (file == NULL) {
        printf("Error: could not open file.\n");
        return;
    }
    fseek(file, 0, SEEK_END);
    int file_size = ftell(file);
    if (file_size > superblock->user_space) {
        printf("Error: file too large.\n");
        return;
    }
    fseek(file, 0, SEEK_SET);
    inodes[inode_index].size = file_size;
    // copy file data to data blocks
    int current_block = -1;
    int previous_block = -1;
    int bytes_read = 0;
    int bytes_left = file_size;
    while (bytes_read < file_size) {
        for (int i = 0; i < superblock->total_data_blocks; i++) {
            if (data_blocks[i].is_used == NOT_USED) {
                if (current_block == -1) {
                    inodes[inode_index].first_block = i; // set first block
                }
                current_block = i;
                break;
            }
        }
        if (current_block == -1) {
            printf("Error: no available data blocks.\n");
            return;
        }
        if (previous_block != -1) {
            data_blocks[previous_block].next_block = current_block; // set next block value of previous block
        }
        data_blocks[current_block].is_used = USED;
        data_blocks[current_block].next_block = END_OF_FILE;
        if (bytes_left > BLOCK_SIZE) {
            // if there are more bytes to read than the block size, read a block only
            fread(data_blocks[current_block].data, BLOCK_SIZE, 1, file);
            bytes_read += BLOCK_SIZE;
            bytes_left -= BLOCK_SIZE;
        } else {
            // otherwise, read the remaining bytes
            fread(data_blocks[current_block].data, bytes_left, 1, file);
            bytes_read += bytes_left;
            bytes_left = 0;
        }
        previous_block = current_block;
    }
    inodes[inode_index].is_used = USED; // set inode as used
    superblock->used_user_space += file_size; // update used user space
    // update superblock, inodes, and data blocks in file
    fseek(fs, 0, SEEK_SET);
    fwrite(superblock, sizeof(SUPERBLOCK), 1, fs);
    fseek(fs, INODES_OFFSET, SEEK_SET);
    fwrite(inodes, sizeof(INODE), superblock->total_inodes, fs);
    fseek(fs, DATA_OFFSET, SEEK_SET);
    fwrite(data_blocks, sizeof(DATA_BLOCK), superblock->total_data_blocks, fs);
    fclose(file);
    fclose(fs);
}

void copy_file_from_fs(char* fs_name, char* file_name, char* output_path) {
    FILE* fs = fopen(fs_name, "r");
    if (fs == NULL) {
        printf("Error: could not open disk file.\n");
        return;
    }
    // load superblock, inodes, and data blocks into memory
    fseek(fs, 0, SEEK_SET);
    fread(superblock, sizeof(SUPERBLOCK), 1, fs);
    if (superblock->magic_number != MAGIC_NUMBER) {
        printf("Error: file is not a file system.\n");
        return;
    }
    fread(inodes, sizeof(INODE), superblock->total_inodes, fs);
    fread(data_blocks, sizeof(DATA_BLOCK), superblock->total_data_blocks, fs);
    int inode_index = get_inode_by_name(file_name);
    if (inode_index == -1) {
        printf("Error: file not found.\n");
        return;
    }
    FILE* file = fopen(output_path, "w");
    if (file == NULL) {
        printf("Error: could not open file.\n");
        return;
    }
    int current_block = inodes[inode_index].first_block; // get first block
    int bytes_left = inodes[inode_index].size;
    while (bytes_left > 0) {
        int bytes_to_write = bytes_left < BLOCK_SIZE ? bytes_left : BLOCK_SIZE; // write a block or the remaining bytes if less than a block
        fwrite(data_blocks[current_block].data, bytes_to_write, 1, file); // write data block to destination file
        bytes_left -= bytes_to_write;
        if (bytes_left > 0) {
            current_block = data_blocks[current_block].next_block; // get next block if there are still bytes left
            if (current_block == -1) {
                printf("Error: file data is corrupted.\n");
                return;
            }
        }
    }
    fclose(file);
    fclose(fs);
}

void list_files(char* fs_name) {
    FILE* fs = fopen(fs_name, "r");
    if (fs == NULL) {
        printf("Error: could not open disk file.\n");
        return;
    }
    // load superblock, inodes, and data blocks into memory
    fseek(fs, 0, SEEK_SET);
    fread(superblock, sizeof(SUPERBLOCK), 1, fs);
    if (superblock->magic_number != MAGIC_NUMBER) {
        printf("Error: file is not a file system.\n");
        return;
    }
    fread(inodes, sizeof(INODE), superblock->total_inodes, fs);
    fread(data_blocks, sizeof(DATA_BLOCK), superblock->total_data_blocks, fs);
    // print free user space
    printf("%d / %d bytes available\n", superblock->user_space - superblock->used_user_space, superblock->user_space);
    // print file names and sizes
    for (int i = 0; i < superblock->total_inodes; i++) {
        if (inodes[i].is_used == USED) {
            printf("File name: %-32.32s\t", inodes[i].name);
            printf("File size: %d bytes\n", inodes[i].size);
        }
    }
    fclose(fs);
}

void delete_file(char* fs_name, char* file_name) {
    FILE* fs = fopen(fs_name, "r+");
    if (fs == NULL) {
        printf("Error: could not open disk file.\n");
        return;
    }
    // load superblock, inodes, and data blocks into memory
    fseek(fs, 0, SEEK_SET);
    fread(superblock, sizeof(SUPERBLOCK), 1, fs);
    if (superblock->magic_number != MAGIC_NUMBER) {
        printf("Error: file is not a file system.\n");
        return;
    }
    fread(inodes, sizeof(INODE), superblock->total_inodes, fs);
    fread(data_blocks, sizeof(DATA_BLOCK), superblock->total_data_blocks, fs);
    int inode_index = get_inode_by_name(file_name);
    if (inode_index == -1) {
        printf("Error: file not found.\n");
        return;
    }
    int current_block = inodes[inode_index].first_block; // get first block
    // free data blocks by setting them as not used
    while (current_block != END_OF_FILE) {
        data_blocks[current_block].is_used = NOT_USED;
        current_block = data_blocks[current_block].next_block;
    }
    inodes[inode_index].is_used = NOT_USED; // set inode as not used
    superblock->used_user_space -= inodes[inode_index].size; // update used user space
    // update superblock, inodes, and data blocks in file
    fseek(fs, 0, SEEK_SET);
    fwrite(superblock, sizeof(SUPERBLOCK), 1, fs);
    fseek(fs, INODES_OFFSET, SEEK_SET);
    fwrite(inodes, sizeof(INODE), superblock->total_inodes, fs);
    fseek(fs, DATA_OFFSET, SEEK_SET);
    fwrite(data_blocks, sizeof(DATA_BLOCK), superblock->total_data_blocks, fs);
    fclose(fs);
}

void defragment_fs(char* fs_name) {
    FILE* fs = fopen(fs_name, "r+");
    if (fs == NULL) {
        printf("Error: could not open disk file.\n");
        return;
    }
    // load superblock, inodes, and data blocks into memory
    fseek(fs, 0, SEEK_SET);
    fread(superblock, sizeof(SUPERBLOCK), 1, fs);
    if (superblock->magic_number != MAGIC_NUMBER) {
        printf("Error: file is not a file system.\n");
        return;
    }
    fread(inodes, sizeof(INODE), superblock->total_inodes, fs);
    fread(data_blocks, sizeof(DATA_BLOCK), superblock->total_data_blocks, fs);
    // defragment data blocks - move used blocks to the beginning 
    for (int i = 0; i < superblock->total_data_blocks; i++) {
        if (data_blocks[i].is_used == NOT_USED) {
            for (int j = i + 1; j < superblock->total_data_blocks; j++) {
                if (data_blocks[j].is_used == USED) {
                    // find the inode that uses this data block
                    for (int k = 0; k < superblock->total_inodes; k++) {
                        if (inodes[k].first_block == j) {
                            // update the inode to point to the new data block
                            inodes[k].first_block = i;
                            break;
                        }
                    }
                    // copy data from used block to free block
                    memcpy(data_blocks[i].data, data_blocks[j].data, BLOCK_SIZE);
                    data_blocks[i].is_used = USED;
                    data_blocks[j].is_used = NOT_USED;
                    // update next_block values
                    if (data_blocks[j].next_block != END_OF_FILE) {
                        data_blocks[i].next_block = i + 1;
                    } else {
                        data_blocks[i].next_block = END_OF_FILE;
                    }
                    // find the data block that points to j and update it to point to i
                    for (int k = 0; k < superblock->total_data_blocks; k++) {
                        if (data_blocks[k].next_block == j) {
                            data_blocks[k].next_block = i;
                            break;
                        }
                    }
                    data_blocks[j].next_block = END_OF_FILE;
                    break;
                }
            }
        }
    }
    // update superblock, inodes, and data blocks in file
    fseek(fs, 0, SEEK_SET);
    fwrite(superblock, sizeof(SUPERBLOCK), 1, fs);
    fseek(fs, INODES_OFFSET, SEEK_SET);
    fwrite(inodes, sizeof(INODE), superblock->total_inodes, fs);
    fseek(fs, DATA_OFFSET, SEEK_SET);
    fwrite(data_blocks, sizeof(DATA_BLOCK), superblock->total_data_blocks, fs);
    fclose(fs);
}

void delete_fs(char* fs_name) {
    FILE* fs = fopen(fs_name, "r");
    if (fs == NULL) {
        printf("Error: could not open disk file.\n");
        return;
    }
    // load superblock into memory to check if it is a file system
    fseek(fs, 0, SEEK_SET);
    superblock = (SUPERBLOCK*) malloc(sizeof(SUPERBLOCK));
    fread(superblock, sizeof(SUPERBLOCK), 1, fs);
    if (superblock->magic_number != MAGIC_NUMBER) {
        printf("Error: file is not a file system.\n");
        return;
    }
    fclose(fs);
    remove(fs_name);
}

void usage_map(char* fs_name) {
    FILE* fs = fopen(fs_name, "r");
    if (fs == NULL) {
        printf("Error: could not open disk file.\n");
        return;
    }
    // load superblock, inodes, and data blocks into memory
    fseek(fs, 0, SEEK_SET);
    fread(superblock, sizeof(SUPERBLOCK), 1, fs);
    if (superblock->magic_number != MAGIC_NUMBER) {
        printf("Error: file is not a file system.\n");
        return;
    }
    fread(inodes, sizeof(INODE), superblock->total_inodes, fs);
    fread(data_blocks, sizeof(DATA_BLOCK), superblock->total_data_blocks, fs);
    // get superblock info
    printf("Superblock:\n");
    printf("Total size: %d bytes\n", superblock->total_size);
    printf("Total inodes: %d\n", superblock->total_inodes);
    printf("Total data blocks: %d\n", superblock->total_data_blocks);
    printf("User space: %d bytes\n", superblock->user_space);
    printf("Used user space: %d bytes\n", superblock->used_user_space);
    printf("Block size: %d bytes\n", superblock->block_size);
    printf("\n");
    // get inodes info
    printf("Inodes:\n");
    for (int i = 0; i < superblock->total_inodes; i++) {
        printf("\tInode %d:\n", i);
        printf("\t\tName: %-32.32s\t", inodes[i].name);
        printf("\t\tSize: %-6.6d bytes\t", inodes[i].size);
        printf("\t\tFirst block: %2.2d\t", inodes[i].first_block);
        printf("\t\tUsed: %s\n", inodes[i].is_used ? "yes" : "no");
    }
    printf("\n");
    // get data blocks info
    printf("Data blocks:\n");
    for (int i = 0; i < superblock->total_data_blocks; i++) {
        printf("\tData block %d:\n", i);
        printf("\t\tUsed: %s\t", data_blocks[i].is_used ? "yes" : "no");
        printf("\t\tNext block: %2.2d\n", data_blocks[i].next_block);
    }
    fclose(fs);
}

int get_free_inode() {
    for (int i = 0; i < superblock->total_inodes; i++) {
        if (inodes[i].is_used == NOT_USED) {
            return i;
        }
    }
    return -1;
}

int get_inode_by_name(char* file_name) {
    for (int i = 0; i < superblock->total_inodes; i++) {
        if (strcmp(inodes[i].name, file_name) == 0) {
            return i;
        }
    }
    return -1;
}