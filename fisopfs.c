#define FUSE_USE_VERSION 30

#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/file.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include "fs_structs.h"

// Auxiliary functions prototypes
int create_generic_file(const char *path, mode_t mode, int type);
int entries_counter(const char *path);
int entries_finder(const char *string);
int find_inode(const char *path);
int save_filesystem_state();
int load_filesystem_state();

// Global File System variables
struct superblock SUPER_BLOCK;
int bitmap_inodes[MAX_INODE_COUNT];
int bitmap_data[MAX_DATA_COUNT];
struct inode inodes[MAX_INODE_COUNT];
static char data_blocks[MAX_DATA_COUNT][MAX_SIZE_DATA_BLOCK];
char filename[MAX_FS_NAME_SIZE];


// | FUSE FUNCTIONS |


static int
fisopfs_utimens(const char *path, const struct timespec tv[2])
{
	printf("[debug] fisop_utimens\n");
	int inode_index = find_inode(path);
	inodes[inode_index].access_time = tv[0].tv_sec;
	inodes[inode_index].modification_time = tv[1].tv_sec;
	return 0;
}

static void *
fisopfs_init(struct fuse_conn_info *conn)
{
	printf("[debug] fisop_init\n");
	FILE *file = fopen(filename, "rb");
	if (!file) {
		SUPER_BLOCK.inodes_count = MAX_INODE_COUNT;
		SUPER_BLOCK.data_block_count = MAX_DATA_COUNT;
		SUPER_BLOCK.bitmap_inodes = bitmap_inodes;
		SUPER_BLOCK.bitmap_data = bitmap_data;
		memset(SUPER_BLOCK.bitmap_inodes, 0, sizeof(int) * MAX_INODE_COUNT);
		memset(SUPER_BLOCK.bitmap_data, 0, sizeof(int) * MAX_DATA_COUNT);
	} else {
		load_filesystem_state();
	}

	return (void *) 0;
}

static int
fisopfs_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_create(%s)\n", path);
	if (strlen(path) > MAX_SIZE_NAME) {
		errno = ENAMETOOLONG;
		return ENAMETOOLONG;
	}

	int result = create_generic_file(path, mode, REG_TYPE);

	return result;
}

static int
fisopfs_mkdir(const char *path, mode_t mode)
{
	printf("[debug] fisopfs_create(%s)\n", path);
	if (strlen(path) > MAX_SIZE_NAME) {
		errno = ENAMETOOLONG;
		return ENAMETOOLONG;
	}

	int result = create_generic_file(path, mode, DIR_TYPE);

	return result;
}

static int
fisopfs_getattr(const char *path, struct stat *st)
{
	printf("[debug] fisopfs_getattr - path: %s\n", path);

	if (strcmp(path, "/") == 0) {
		st->st_uid = 1717;
		st->st_mode = __S_IFDIR | 0755;
		st->st_nlink = 2;
	} else {
		int inode_index = find_inode(path);

		if (inode_index < 0) {
			return -ENOENT;
		}

		st->st_uid = inodes[inode_index].user_id;
		st->st_mode = __S_IFDIR | 0755;
		st->st_nlink = 2;
		st->st_mtime = inodes[inode_index].modification_time;
		st->st_atime = inodes[inode_index].access_time;

		if (inodes[inode_index].type == REG_TYPE) {
			st->st_size = inodes[inode_index].size;
			st->st_mode = __S_IFREG | 0644;
			st->st_nlink = 1;
			st->st_blksize = MAX_SIZE_DATA_BLOCK;
			st->st_blocks =
			        1;  // The inode only stores one block in this filesystem
		}
	}

	return 0;
}

