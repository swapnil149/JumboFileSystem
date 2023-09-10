#include "jumbo_file_system.h"
#include<stdio.h>
#include<string.h>
// C does not have a bool type, so I created one that you can use
typedef char bool_t;
#define TRUE 1
#define FALSE 0


static block_num_t current_dir;


// optional helper function you can implement to tell you if a block is a dir node or an inode
static bool_t is_dir(block_num_t block_num) {
  return TRUE;
}
/* jfs_mount
 *   prepares the DISK file on the _real_ file system to have file system
 *   blocks read and written to it.  The application _must_ call this function
 *   exactly once before calling any other jfs_* functions.  If your code
 *   requires any additional one-time initialization before any other jfs_*
 *   functions are called, you can add it here.
 * filename - the name of the DISK file on the _real_ file system
 * returns 0 on success or -1 on error; errors should only occur due to
 *   errors in the underlying disk syscalls.
 */
int jfs_mount(const char* filename) {
  int ret = bfs_mount(filename);
  current_dir = 1;
  return ret;
}


/* jfs_mkdir
 *   creates a new subdirectory in the current directory
 * directory_name - name of the new subdirectory
 * returns 0 on success or one of the following error codes on failure:
 *   E_EXISTS, E_MAX_NAME_LENGTH, E_MAX_DIR_ENTRIES, E_DISK_FULL
 */
int jfs_mkdir(const char* directory_name) {
  struct block current_dir_block;
  if(read_block(current_dir, &current_dir_block) != 0){
    return E_UNKNOWN;
  }
  // Check if the subdirectory already exists in the current directory.
  for (int i = 0; i < current_dir_block.contents.dirnode.num_entries; i++) {
    if (strcmp(directory_name, current_dir_block.contents.dirnode.entries[i].name) == 0) {
      return E_EXISTS;
    }
  }
  // check if the name of the subdirectory is valid.
  if (strlen(directory_name) > MAX_NAME_LENGTH) {
    return E_MAX_NAME_LENGTH;
  }
  // Check if there is space in the directory for a new entry.
  if (current_dir_block.contents.dirnode.num_entries >= MAX_DIR_ENTRIES) {
    return E_MAX_DIR_ENTRIES;
  }
  // allocate a new block for the directory's contents
  block_num_t dir_block_num = allocate_block();
  if (dir_block_num == 0) {
    release_block(dir_block_num);
    return E_DISK_FULL;
  }
  // initialize the directory's contents
  struct block dir_block;
  dir_block.is_dir = 0;
  dir_block.contents.dirnode.num_entries = 0;
  // dir_block.contents.dirnode.entries[MAX_DIR_ENTRIES]

  current_dir_block.contents.dirnode.entries[current_dir_block.contents.dirnode.num_entries].block_num = dir_block_num;
  strcpy(current_dir_block.contents.dirnode.entries[current_dir_block.contents.dirnode.num_entries].name, directory_name);
  current_dir_block.contents.dirnode.num_entries++;
  
  if (write_block(current_dir, &current_dir_block) != 0) {
    return E_UNKNOWN;
  }
  if (write_block(dir_block_num, &dir_block) != 0) {
    return E_UNKNOWN;
  }
  return E_SUCCESS;
}


/* jfs_chdir
 *   changes the current directory to the specified subdirectory, or changes
 *   the current directory to the root directory if the directory_name is NULL
 * directory_name - name of the subdirectory to make the current
 *   directory; if directory_name is NULL then the current directory
 *   should be made the root directory instead
 * returns 0 on success or one of the following error codes on failure:
 *   E_NOT_EXISTS, E_NOT_DIR
 */
