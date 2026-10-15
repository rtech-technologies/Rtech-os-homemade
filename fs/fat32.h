#ifndef FAT32_H
#define FAT32_H

#include <stdint.h>
#include <stdbool.h>

void init_fat32(void);

// Basic Read/Write API
bool fs_read_file(const char *path, void *out_buf);
bool fs_write_file(const char *path, const void *in_buf, uint32_t size);
void fs_list_dir(const char *path); // Just prints to screen via debug or we can stub

#endif
