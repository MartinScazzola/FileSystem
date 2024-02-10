#include <sys/stat.h>
#include <time.h>

#define MAX_SIZE_NAME 100
#define MAX_DATA_COUNT 1000
#define MAX_SIZE_DATA_BLOCK 4096
#define MAX_INODE_COUNT 1000

#define FREE 0
#define ALLOCATED 1

#define REG_TYPE 1
#define DIR_TYPE 0

#define MAX_FS_NAME_SIZE 100

struct superblock {
	int inodes_count;
	int data_block_count;
	int *bitmap_inodes;
	int *bitmap_data;
};

struct inode {
	int inode_number;
	off_t size;
	pid_t pid;
	mode_t mode;
	time_t creation_time;
	time_t modification_time;
	time_t access_time;
	char path_name[MAX_SIZE_NAME];
	uid_t user_id;
	gid_t group_id;
	int block_index;
	int type;
};