int jfs_chdir(const char* directory_name) {
  struct block current_dir_block;
  if(read_block(current_dir, &current_dir_block) != 0){
    return E_UNKNOWN;
  }
  if(directory_name == NULL){
    current_dir = 1;
    return E_SUCCESS;
  }

  block_num_t dir_block_num = 0;
  // Check if the subdirectory exists
  for (int i = 0; i < current_dir_block.contents.dirnode.num_entries; i++) {
    if (strcmp(directory_name, current_dir_block.contents.dirnode.entries[i].name) == 0) {
      dir_block_num = current_dir_block.contents.dirnode.entries[i].block_num;
      break;
    }
  }
  if(!dir_block_num){
    return E_NOT_EXISTS;
  }
  struct block dir_block;
  if(read_block(dir_block_num, &dir_block) != 0){
    return E_UNKNOWN;
  }
  if (dir_block.is_dir != 0) {
    return E_NOT_DIR;
  }
  current_dir = dir_block_num;
  return E_SUCCESS;
}


/* jfs_ls
 *   finds the names of all the files and directories in the current directory
 *   and writes the directory names to the directories argument and the file
 *   names to the files argument
 * directories - array of strings; the function will set the strings in the
 *   array, followed by a NULL pointer after the last valid string; the strings
 *   should be malloced and the caller will free them
 * file - array of strings; the function will set the strings in the
 *   array, followed by a NULL pointer after the last valid string; the strings
 *   should be malloced and the caller will free them
 * returns 0 on success or one of the following error codes on failure:
 *   (this function should always succeed)
 */
int jfs_ls(char* directories[MAX_DIR_ENTRIES+1], char* files[MAX_DIR_ENTRIES+1]) {
  struct block current_dir_block;
  if(read_block(current_dir, &current_dir_block) != 0){
    return E_UNKNOWN;
  }
  int dir_count = 0;
  int file_count = 0;
  if(current_dir_block.contents.dirnode.num_entries > 0) {
    struct block dir_block;
    block_num_t dir_block_num;
    for (int i = 0; i < current_dir_block.contents.dirnode.num_entries; i++) {
      dir_block_num = current_dir_block.contents.dirnode.entries[i].block_num;
      if(read_block(dir_block_num, &dir_block) != 0){
        return E_UNKNOWN;
      }
      if(dir_block.is_dir == 0){
        directories[dir_count] = (char*) malloc((MAX_NAME_LENGTH + 1) * sizeof(char));
        strcpy(directories[dir_count], current_dir_block.contents.dirnode.entries[i].name);
        dir_count = dir_count + 1;
      } else {
        files[file_count] = (char*) malloc((MAX_NAME_LENGTH + 1) * sizeof(char));
        strcpy(files[file_count], current_dir_block.contents.dirnode.entries[i].name);
        file_count = file_count + 1;
      }
    }
  }
  directories[dir_count] = NULL;
  files[file_count] = NULL;
  return E_SUCCESS;
}


/* jfs_rmdir
 *   removes the specified subdirectory of the current directory
 * directory_name - name of the subdirectory to remove
 * returns 0 on success or one of the following error codes on failure:
 *   E_NOT_EXISTS, E_NOT_DIR, E_NOT_EMPTY
 */
int jfs_rmdir(const char* directory_name) {
  struct block current_dir_block;
  if (read_block(current_dir, &current_dir_block) != 0) {
    return E_UNKNOWN;
  }
  // Find the entry in the current directory corresponding to the directory to remove.
  int entry_idx = -1;
  for (int i = 0; i < current_dir_block.contents.dirnode.num_entries; i++) {
    if (strcmp(directory_name, current_dir_block.contents.dirnode.entries[i].name) == 0) {
      entry_idx = i;
      break;
    }
  }
  if (entry_idx == -1) {
    return E_NOT_EXISTS;
  }
  // Get the block number of the directory to remove.
  block_num_t dir_block_num = current_dir_block.contents.dirnode.entries[entry_idx].block_num;
  // Read the directory block.
  struct block dir_block;
  if (read_block(dir_block_num, &dir_block) != 0) {
    return E_UNKNOWN;
  }
  if (dir_block.is_dir != 0) {
    // Not a directory.
    return E_NOT_DIR;
  }
  if (dir_block.contents.dirnode.num_entries > 0) {
    // Directory is not empty.
    return E_NOT_EMPTY;
  }
  // Remove the directory entry from the current directory.
  for (int i = entry_idx; i < current_dir_block.contents.dirnode.num_entries - 1; i++) {
    current_dir_block.contents.dirnode.entries[i] = current_dir_block.contents.dirnode.entries[i+1];
  }
  current_dir_block.contents.dirnode.num_entries--;
  // Write the updated current directory block.
  if (write_block(current_dir, &current_dir_block) != 0) {
    return E_UNKNOWN;
  }
  // Release the directory block.
  if (release_block(dir_block_num) != 0) {
    return E_UNKNOWN;
  }
  return E_SUCCESS;
}


