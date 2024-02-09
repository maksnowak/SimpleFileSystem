#define MAX_FILES 16
#define MAX_FILE_NAME 32
#define BLOCK_SIZE 1024
#define SUPERBLOCK_OFFSET 0
#define INODES_OFFSET sizeof(SUPERBLOCK)
#define DATA_OFFSET (sizeof(INODE) * MAX_FILES) + INODES_OFFSET
#define NOT_USED 0
#define USED 1
#define END_OF_FILE -1
#define MAGIC_NUMBER 0x5016e171

typedef struct data_block {
    int is_used;
    char data[BLOCK_SIZE];
    int next_block;
} DATA_BLOCK;

typedef struct inode {
    char name[MAX_FILE_NAME];
    int size;
    int first_block;
    int is_used;
} INODE;

typedef struct superblock {
    int magic_number;
    int total_size;
    int total_inodes;
    int total_data_blocks;
    int user_space;
    int used_user_space;
    int block_size;
} SUPERBLOCK;

void init_fs(char* fs_name, int blocks);
void select_fs(char* fs_name);
void copy_file_to_fs(char* fs_name, char* path_to_file);
void copy_file_from_fs(char* fs_name, char* file_name, char* output_path);
void list_files(char* fs_name);
void delete_file(char* fs_name, char* file_name);
void defragment_fs(char* fs_name);
void delete_fs(char* fs_name);
void usage_map(char* fs_name);

int get_free_inode();
int get_inode_by_name(char* file_name);