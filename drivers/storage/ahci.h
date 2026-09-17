#ifndef AHCI_H
#define AHCI_H

#include <stdint.h>
#include <stdbool.h>

void init_storage(void);

// Basic Read/Write API (LBA48)
// Reads/writes count sectors starting from lba into buf
bool ahci_read_sectors(uint64_t lba, uint32_t count, void *buf);
bool ahci_write_sectors(uint64_t lba, uint32_t count, void *buf);

// Stub NVMe interface
void init_nvme_queue_stub(void);

#endif