/* jfs_creat
 *   creates a new, empty file with the specified name
 * file_name - name to give the new file
 * returns 0 on success or one of the following error codes on failure:
 *   E_EXISTS, E_MAX_NAME_LENGTH, E_MAX_DIR_ENTRIES, E_DISK_FULL
 */
int jfs_creat(const char* file_name) {
   // check if the name of the file is valid.
  if (strlen(file_name) > MAX_NAME_LENGTH) {
      return E_MAX_NAME_LENGTH; // file name is too long
  }
  struct block current_dir_block;
  if(read_block(current_dir, &current_dir_block) != 0){
    return E_UNKNOWN;
  }
  // Check if the file already exists in the directory.
  for (int i = 0; i < current_dir_block.contents.dirnode.num_entries; i++) {
    if (strcmp(file_name, current_dir_block.contents.dirnode.entries[i].name) == 0) {
      return E_EXISTS;
    }
  }
  // Check if there is space in the directory for a new entry.
   if (current_dir_block.contents.dirnode.num_entries >= MAX_DIR_ENTRIES) {
    return E_MAX_DIR_ENTRIES;
  }
  // allocate a new block for the file's contents
  block_num_t file_block_num = allocate_block();
  if (file_block_num == 0) {
    release_block(file_block_num);
    return E_DISK_FULL;
  }
  // initialize the directory's contents
  struct block file_block;
  file_block.is_dir = 1;
  file_block.contents.inode.file_size = 0;

  current_dir_block.contents.dirnode.entries[current_dir_block.contents.dirnode.num_entries].block_num = file_block_num;
  strcpy(current_dir_block.contents.dirnode.entries[current_dir_block.contents.dirnode.num_entries].name, file_name);
  current_dir_block.contents.dirnode.num_entries++;
  
  if (write_block(current_dir, &current_dir_block) != 0) {
    return E_UNKNOWN;
  }
  if (write_block(file_block_num, &file_block) != 0) {
    return E_UNKNOWN;
  }
  return E_SUCCESS;

}


/* jfs_remove
 *   deletes the specified file and all its data (note that this cannot delete
 *   directories; use rmdir instead to remove directories)
 * file_name - name of the file to remove
 * returns 0 on success or one of the following error codes on failure:
 *   E_NOT_EXISTS, E_IS_DIR
 */