static int
fisopfs_readdir(const char *path,
                void *buffer,
                fuse_fill_dir_t filler,
                off_t offset,
                struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_readdir - path: %s\n", path);

	// Los directorios '.' y '..'
	filler(buffer, ".", NULL, 0);
	filler(buffer, "..", NULL, 0);

	// Si nos preguntan por el directorio raiz, mostrar los archivos y subdirectorios
	if (strcmp(path, "/") == 0) {
		for (int i = 0; i < MAX_INODE_COUNT; i++) {
			if (SUPER_BLOCK.bitmap_inodes[i] == ALLOCATED &&
			    entries_counter(inodes[i].path_name) == 1) {
				filler(buffer, inodes[i].path_name + 1, NULL, 0);
			}
		}
		return 0;
	}
	int inode_idx = find_inode(path);
	if (inode_idx < 0) {
		return -ENOENT;
	}

	int slashes = entries_counter(path);

	for (int i = 0; i < MAX_INODE_COUNT; i++) {
		if (SUPER_BLOCK.bitmap_inodes[i] == ALLOCATED &&
		    strstr(inodes[i].path_name, path) != NULL) {
			if (entries_counter(inodes[i].path_name) == slashes + 1) {
				filler(buffer,
				       inodes[i].path_name +
				               entries_finder(inodes[i].path_name),
				       NULL,
				       0);  // Agregar el nombre del archivo o subdirectorio al buffer
			}
		}
	}

	return 0;
}

static int
fisopfs_write(const char *path,
              const char *buf,
              size_t size,
              off_t offset,
              struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_write - path: %s\n", path);

	int inode_index = find_inode(path);
	if (inode_index < 0) {
		return -ENOENT;
	}

	if (offset + size > MAX_SIZE_DATA_BLOCK) {
		return -ENOSPC;
	}

	memcpy(data_blocks[inodes[inode_index].block_index] + offset, buf, size);

	if (offset + size > inodes[inode_index].size) {
		inodes[inode_index].size = offset + size;
	}

	inodes[inode_index].modification_time = time(NULL);

	return size;
}

static int
fisopfs_truncate(const char *path, off_t size)
{
	printf("[debug] fisopfs_truncate(%s, %ld)\n", path, size);

	int inode_index = find_inode(path);

	if (inode_index < 0) {
		return -ENOENT;
	}

	if (S_ISDIR(inodes[inode_index].mode)) {
		return -EISDIR;
	}

	if (S_ISLNK(inodes[inode_index].mode)) {
		return -EPERM;
	}

	if (size < 0) {
		return -EINVAL;
	}

	if (inodes[inode_index].block_index > -1) {
		int block_index = inodes[inode_index].block_index;
		if (size >= MAX_SIZE_DATA_BLOCK) {
			memset(data_blocks[block_index], 0, MAX_SIZE_DATA_BLOCK);
			inodes[inode_index].block_index = -1;
			inodes[inode_index].size = 0;
			bitmap_data[block_index] = FREE;
		} else {
			memset(data_blocks[block_index] + size,
			       0,
			       MAX_SIZE_DATA_BLOCK - size);
		}
	}

	inodes[inode_index].access_time = time(NULL);
	inodes[inode_index].modification_time = inodes[inode_index].access_time;
	return 0;
}

static int
fisopfs_flush(const char *path, struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_flush - path: %s\n", path);
	save_filesystem_state();
	return 0;
}

static void
fisopfs_destroy(void *private_data)
{
	printf("[debug] fisopfs_destroy - path: %p\n", private_data);
	save_filesystem_state();
}

static int
fisopfs_read(const char *path,
             char *buffer,
             size_t size,
             off_t offset,
             struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_read - path: %s, offset: %lu, size: %lu\n",
	       path,
	       offset,
	       size);

	int inode_index = find_inode(path);

	if (inode_index < 0) {
		return -ENOENT;
	}

	inodes[inode_index].access_time = time(NULL);

	off_t remaining_size = MAX_SIZE_DATA_BLOCK - offset;

	if (remaining_size <= 0) {
		return 0;  // Offset is at or beyond the end of the file
	}

	size_t bytes_to_read = size;
	if (remaining_size < bytes_to_read) {
		bytes_to_read = remaining_size;
	}


	int data_block_index = inodes[inode_index].block_index;

	if (data_block_index < 0) {
		return -ENOENT;
	}

	memcpy(buffer, data_blocks[data_block_index] + offset, bytes_to_read);


	return bytes_to_read;
}

