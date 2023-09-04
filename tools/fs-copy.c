#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

#include "include/fs.h"

#define SEARCH_FAILURE 0xFFFFFFFF

#define BLOCK_ENTRIES (BLOCK_BYTES / sizeof(struct dirent))

struct path_result {
    struct dirent parent;
    struct dirent target;
    uint32_t target_dirent;
    char name[FILENAME_LEN];
    bool failure;
    bool not_found;
};

FILE *disk;
uint32_t disk_size;

uint32_t disk_blocks, root_blocks, skip_blocks, atbl_blocks;
uint32_t root_start, atbl_start;

uint32_t rd_dword(uint32_t pos) {
    uint32_t val = 0;
    
    fseek(disk, pos, SEEK_SET);
    fread(&val, sizeof(uint32_t), 1, disk);
    
    return val;
}

void rd_dirent(struct dirent *res, uint32_t dirent) {
    uint32_t pos = (root_start * BLOCK_BYTES) + (dirent * sizeof(struct dirent));
    
    if (pos >= (root_start + root_blocks) * BLOCK_BYTES) {
        printf("Error: That directory is out of this world!\n");
        exit(1);
    }
    
    fseek(disk, pos, SEEK_SET);
    fread(res, sizeof(struct dirent), 1, disk);
}

void wr_dirent(uint32_t dirent, struct dirent *dirent_src) {
    uint32_t pos = (root_start * BLOCK_BYTES) + (dirent * sizeof(struct dirent));
    
    if (pos >= (root_start + root_blocks) * BLOCK_BYTES) {
        printf("Error: That directory is out of this world!\n");
        exit(1);
    }
    
    fseek(disk, pos, SEEK_SET);
    fwrite(dirent_src, sizeof(struct dirent), 1, disk);
}

uint32_t import_chain(FILE *source) {
    uint8_t *block_buf = malloc(BLOCK_BYTES);
    if (!block_buf) {
        printf("Error: Out of memory; please purchase a RAM expansion.\n");
        exit(1);
    }
    
    fseek(source, 0, SEEK_END);
    uint32_t source_size = ftell(source);
    rewind(source);
    
    if (!source_size)
        return FS_TBLENT_NULL;
    
    uint32_t source_size_blocks = (source_size + BLOCK_BYTES - 1) / BLOCK_BYTES;
    
    uint32_t *blocklist = malloc(source_size_blocks * sizeof(uint32_t));
    if (!blocklist) {
        printf("Error: Out of memory; please purchase a RAM expansion.\n");
        exit(1);
    }
    
    fseek(disk, atbl_start * BLOCK_BYTES, SEEK_SET);
    uint32_t block = 0;
    for (uint32_t i = 0; i < source_size_blocks; i++) {
        uint32_t vvv;
        for (; fread(&vvv, sizeof(uint32_t), 1, disk), vvv; block++);
        blocklist[i] = block++;
    }
    
    for (uint32_t i = 0; i < source_size_blocks; i++) {
        fseek(disk, blocklist[i] * BLOCK_BYTES, SEEK_SET);
        fwrite(block_buf, 1, fread(block_buf, 1, BLOCK_BYTES, source), disk);
    }
    
    for (uint32_t i = 0;; i++) {
        fseek(disk, atbl_start * BLOCK_BYTES + blocklist[i] * sizeof(uint32_t), SEEK_SET);
        if (i == source_size_blocks - 1) {
            uint32_t vvv = FS_TBLENT_NULL;
            fwrite(&vvv, sizeof(uint32_t), 1, disk);
            break;
        }
        fwrite(&blocklist[i+1], sizeof(uint32_t), 1, disk);
    }
    
    block = blocklist[0];
    
    free(blocklist);
    free(block_buf);
    return block;
}

// returns unique dirent id, SEARCH_FAILURE upon failure/not found
uint32_t search(const char *name, uint32_t parent, uint8_t type) {
    struct dirent dirent;
    
    fseek(disk, root_start * BLOCK_BYTES, SEEK_SET);
    for (uint32_t i = 0;; i++) {
        fread(&dirent, sizeof(struct dirent), 1, disk);
        if (!dirent.parent_id)
            return SEARCH_FAILURE;
        if (i >= (root_blocks * BLOCK_ENTRIES))
            return SEARCH_FAILURE;
        if ((dirent.parent_id == parent) && (dirent.type == type) && (!strcmp(dirent.name, name)))
            return i;
    }
}

// returns a struct of useful info
// failure flag set upon failure
// not_found flag set upon not found
// even if the file is not found, info about the "parent"
// directory and name are still returned
struct path_result path_resolver(const char *path, uint8_t type) {
    char name[FILENAME_LEN];
    struct dirent parent = {0};
    int last = 0;
    int i;
    struct path_result result;
    struct dirent empty_dirent = {0};
    
    result.name[0] = 0;
    result.target_dirent = 0;
    result.parent = empty_dirent;
    result.target = empty_dirent;
    result.failure = false;
    result.not_found = false;
    
    parent.target_id = FS_DIRENT_ROOT;
    
    if ((type == FS_FTYPE_D) && !strcmp(path, "/")) {
        result.target.target_id = FS_DIRENT_ROOT;
        return result;
    }
    
    if ((type == FS_FTYPE_F) && !strcmp(path, "/")) {
        result.failure = true;
        return result;
    }
    
    if (*path == '/')
        path++;
    
next:
    for (i = 0; *path != '/'; path++) {
        if (!*path) {
            last = 1;
            break;
        }
        
        name[i++] = *path;
    }
    
    name[i] = 0;
    path++;
    
