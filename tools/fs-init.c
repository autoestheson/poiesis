#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "include/fs.h"
#include "boot/bootblock.h"

FILE *disk;
uint32_t disk_bytes;

uint32_t disk_blocks, root_blocks, skip_blocks, atbl_blocks;

uint32_t rd_dword(uint32_t pos) {
    uint32_t val = 0;
    
    fseek(disk, pos, SEEK_SET);
    fread(&val, sizeof(uint32_t), 1, disk);
    
    return val;
}

void wr_dword(uint32_t pos, uint32_t val) {
    fseek(disk, pos, SEEK_SET);
    fwrite(&val, sizeof(uint32_t), 1, disk);
}

void format_pass1(int argc, char **argv, int quick) {
    disk_blocks = disk_bytes / BLOCK_BYTES;
    root_blocks = disk_blocks / 20;
    skip_blocks = 16;
    
    fseek(disk, 0, SEEK_SET);
    fwrite(bootblock, 512, 1, disk);
    
    fseek(disk, FS_MAGIC_AT, SEEK_SET);
    fputs("EKFS", disk);
    
    wr_dword(FS_DISK_BLOCKS_AT, disk_blocks);
    wr_dword(FS_ROOT_BLOCKS_AT, root_blocks);
    wr_dword(FS_SKIP_BLOCKS_AT, skip_blocks);
    
    wr_dword(FS_DISK_ID_AT, 0xFFFFFFFF);
    
    if (!quick) {
        fseek(disk, skip_blocks * BLOCK_BYTES, SEEK_SET);
        
        // zero out the rest of the disk
        uint8_t *zeroblock = calloc(BLOCK_BYTES, 1);
        if (!zeroblock) {
            printf("Error: Memory allocation error\n");
            exit(1);
        }
        
        for (uint32_t i = skip_blocks * BLOCK_BYTES; i < disk_bytes; i += BLOCK_BYTES)
            fwrite(zeroblock, BLOCK_BYTES, 1, disk);
        
        free(zeroblock);
    }
}

void format_pass2() {
    uint32_t pos = skip_blocks * BLOCK_BYTES;
    
    for (uint32_t i = 0; i < skip_blocks + atbl_blocks + root_blocks; i++) {
        wr_dword(pos, FS_TBLENT_EKFS);
        pos += sizeof(uint32_t);
    }
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: fs-copy [disk image]\n");
        exit(1);
    }
    
    if (!(disk = fopen(argv[1], "r+"))) {
        printf("Error: Failed to open image \"%s\"\n", argv[1]);
        exit(1);
    }
    
    fseek(disk, 0, SEEK_END);
    disk_bytes = ftell(disk);
    rewind(disk);
    
    if (disk_bytes % BLOCK_BYTES) {
        printf("Error: Image \"%s\" is not block-aligned\n", argv[1]);
        fclose(disk);
        exit(1);
    }
    
    format_pass1(argc, argv, 0);
    
    atbl_blocks = (disk_blocks * sizeof(uint32_t)) / BLOCK_BYTES;
    
    if ((disk_blocks * sizeof(uint32_t)) % BLOCK_BYTES)
        atbl_blocks++;
    
    format_pass2();
    
    fclose(disk);
}