static int
fisopfs_unlink(const char *path)
{
	printf("[debug] fisopfs_unlink(%s)\n", path);

	int inode_index = find_inode(path);
	if (inode_index < 0) {
		errno = ENOENT;
		return -ENOENT;
	}

	// Check if file is a DIR
	if (inodes[inode_index].type == DIR_TYPE) {
		errno = EISDIR;
		return -EISDIR;
	}

	// Free it's inodes and data blocks
	SUPER_BLOCK.bitmap_inodes[inode_index] = FREE;

	for (int i = 0; i < MAX_INODE_COUNT; i++) {
		int data_block_index = inodes[inode_index].block_index;
		if (data_block_index >= 0) {
			SUPER_BLOCK.bitmap_data[data_block_index] = FREE;
			memset(data_blocks[data_block_index],
			       0,
			       MAX_SIZE_DATA_BLOCK);
		}
	}

	// Free name
	memset(inodes[inode_index].path_name, 0, MAX_SIZE_NAME);
	return 0;
}

static int
fisopfs_rmdir(const char *path)
{
	printf("[debug] fisopfs_rmdir(%s)\n", path);

	int inode_index = find_inode(path);
	if (inode_index < 0) {
		errno = ENOENT;
		return -ENOENT;
	}

	// Check if the file is not a DIR
	if (inodes[inode_index].type != DIR_TYPE) {
		errno = ENOTDIR;
		return -ENOTDIR;
	}


	//  Free it's inodes and data blocks
	SUPER_BLOCK.bitmap_inodes[inode_index] = FREE;

	for (int i = 0; i < MAX_INODE_COUNT; i++) {
		int data_block_index = inodes[inode_index].block_index;
		if (data_block_index >= 0) {
			SUPER_BLOCK.bitmap_data[data_block_index] = FREE;
			memset(data_blocks[data_block_index],
			       0,
			       MAX_SIZE_DATA_BLOCK);
		}
	}

	// Free name
	memset(inodes[inode_index].path_name, 0, MAX_SIZE_NAME);
	return 0;
}

static struct fuse_operations operations = {
	.getattr = fisopfs_getattr,
	.readdir = fisopfs_readdir,
	.read = fisopfs_read,
	.create = fisopfs_create,
	.init = fisopfs_init,
	.utimens = fisopfs_utimens,
	.write = fisopfs_write,
	.truncate = fisopfs_truncate,
	.flush = fisopfs_flush,
	.mkdir = fisopfs_mkdir,
	.rmdir = fisopfs_rmdir,
	.unlink = fisopfs_unlink,
	.destroy = fisopfs_destroy,
};


// Main function
int
main(int argc, char *argv[])
{
	char f_name[MAX_FS_NAME_SIZE];

	if (argc == 2) {
		strcpy(f_name, "filesystem");
	} else {
		strcpy(f_name, argv[argc - 1]);
	}

	strcat(f_name, ".fisopfs");

	strcpy(filename, f_name);

	return fuse_main(argc, argv, &operations, NULL);
}


// Auxiliary Functions Implementations

// Count the occurrences of the character "/" in a path
int
entries_counter(const char *path)
{
	int occurrences = 0;

	for (int i = 0; i < strlen(path); i++) {
		if (path[i] == '/') {
			occurrences++;
		}
	}
	return occurrences;
}

// Finds the last slash ("/") index in a path (char *)
int
entries_finder(const char *path)
{
	int occurrences = 0;

	for (int i = 0; i < strlen(path); i++) {
		if (path[i] == '/') {
			occurrences = i;
		}
	}
	return occurrences + 1;
}


// Finds the inode index associated to the given path
int
find_inode(const char *path)
{
	for (int i = 0; i < MAX_INODE_COUNT; i++) {
		if (SUPER_BLOCK.bitmap_inodes[i] == ALLOCATED &&
		    strcmp(inodes[i].path_name, path) == 0) {
			return i;
		}
	}
	return -1;
}