    if (!last) {
        uint32_t search_res = search(name, parent.target_id, FS_FTYPE_D);
        if (search_res == SEARCH_FAILURE) {
            result.failure = true; // fail if search fails
            return result;
        }
        rd_dirent(&parent, search_res);
    } else {
        uint32_t search_res = search(name, parent.target_id, type);
        if (search_res == SEARCH_FAILURE) {
            result.not_found = true;
        } else {
            rd_dirent(&result.target, search_res);
            result.target_dirent = search_res;
        }
        
        result.parent = parent;
        strcpy(result.name, name);
        return result;
    }
    
    goto next;
}

uint32_t get_free_id() {
    uint32_t id = 1;
    uint32_t i;
    
    uint32_t pos = (root_start * BLOCK_BYTES);
    fseek(disk, pos, SEEK_SET);
    
    for (i = 0;; i++) {
        struct dirent dirent;
        fread(&dirent, sizeof(struct dirent), 1, disk);
        if (!dirent.parent_id)
            break;
        if ((dirent.type == 1) && (dirent.target_id == id))
            id = (dirent.target_id + 1);
    }
    
    return id;
}

void mkdir_cmd(char *newdirname) {
    uint32_t i;
    struct dirent dirent = {0};
    
    struct path_result path_result = path_resolver(newdirname, FS_FTYPE_D);
    
    // check if it exists
    if (!(path_result.not_found)) {
        printf("Error: Directory \"%s\" already exists\n", newdirname);
        return;
    }
    
    // find empty dirent
    uint32_t pos = (root_start * BLOCK_BYTES);
    fseek(disk, pos, SEEK_SET);
    for (i = 0;; i++) {
        struct dirent dirent_i;
        fread(&dirent_i, sizeof(struct dirent), 1, disk);
        if ((dirent_i.parent_id == 0) || (dirent_i.parent_id == FS_DIRENT_GARB))
            break;
    }
    
    dirent.parent_id = path_result.parent.target_id;
    dirent.target_id = get_free_id();
    dirent.ctime = dirent.mtime = time(NULL);
    dirent.size = 0;
    dirent.type = FS_FTYPE_D;
    dirent.perms = 0;
    dirent.xattr = 0;
    strcpy(dirent.name, path_result.name);
    
    wr_dirent(i, &dirent);
}

void import_cmd(char *source_name, char *destination_name) {
    FILE *source;
    struct dirent dirent = {0};
    uint32_t i;
    
    if (path_resolver(destination_name, FS_FTYPE_F).failure) {
        char newdirname[4096];
        int i = 0;
subdir:
        for (;; i++) {
            if (destination_name[i] == '/')
                break;
            newdirname[i] = destination_name[i];
        }
        newdirname[i] = 0;
        mkdir_cmd(newdirname);
        if (path_resolver(destination_name, FS_FTYPE_F).failure) {
            newdirname[i++] = '/';
            goto subdir;
        }
    }
    
    struct path_result path_result = path_resolver(destination_name, FS_FTYPE_F);
    
    if (!path_result.not_found) {
        printf("Error: File \"%s\" already exists\n", source_name);
        return;
    }
    
    if ((source = fopen(source_name, "r")) == NULL) {
        printf("Error: Failed to open file \"%s\"\n", source_name);
        return;
    }
    
    uint32_t payload = import_chain(source);
    
    dirent.parent_id = path_result.parent.target_id;
    dirent.target_id = payload;
    dirent.ctime = dirent.mtime = time(NULL);
    fseek(source, 0, SEEK_END);
    dirent.size = ftell(source);
    dirent.type = FS_FTYPE_F;
    dirent.perms = 0;
    dirent.xattr = 0;
    strcpy(dirent.name, path_result.name);
    
    // find empty dirent
    uint32_t pos = root_start * BLOCK_BYTES;
    fseek(disk, pos, SEEK_SET);
    for (i = 0;; i++) {
        struct dirent dirent_i;
        fread(&dirent_i, sizeof(struct dirent), 1, disk);
        if ((dirent_i.parent_id == 0) || (dirent_i.parent_id == FS_DIRENT_GARB))
            break;
    }
    wr_dirent(i, &dirent);
    
    fclose(source);
}

int main(int argc, char **argv) {
    if (argc < 4) {
        printf("Usage: fs-copy [disk image] [source file] [destination file]\n");
        exit(1);
    }
    
    if (!(disk = fopen(argv[1], "r+"))) {
        printf("Error: Failed to open image \"%s\"\n", argv[1]);
        exit(1);
    }
    
    fseek(disk, 0, SEEK_END);
    disk_size = ftell(disk);
    rewind(disk);
    
    if (disk_size % BLOCK_BYTES) {
        printf("Error: Image \"%s\" is not block-aligned\n", argv[1]);
        fclose(disk);
        exit(1);
    }
    
    char signature[4] = {0};
    fseek(disk, 4, SEEK_SET);
    fread(signature, 4, 1, disk);
    if (strncmp(signature, "EKFS", 4)) {
        printf("Error: Image \"%s\" is not a valid filesystem\n", argv[1]);
        fclose(disk);
        exit(1);
    }
    
    skip_blocks = rd_dword(FS_SKIP_BLOCKS_AT);
    atbl_start = skip_blocks;
    
    disk_blocks = disk_size / BLOCK_BYTES;
    if (rd_dword(FS_DISK_BLOCKS_AT) != disk_blocks)
        printf("Warning: Image \"%s\" lied about its size\n", argv[1]);
    
    atbl_blocks = (disk_blocks * sizeof(uint32_t)) / BLOCK_BYTES;
    
    if ((disk_blocks * sizeof(uint32_t)) % BLOCK_BYTES)
        atbl_blocks++;
    
    root_blocks = rd_dword(FS_ROOT_BLOCKS_AT);
    
    root_start = atbl_start + atbl_blocks;
    
    import_cmd(argv[2], argv[3]);
    
    fclose(disk);
}
