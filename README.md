# Jumbo File System (JFS)
Jumbo File System (JFS) is a simple direct indexed file system that provides functions similar to those found in Linux (POSIX) file systems, allowing you to interact with files and directories.

## Features
JFS includes the following features:

- Mounting and unmounting of the file system.
- Creating and removing directories.
- Changing the current working directory.
- Listing files and subdirectories in the current directory.
- Creating and deleting files.
- Retrieving file and directory information.
- Appending data to files.
- Reading data from files.

## Usage
To use JFS, follow these steps:

1) Mount the File System: Call jfs_mount with the filename of the simulated disk to prepare it for file system operations. This should be done once before using any other functions.

2) Interact with the File System: Use the various JFS functions to perform file system operations, such as creating directories, listing files, creating files, appending data, and more.

3) Unmount the File System: Call jfs_unmount to cleanly unmount the file system when you are finished. This should be done once after all other operations.

## Functions
Here is an overview of the main functions provided by JFS:

****jfs_mount(const char* filename):*** Initializes the file system by mounting the specified disk file.

***jfs_unmount():*** Unmounts the file system and performs cleanup.

****jfs_mkdir(const char* directory_name):**** Creates a new subdirectory in the current directory.

***jfs_chdir(const char* directory_name):*** Changes the current working directory. Passing NULL sets it to the root directory.

***jfs_ls(char* directories[MAX_DIR_ENTRIES+1], char* files[MAX_DIR_ENTRIES+1]):*** Lists the subdirectories and files in the current directory.

***jfs_rmdir(const char* directory_name):*** Removes a subdirectory from the current directory.

***jfs_creat(const char* file_name):*** Creates an empty file.

***jfs_remove(const char* file_name):*** Deletes a file.

****jfs_stat(const char* name, struct stats* buf):*** Retrieves information about a file or directory.

****jfs_write(const char* file_name, const void* buf, unsigned short count):*** Appends data to a file.

******jfs_read(const char* file_name, const void* buf, unsigned short* ptr_count):*** Reads data from a file into a buffer.