int jfs_remove(const char* file_name) {
  struct block current_dir_block;
  if(read_block(current_dir, &current_dir_block) != 0){
    return E_UNKNOWN;
  }
  // Check if the file exists in the directory.
  int file_index = -1;
  for (int i = 0; i < current_dir_block.contents.dirnode.num_entries; i++) {
    if (strcmp(file_name, current_dir_block.contents.dirnode.entries[i].name) == 0) {
      file_index = i;
      break;
    }
  }
  if (file_index == -1) {
    return E_NOT_EXISTS;
  }
  // Get the file's block number.
  block_num_t file_block_num = current_dir_block.contents.dirnode.entries[file_index].block_num;

  struct block file_block;
  if (read_block(file_block_num, &file_block) != 0){
    return E_UNKNOWN;
  }
  // Check if the file is a directory.
  if (file_block.is_dir == 0) {
    return E_IS_DIR;
  }

  // Remove file from the current directory.
  for (int i = file_index; i < current_dir_block.contents.dirnode.num_entries - 1; i++) {
    current_dir_block.contents.dirnode.entries[i] = current_dir_block.contents.dirnode.entries[i+1];
  }
  current_dir_block.contents.dirnode.num_entries--;
  // Write the updated current directory block.
  if (write_block(current_dir, &current_dir_block) != 0) {
    return E_UNKNOWN;
  }
  // Release the file block.
  if(file_block.contents.inode.file_size > 0){
    int num_data_blocks = file_block.contents.inode.file_size / BLOCK_SIZE;
    if(file_block.contents.inode.file_size %  BLOCK_SIZE != 0) {
      num_data_blocks++;
    }
    for(int i = 0; i < num_data_blocks; i++){
      if(release_block(file_block.contents.inode.data_blocks[i]) != 0){
        return E_UNKNOWN;
      }
    }
  }
  if (release_block(file_block_num) != 0) {
    return E_UNKNOWN;
  }
  return E_SUCCESS;
}


/* jfs_stat
 *   returns the file or directory stats (see struct stat for details)
 * name - name of the file or directory to inspect
 * buf  - pointer to a struct stat (already allocated by the caller) where the
 *   stats will be written
 * returns 0 on success or one of the following error codes on failure:
 *   E_NOT_EXISTS
 */
int jfs_stat(const char* name, struct stats* buf) {
  struct block current_dir_block;
  if(read_block(current_dir, &current_dir_block) != 0){
    return E_UNKNOWN;
  }
  // Check if the file exists in the directory.
  int index = -1;
  for (int i = 0; i < current_dir_block.contents.dirnode.num_entries; i++) {
    if (strcmp(name, current_dir_block.contents.dirnode.entries[i].name) == 0) {
      index = i;
      break;
    }
  }
  if (index == -1) {
    return E_NOT_EXISTS;
  }
  // Get the directory or file's block number.
  block_num_t dirFile_block_num = current_dir_block.contents.dirnode.entries[index].block_num;

  struct block dirFile_block;
  if (read_block(dirFile_block_num, &dirFile_block) != 0){
    return E_UNKNOWN;
  }

  // Fill in the struct stats
  buf->block_num = dirFile_block_num;
  buf->is_dir = dirFile_block.is_dir;
  strncpy(buf->name, name, MAX_NAME_LENGTH + 1);
  if (dirFile_block.is_dir != 0) {
    if(dirFile_block.contents.inode.file_size > 0){
      int num_data_blocks = dirFile_block.contents.inode.file_size / BLOCK_SIZE;
      if(dirFile_block.contents.inode.file_size %  BLOCK_SIZE != 0) {
        num_data_blocks++;
      }
      buf->num_data_blocks = num_data_blocks;
    } else {
      buf->num_data_blocks = 0;
    }
    buf->file_size = dirFile_block.contents.inode.file_size;
  }
  return E_SUCCESS;
}


/* jfs_write
 *   appends the data in the buffer to the end of the specified file
 * file_name - name of the file to append data to
 * buf - buffer containing the data to be written (note that the data could be
 *   binary, not text, and even if it is text should not be assumed to be null
 *   terminated)
 * count - number of bytes in buf (write exactly this many)
 * returns 0 on success or one of the following error codes on failure:
 *   E_NOT_EXISTS, E_IS_DIR, E_MAX_FILE_SIZE, E_DISK_FULL
 */
