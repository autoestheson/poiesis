#ifndef FS_H
#define FS_H

#include <stdint.h>

#define FILENAME_LEN 32

#define BLOCK_BYTES 512

#define FS_FTYPE_F 0x00
#define FS_FTYPE_D 0x01

// Allocation table:
// Each block has a respective entry, indicating that it is:
#define FS_TBLENT_FREE 0x00000000 // available for use;
//        0x00000001 - 0xFFFFFFEF :: indicating the next entry;
#define FS_TBLENT_EKFS 0xFFFFFFF0 // used by the system;
#define FS_TBLENT_NULL 0xFFFFFFFF // the last block in a chain.
// This, along with each block's linear index into the table,
// allows us to represent files as chains of entries.

// Directories:
// Each directory consists of several entries, each serving as:
#define FS_DIRENT_NULL 0x00000000 // the end of a directory;
//        0x00000000 - 0xFFFFFFFD :: a normal directory entry;
#define FS_DIRENT_GARB 0xFFFFFFFE // garbage (deleted);
#define FS_DIRENT_ROOT 0xFFFFFFFF // the root directory.

// These are all indexes into the disk of 32-bit integers representing:
#define FS_MAGIC_AT       4  // The string "EKFS"
#define FS_DISK_BLOCKS_AT 8  // The number of blocks in the disk
#define FS_ROOT_BLOCKS_AT 12 // The number of blocks in the root directory
#define FS_SKIP_BLOCKS_AT 16 // The number of blocks reserved by the operating system
#define FS_DISK_ID_AT     20 // The disk identification code

struct dirent {
    uint32_t parent_id;
    uint32_t target_id;
    uint64_t ctime; // Time of creation
    uint64_t mtime; // Time of (last) modification
    uint32_t size;
    uint8_t type;
    uint8_t perms;
    uint16_t xattr;
    char name[FILENAME_LEN];
};

#endif