// It creates a file or a directory in the specificated path
// If type is REG_TYPE, it creates a file
// If type is DIR_TYPE, it creates a directory
int
create_generic_file(const char *path, mode_t mode, int type)
{
	int inode_index = 0;
	int block_index = 0;

	while (inode_index < MAX_INODE_COUNT &&
	       SUPER_BLOCK.bitmap_inodes[inode_index] != FREE) {
		inode_index++;
	}

	if (inode_index >= MAX_INODE_COUNT) {
		return -ENOMEM;
	}

	while (block_index < MAX_DATA_COUNT &&
	       SUPER_BLOCK.bitmap_data[block_index] != FREE) {
		block_index++;
	}

	if (block_index >= MAX_DATA_COUNT) {
		return -ENOMEM;
	}

	SUPER_BLOCK.bitmap_inodes[inode_index] = ALLOCATED;
	SUPER_BLOCK.bitmap_data[block_index] = ALLOCATED;

	inodes[inode_index].inode_number = inode_index;
	inodes[inode_index].mode = mode;
	inodes[inode_index].creation_time = time(NULL);
	inodes[inode_index].modification_time = inodes[inode_index].creation_time;
	inodes[inode_index].access_time = inodes[inode_index].creation_time;
	inodes[inode_index].block_index = block_index;
	inodes[inode_index].user_id = getuid();
	inodes[inode_index].user_id = getgid();
	inodes[inode_index].size = 0;


	inodes[inode_index].type = type;


	strcpy(inodes[inode_index].path_name, path);

	return 0;
}


// This function saves the file system state in a file
int
save_filesystem_state()
{
	FILE *file = fopen(filename, "wb");
	if (file == NULL) {
		printf("Error al abrir el archivo para guardar el estado del "
		       "sistema de archivos.\n");
		return -1;
	}

	// Write SUPER_BLOCK Struct
	if (fwrite(&SUPER_BLOCK, sizeof(struct superblock), 1, file) != 1) {
		return -1;
	}
	// Write Bitmaps of Inodes and Blocks
	for (int i = 0; i < MAX_DATA_COUNT; i++) {
		if (fwrite(&bitmap_data[i], sizeof(bitmap_data[i]), 1, file) != 1) {
			return -1;
		}
	}

	for (int i = 0; i < MAX_INODE_COUNT; i++) {
		if (fwrite(&bitmap_inodes[i], sizeof(bitmap_inodes[i]), 1, file) !=
		    1) {
			return -1;
		}
	}

	// Write Inode Structs
	for (int i = 0; i < MAX_INODE_COUNT; i++) {
		if (fwrite(&inodes[i], sizeof(inodes[i]), 1, file) != 1) {
			return -1;
		}
	}

	// Write Data Blocks
	for (int i = 0; i < MAX_DATA_COUNT; i++) {
		if (fwrite(&data_blocks[i], sizeof(data_blocks[i]), 1, file) != 1) {
			return -1;
		}
	}

	fclose(file);
	return 0;
}

// Function for loading the filesystem from a file
int
load_filesystem_state()
{
	FILE *file = fopen(filename, "rb");
	if (!file) {
		printf("Error al abrir el archivo para cargar el estado del "
		       "sistema de archivos.\n");
		return -1;
	}

	// Read SUPER_BLOCK struct
	if (fread(&SUPER_BLOCK, sizeof(struct superblock), 1, file) != 1) {
		return -1;
	}

	// Read Bitmaps of Inodes and Blocks
	for (int i = 0; i < MAX_DATA_COUNT; i++) {
		if (fread(&bitmap_data[i], sizeof(bitmap_data[i]), 1, file) != 1) {
			return -1;
		}
	}
	SUPER_BLOCK.bitmap_data = bitmap_data;


	for (int i = 0; i < MAX_INODE_COUNT; i++) {
		if (fread(&bitmap_inodes[i], sizeof(bitmap_inodes[i]), 1, file) !=
		    1) {
			return -1;
		}
	}
	SUPER_BLOCK.bitmap_inodes = bitmap_inodes;

	// Read Inodes Struct
	for (int i = 0; i < MAX_INODE_COUNT; i++) {
		if (fread(&inodes[i], sizeof(inodes[i]), 1, file) != 1) {
			return -1;
		}
	}
	// Read Data Blocks
	for (int i = 0; i < MAX_DATA_COUNT; i++) {
		if (fread(&data_blocks[i], sizeof(data_blocks[i]), 1, file) != 1) {
			return -1;
		}
	}

	fclose(file);
	return 0;
}