int jfs_write(const char* file_name, const void* buf, unsigned short count) {
  struct block current_dir_block;
  if(read_block(current_dir, &current_dir_block) != 0){
    return E_UNKNOWN;
  }
  // Check if the file exists
  int file_index = -1;
  for (int i = 0; i < current_dir_block.contents.dirnode.num_entries; i++) {
    if (strcmp(file_name, current_dir_block.contents.dirnode.entries[i].name) == 0) {
      file_index = i;
      break;
    }
  }
  if (file_index == -1) {
    return E_NOT_EXISTS;
  }
  block_num_t file_block_num = current_dir_block.contents.dirnode.entries[file_index].block_num;
  struct block file_block;
  if (read_block(file_block_num, &file_block) != 0){
    return E_UNKNOWN;
  }
  if(file_block.is_dir == 0){
    return E_IS_DIR;
  }
  
  if(file_block.contents.inode.file_size + count > MAX_FILE_SIZE){
    return E_MAX_FILE_SIZE;
  }
  
  unsigned short file_size = file_block.contents.inode.file_size; 
  unsigned short num_data_blocks_used = file_size / BLOCK_SIZE; 
  if(file_size %  BLOCK_SIZE != 0) {
    num_data_blocks_used++;
  }
  unsigned short total_num_data_blocks = (file_size + count)/BLOCK_SIZE;
  if((file_size + count) %  BLOCK_SIZE != 0) {
    total_num_data_blocks++;
  }
  unsigned short num_data_blocks_needed =  total_num_data_blocks - num_data_blocks_used;

  block_num_t data_block_num_list[num_data_blocks_needed];
  //check whether disk is full or not
  int index = -1;
  for (int i = 0; i < num_data_blocks_needed; i++) {
    // allocate a new block for the file data
    block_num_t new_data_block_num = allocate_block();
    data_block_num_list[i] = new_data_block_num;
    if (new_data_block_num == 0) {
      index = i; 
      break;
    }
  }
  if(index != -1){
    for (int i = 0; i < index + 1; i++) {
    // release allocated blocks for the file data
      release_block(data_block_num_list[i]);
    }
    return E_DISK_FULL;
  }
  char* data = (char*)buf;
  if((num_data_blocks_needed == 0) || (file_size % BLOCK_SIZE) > 0){
    char* data_block_buf = malloc(BLOCK_SIZE);
    char* original_data_block_buf = data_block_buf;
    memset(data_block_buf, 0, BLOCK_SIZE);
    read_block(file_block.contents.inode.data_blocks[num_data_blocks_used-1], data_block_buf);
    data_block_buf += file_size % BLOCK_SIZE;
    unsigned short rem_block_size = BLOCK_SIZE - (file_size % BLOCK_SIZE);
    num_data_blocks_needed == 0 ? memcpy(data_block_buf, data, count) : memcpy(data_block_buf, data, rem_block_size);
    if(num_data_blocks_needed > 0 && ((file_size % BLOCK_SIZE) > 0)){
      data += rem_block_size;
    }
    write_block(file_block.contents.inode.data_blocks[num_data_blocks_used-1], original_data_block_buf);
    free(original_data_block_buf);
  }
  int curr_block_index = num_data_blocks_used == 0 ? 0 : num_data_blocks_used;
  unsigned short curr_byte = file_size;
  for (int i = 0; i < num_data_blocks_needed; i++) {
    char* data_block_buf = malloc(BLOCK_SIZE);
    memset(data_block_buf, 0, BLOCK_SIZE);
    if (i == num_data_blocks_needed - 1 && i != 0) {
      // this is the last block, so only write the remaining data
      unsigned short rem_block_size = (count % BLOCK_SIZE) + (file_size % BLOCK_SIZE);
      if (rem_block_size == 0) {
        rem_block_size = BLOCK_SIZE;
      }
      memcpy(data_block_buf, data, rem_block_size); 
    } else {
      if(i == 0 && count <= BLOCK_SIZE){
        memcpy(data_block_buf, data, count);
        data += count;
      } else{
      // write the entire block of data
        memcpy(data_block_buf, data, BLOCK_SIZE);
        data += BLOCK_SIZE;
      }
    }
    file_block.contents.inode.data_blocks[curr_block_index + i] = data_block_num_list[i];
    write_block(data_block_num_list[i],data_block_buf);
    free(data_block_buf);
  }
  // update the inode with the new file size
  file_block.contents.inode.file_size += count;
  // write the inode block back to the disk
  if (write_block(file_block_num, &file_block) != 0) {
    return E_UNKNOWN;
  }
  return E_SUCCESS;
}

/* jfs_read
 *   reads the specified file and copies its contents into the buffer, up to a
 *   maximum of *ptr_count bytes copied (but obviously no more than the file
 *   size, either)
 * file_name - name of the file to read
 * buf - buffer where the file data should be written
 * ptr_count - pointer to a count variable (allocated by the caller) that
 *   contains the size of buf when it's passed in, and will be modified to
 *   contain the number of bytes actually written to buf (e.g., if the file is
 *   smaller than the buffer) if this function is successful
 * returns 0 on success or one of the following error codes on failure:
 *   E_NOT_EXISTS, E_IS_DIR
 */
int jfs_read(const char* file_name, void* buf, unsigned short* ptr_count) {
  struct block current_dir_block;
  if(read_block(current_dir, &current_dir_block) != 0){
    return E_UNKNOWN;
  }
  // Check if the file exists
  int file_index = -1;
  for (int i = 0; i < current_dir_block.contents.dirnode.num_entries; i++) {
    if (strcmp(file_name, current_dir_block.contents.dirnode.entries[i].name) == 0) {
      file_index = i;
      break;
    }
  }
  if (file_index == -1) {
    return E_NOT_EXISTS;
  }

  block_num_t file_block_num = current_dir_block.contents.dirnode.entries[file_index].block_num;

  struct block file_block;
  if (read_block(file_block_num, &file_block) != 0){
    return E_UNKNOWN;
  }
  if(file_block.is_dir == 0){
    return E_IS_DIR;
  }

  unsigned short file_size = file_block.contents.inode.file_size;
  unsigned short count = *ptr_count;

  if (count > file_size) {
    count = file_size;
  }

  unsigned short num_blocks_to_read = count / BLOCK_SIZE;
  if (count % BLOCK_SIZE != 0) {
    num_blocks_to_read++;
  }

  unsigned short total_bytes_read = 0;
  char* data = (char*) buf;
  char* initial_data = data;
  for (int i = 0; i < num_blocks_to_read; i++) {
    char* data_block_buf = malloc(BLOCK_SIZE);
    memset(data_block_buf, 0, BLOCK_SIZE);

    if (read_block(file_block.contents.inode.data_blocks[i], data_block_buf) != 0) {
      free(data_block_buf);
      return E_UNKNOWN;
    }
    if (i == num_blocks_to_read - 1) {
      // Last block to read
      unsigned short remaining_bytes = count - total_bytes_read;
      memcpy(data, data_block_buf, remaining_bytes);
      total_bytes_read += remaining_bytes;
      data += remaining_bytes;
    } else {
      memcpy(data, data_block_buf, BLOCK_SIZE);
      total_bytes_read += BLOCK_SIZE;
      data += BLOCK_SIZE;
    }
    free(data_block_buf);
  }
  *ptr_count = total_bytes_read;
  data = initial_data;
  return E_SUCCESS;
}

/* jfs_unmount
 *   makes the file system no longer accessible (unless it is mounted again).
 *   This should be called exactly once after all other jfs_* operations are
 *   complete; it is invalid to call any other jfs_* function (except
 *   jfs_mount) after this function complete.  Basically, this closes the DISK
 *   file on the _real_ file system.  If your code requires any clean up after
 *   all other jfs_* functions are done, you may add it here.
 * returns 0 on success or -1 on error; errors should only occur due to
 *   errors in the underlying disk syscalls.
 */
int jfs_unmount() {
  int ret = bfs_unmount();
  return ret;
}
